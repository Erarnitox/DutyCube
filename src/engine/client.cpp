// client.cpp, mostly network related client game code

#include "engine/engine.h"

ENetHost* clienthost = nullptr;
ENetPeer *curpeer = nullptr, *connpeer = nullptr;
int connmillis = 0, connattempts = 0, discmillis = 0;

auto multiplayer(bool msg) -> bool {
	bool val = curpeer || hasnonlocalclients();
	if (val && msg)
		conoutf(CON_ERROR, "operation not available in multiplayer");
	return val;
}

void setrate(int rate) {
	if (!curpeer)
		return;
	enet_host_bandwidth_limit(clienthost, rate * 1024, rate * 1024);
}

VARF(rate, 0, 0, 1024, setrate(rate));

void throttle();

VARF(throttle_interval, 0, 5, 30, throttle());
VARF(throttle_accel, 0, 2, 32, throttle());
VARF(throttle_decel, 0, 2, 32, throttle());

void throttle() {
	if (!curpeer)
		return;
	assert(ENET_PEER_PACKET_THROTTLE_SCALE == 32);
	enet_peer_throttle_configure(curpeer, throttle_interval * 1000, throttle_accel, throttle_decel);
}

auto isconnected(bool attempt, bool local) -> bool {
	return curpeer || (attempt && connpeer) || (local && haslocalclients());
}

ICOMMAND(isconnected, "bb", (int* attempt, int* local), intret(isconnected(*attempt > 0, *local != 0) ? 1 : 0));

auto connectedpeer() -> const ENetAddress* {
	return curpeer ? &curpeer->address : nullptr;
}

ICOMMAND(connectedip, "", (), {
	const ENetAddress* address = connectedpeer();
	string hostname;
	result(address && enet_address_get_host_ip(address, hostname, sizeof(hostname)) >= 0 ? hostname : "");
});

ICOMMAND(connectedport, "", (), {
	const ENetAddress* address = connectedpeer();
	intret(address ? address->port : -1);
});

void abortconnect() {
	if (!connpeer)
		return;
	game::connectfail();
	if (connpeer->state != ENET_PEER_STATE_DISCONNECTED)
		enet_peer_reset(connpeer);
	connpeer = nullptr;
	if (curpeer)
		return;
	enet_host_destroy(clienthost);
	clienthost = nullptr;
}

SVARP(connectname, "");
VARP(connectport, 0, 0, 0xFFFF);

void connectserv(const char* servername, int serverport, const char* serverpassword) {
	if (connpeer) {
		conoutf("aborting connection attempt");
		abortconnect();
	}

	if (serverport <= 0)
		serverport = server::serverport();

	ENetAddress address;
	address.port = serverport;

	if (servername) {
		if (strcmp(servername, connectname))
			setsvar("connectname", servername);
		if (serverport != connectport)
			setvar("connectport", serverport);
		addserver(servername, serverport, serverpassword && serverpassword[0] ? serverpassword : nullptr);
		conoutf("attempting to connect to %s:%d", servername, serverport);
		if (!resolverwait(servername, &address)) {
			conoutf("\f3could not resolve server %s", servername);
			return;
		}
	} else {
		setsvar("connectname", "");
		setvar("connectport", 0);
		conoutf("attempting to connect over LAN");
		address.host = ENET_HOST_BROADCAST;
	}

	if (!clienthost) {
		clienthost = enet_host_create(nullptr, 2, server::numchannels(), rate * 1024, rate * 1024);
		if (!clienthost) {
			conoutf("\f3could not connect to server");
			return;
		}
		clienthost->duplicatePeers = 0;
	}

	connpeer = enet_host_connect(clienthost, &address, server::numchannels(), 0);
	enet_host_flush(clienthost);
	connmillis = totalmillis;
	connattempts = 0;

	game::connectattempt(servername ? servername : "", serverpassword ? serverpassword : "", address);
}

