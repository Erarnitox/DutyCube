#include "game/game.h"

namespace entities {
using namespace game;

auto extraentinfosize() -> int {
	return 0;
}  // size in bytes of what the 2 methods below read/write... so it can be skipped by other games

void writeent(entity& e, char* buf)	 // write any additional data to disk (except for ET_ ents)
{
}

void readent(entity& e, char* buf, int ver)	 // read from disk, and init
{
}

#ifndef STANDALONE
vector<extentity*> ents;

auto getents() -> vector<extentity*>& {
	return ents;
}

auto mayattach(extentity& e) -> bool {
	return false;
}
auto attachent(extentity& e, extentity& a) -> bool {
	return false;
}

auto itemname(int i) -> const char* {
	return nullptr;
#if 0
        int t = ents[i]->type;
        if(!validitem(t)) return NULL;
        return itemstats[t-I_FIRST].name;
#endif
}

auto itemicon(int i) -> int {
	return -1;
#if 0
        int t = ents[i]->type;
        if(!validitem(t)) return -1;
        return itemstats[t-I_FIRST].icon;
#endif
}

auto entmdlname(int type) -> const char* {
	static const char* const entmdlnames[MAXENTTYPES] = {
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "game/teleport", nullptr, nullptr, nullptr};
	return entmdlnames[type];
}

auto entmodel(const entity& e) -> const char* {
	if (e.type == TELEPORT) {
		if (e.attr2 > 0)
			return mapmodelname(e.attr2);
		if (e.attr2 < 0)
			return nullptr;
	}
	return e.type < MAXENTTYPES ? entmdlname(e.type) : nullptr;
}

void preloadentities() {
	loopi(MAXENTTYPES) {
		const char* mdl = entmdlname(i);
		if (!mdl)
			continue;
		preloadmodel(mdl);
	}
	loopv(ents) {
		extentity& e = *ents[i];
		switch (e.type) {
		case TELEPORT:
			if (e.attr2 > 0)
				preloadmodel(mapmodelname(e.attr2));
		case JUMPPAD:
			if (e.attr4 > 0)
				preloadmapsound(e.attr4);
			break;
		}
	}
}

void renderentities() {
	loopv(ents) {
		extentity& e = *ents[i];
		int revs = 10;
		switch (e.type) {
		case TELEPORT:
			if (e.attr2 < 0)
				continue;
			break;
		default:
			if (!e.spawned() || !validitem(e.type))
				continue;
			break;
		}
		const char* mdlname = entmodel(e);
		if (mdlname) {
			vec p = e.o;
			p.z += 1 + sinf(lastmillis / 100.0 + e.o.x + e.o.y) / 20;
			rendermodel(mdlname,
						ANIM_MAPMODEL | ANIM_LOOP,
						p,
						lastmillis / (float)revs,
						0,
						0,
						MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED);
		}
	}
}

void addammo(int type, int& v, bool local) {
#if 0
        itemstat &is = itemstats[type-I_FIRST];
        v += is.add;
        if(v>is.max) v = is.max;
        if(local) msgsound(is.sound);
#endif
}

// these two functions are called when the server acknowledges that you really
// picked up the item (in multiplayer someone may grab it before you).

void pickupeffects(int n, gameent* d) {
#if 0
        if(!ents.inrange(n)) return;
        int type = ents[n]->type;
        if(!validitem(type)) return;
        ents[n]->clearspawned();
        if(!d) return;
        itemstat &is = itemstats[type-I_FIRST];
        if(d!=player1 || isthirdperson())
        {
            //particle_text(d->abovehead(), is.name, PART_TEXT, 2000, 0xFFC864, 4.0f, -8);
            particle_icon(d->abovehead(), is.icon%4, is.icon/4, PART_HUD_ICON_GREY, 2000, 0xFFFFFF, 2.0f, -8);
        }
        playsound(itemstats[type-I_FIRST].sound, d!=player1 ? &d->o : NULL, NULL, 0, 0, 0, -1, 0, 1500);
        d->pickup(type);
        if(d==player1) switch(type)
        {
        }
#endif
}

// these functions are called when the client touches the item

void teleporteffects(gameent* d, int tp, int td, bool local) {
	if (ents.inrange(tp) && ents[tp]->type == TELEPORT) {
		extentity& e = *ents[tp];
		if (e.attr4 >= 0) {
			int snd = S_TELEPORT, flags = 0;
			if (e.attr4 > 0) {
				snd = e.attr4;
				flags = SND_MAP;
			}
			if (d == player1)
				playsound(snd, nullptr, nullptr, flags);
			else {
				playsound(snd, &e.o, nullptr, flags);
				if (ents.inrange(td) && ents[td]->type == TELEDEST)
					playsound(snd, &ents[td]->o, nullptr, flags);
			}
		}
	}
	if (local && d->clientnum >= 0) {
		sendposition(d);
		packetbuf p(32, ENET_PACKET_FLAG_RELIABLE);
		putint(p, N_TELEPORT);
		putint(p, d->clientnum);
		putint(p, tp);
		putint(p, td);
		sendclientpacket(p.finalize(), 0);
		flushclient();
	}
}

void jumppadeffects(gameent* d, int jp, bool local) {
	if (ents.inrange(jp) && ents[jp]->type == JUMPPAD) {
		extentity& e = *ents[jp];
		if (e.attr4 >= 0) {
			int snd = S_JUMPPAD, flags = 0;
			if (e.attr4 > 0) {
				snd = e.attr4;
				flags = SND_MAP;
			}
			if (d == player1)
				playsound(snd, nullptr, nullptr, flags);
			else
				playsound(snd, &e.o, nullptr, flags);
		}
	}
	if (local && d->clientnum >= 0) {
		sendposition(d);
		packetbuf p(16, ENET_PACKET_FLAG_RELIABLE);
		putint(p, N_JUMPPAD);
		putint(p, d->clientnum);
		putint(p, jp);
		sendclientpacket(p.finalize(), 0);
		flushclient();
	}
}

void teleport(int n, gameent* d)  // also used by monsters
{
	int e = -1, tag = ents[n]->attr1, beenhere = -1;
	for (;;) {
		e = findentity(TELEDEST, e + 1);
		if (e == beenhere || e < 0) {
			conoutf(CON_WARN, "no teleport destination for tag %d", tag);
			return;
		}
		if (beenhere < 0)
			beenhere = e;
		if (ents[e]->attr2 == tag) {
			teleporteffects(d, n, e, true);
			d->o = ents[e]->o;
			d->yaw = ents[e]->attr1;
			if (ents[e]->attr3 > 0) {
				vec dir;
				vecfromyawpitch(d->yaw, 0, 1, 0, dir);
				float speed = d->vel.magnitude2();
				d->vel.x = dir.x * speed;
				d->vel.y = dir.y * speed;
			} else
				d->vel = vec(0, 0, 0);
			entinmap(d);
			updatedynentcache(d);
			ai::inferwaypoints(d, ents[n]->o, ents[e]->o, 16.f);
			break;
		}
	}
}

VARR(teleteam, 0, 1, 1);

void trypickup(int n, gameent* d) {
	switch (ents[n]->type) {
	default:
		if (d->canpickup(ents[n]->type)) {
			addmsg(N_ITEMPICKUP, "rci", d, n);
			ents[n]->clearspawned();  // even if someone else gets it first
		}
		break;

	case TELEPORT: {
		if (d->lastpickup == ents[n]->type && lastmillis - d->lastpickupmillis < 500)
			break;
		if (!teleteam && m_teammode)
			break;
		if (ents[n]->attr3 > 0) {
			defformatstring(hookname, "can_teleport_%d", ents[n]->attr3);
			if (!execidentbool(hookname, true))
				break;
		}
		d->lastpickup = ents[n]->type;
		d->lastpickupmillis = lastmillis;
		teleport(n, d);
		break;
	}

	case JUMPPAD: {
		if (d->lastpickup == ents[n]->type && lastmillis - d->lastpickupmillis < 300)
			break;
		d->lastpickup = ents[n]->type;
		d->lastpickupmillis = lastmillis;
		jumppadeffects(d, n, true);
		if (d->ai)
			d->ai->becareful = true;
		d->falling = vec(0, 0, 0);
		d->physstate = PHYS_FALL;
		d->timeinair = 1;
		d->vel = vec(ents[n]->attr3 * 10.0f, ents[n]->attr2 * 10.0f, ents[n]->attr1 * 12.5f);
		break;
	}
	}
}

void checkitems(gameent* d) {
	if (d->state != CS_ALIVE)
		return;
	vec o = d->feetpos();
	loopv(ents) {
		extentity& e = *ents[i];
		if (e.type == NOTUSED)
			continue;
		if (!e.spawned() && e.type != TELEPORT && e.type != JUMPPAD)
			continue;
		float dist = e.o.dist(o);
		if (dist < (e.type == TELEPORT ? 16 : 12))
			trypickup(i, d);
	}
}

void putitems(packetbuf& p)	 // puts items in network stream and also spawns them locally
{
	putint(p, N_ITEMLIST);
	loopv(ents) if (validitem(ents[i]->type)) {
		putint(p, i);
		putint(p, ents[i]->type);
	}
	putint(p, -1);
}

void resetspawns() {
	loopv(ents) ents[i]->clearspawned();
}

void spawnitems(bool force) {
	loopv(ents) if (validitem(ents[i]->type)) {
		ents[i]->setspawned(force || !server::delayspawn(ents[i]->type));
	}
}

void setspawn(int i, bool on) {
	if (ents.inrange(i))
		ents[i]->setspawned(on);
}

auto newentity() -> extentity* {
	return new gameentity();
}
void deleteentity(extentity* e) {
	delete (gameentity*)e;
}

void clearents() {
	while (ents.length())
		deleteentity(ents.pop());
}

void animatemapmodel(const extentity& e, int& anim, int& basetime) {
}

void fixentity(extentity& e) {
	switch (e.type) {
	case FLAG:
		e.attr5 = e.attr4;
		e.attr4 = e.attr3;
	case TELEDEST:
		e.attr3 = e.attr2;
		e.attr2 = e.attr1;
		e.attr1 = (int)player1->yaw;
		break;
	}
}

void entradius(extentity& e, bool color) {
	switch (e.type) {
	case TELEPORT:
		loopv(ents) if (ents[i]->type == TELEDEST && e.attr1 == ents[i]->attr2) {
			renderentarrow(e, vec(ents[i]->o).sub(e.o).normalize(), e.o.dist(ents[i]->o));
			break;
		}
		break;

	case JUMPPAD:
		renderentarrow(e, vec((int)(char)e.attr3 * 10.0f, (int)(char)e.attr2 * 10.0f, e.attr1 * 12.5f).normalize(), 4);
		break;

	case FLAG:
	case TELEDEST: {
		vec dir;
		vecfromyawpitch(e.attr1, 0, 1, 0, dir);
		renderentarrow(e, dir, 4);
		break;
	}
	}
}

auto printent(extentity& e, char* buf, int len) -> bool {
	return false;
}

auto entnameinfo(entity& e) -> const char* {
	return "";
}
auto entname(int i) -> const char* {
	static const char* const entnames[MAXENTTYPES] = {"none?",
													  "light",
													  "mapmodel",
													  "playerstart",
													  "envmap",
													  "particles",
													  "sound",
													  "spotlight",
													  "decal",
													  "teleport",
													  "teledest",
													  "jumppad",
													  "flag"};
	return i >= 0 && size_t(i) < sizeof(entnames) / sizeof(entnames[0]) ? entnames[i] : "";
}

void editent(int i, bool local) {
	extentity& e = *ents[i];
	// e.flags = 0;
	if (local)
		addmsg(N_EDITENT,
			   "rii3ii5",
			   i,
			   (int)(e.o.x * DMF),
			   (int)(e.o.y * DMF),
			   (int)(e.o.z * DMF),
			   e.type,
			   e.attr1,
			   e.attr2,
			   e.attr3,
			   e.attr4,
			   e.attr5);
}

auto dropheight(entity& e) -> float {
	if (e.type == FLAG)
		return 0.0f;
	return 4.0f;
}
#endif
}  // namespace entities