void reconnect(const char* serverpassword) {
	if (!connectname[0] || connectport <= 0) {
		conoutf(CON_ERROR, "no previous connection");
		return;
	}

	connectserv(connectname, connectport, serverpassword);
}

void disconnect(bool async, bool cleanup) {
	if (curpeer) {
		if (!discmillis) {
			enet_peer_disconnect(curpeer, DISC_NONE);
			enet_host_flush(clienthost);
			discmillis = totalmillis;
		}
		if (curpeer->state != ENET_PEER_STATE_DISCONNECTED) {
			if (async)
				return;
			enet_peer_reset(curpeer);
		}
		curpeer = nullptr;
		discmillis = 0;
		conoutf("disconnected");
		game::gamedisconnect(cleanup);
		mainmenu = 1;
	}
	if (!connpeer && clienthost) {
		enet_host_destroy(clienthost);
		clienthost = nullptr;
	}
}

void trydisconnect(bool local) {
	if (connpeer) {
		conoutf("aborting connection attempt");
		abortconnect();
	} else if (curpeer) {
		conoutf("attempting to disconnect...");
		disconnect(!discmillis);
	} else if (local && haslocalclients())
		localdisconnect();
	else
		conoutf("not connected");
}

ICOMMAND(connect, "sis", (char* name, int* port, char* pw), connectserv(name, *port, pw));
ICOMMAND(lanconnect, "is", (int* port, char* pw), connectserv(NULL, *port, pw));
COMMAND(reconnect, "s");
ICOMMAND(disconnect, "b", (int* local), trydisconnect(*local != 0));
ICOMMAND(localconnect, "", (), {
	if (!isconnected())
		localconnect();
});
ICOMMAND(localdisconnect, "", (), {
	if (haslocalclients())
		localdisconnect();
});

void sendclientpacket(ENetPacket* packet, int chan) {
	if (curpeer)
		enet_peer_send(curpeer, chan, packet);
	else
		localclienttoserver(chan, packet);
}

void flushclient() {
	if (clienthost)
		enet_host_flush(clienthost);
}

void neterr(const char* s, bool disc) {
	conoutf(CON_ERROR, "\f3illegal network message (%s)", s);
	if (disc)
		disconnect();
}

void localservertoclient(int chan, ENetPacket* packet)	// processes any updates from the server
{
	packetbuf p(packet);
	game::parsepacketclient(chan, p);
}

void clientkeepalive() {
	if (clienthost)
		enet_host_service(clienthost, nullptr, 0);
}

void gets2c()  // get updates from the server
{
	ENetEvent event;
	if (!clienthost)
		return;
	if (connpeer && totalmillis / 3000 > connmillis / 3000) {
		conoutf("attempting to connect...");
		connmillis = totalmillis;
		++connattempts;
		if (connattempts > 3) {
			conoutf("\f3could not connect to server");
			abortconnect();
			return;
		}
	}
	while (clienthost && enet_host_service(clienthost, &event, 0) > 0)
		switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT:
			disconnect(false, false);
			localdisconnect(false);
			curpeer = connpeer;
			connpeer = nullptr;
			conoutf("connected to server");
			throttle();
			if (rate)
				setrate(rate);
			game::gameconnect(true);
			break;

		case ENET_EVENT_TYPE_RECEIVE:
			if (discmillis)
				conoutf("attempting to disconnect...");
			else
				localservertoclient(event.channelID, event.packet);
			enet_packet_destroy(event.packet);
			break;

		case ENET_EVENT_TYPE_DISCONNECT:
			if (event.data >= DISC_NUM)
				event.data = DISC_NONE;
			if (event.peer == connpeer) {
				conoutf("\f3could not connect to server");
				abortconnect();
			} else {
				if (!discmillis || event.data) {
					const char* msg = disconnectreason(event.data);
					if (msg)
						conoutf("\f3server network error, disconnecting (%s) ...", msg);
					else
						conoutf("\f3server network error, disconnecting...");
				}
				disconnect();
			}
			return;

		default:
			break;
		}
}
