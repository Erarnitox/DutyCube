#include "game/game.h"

#include <cstdio>

#include "engine/engine.h"

namespace game {
bool intermission = false;
int maptime = 0, maprealtime = 0, maplimit = -1;
int lasthit = 0, lastspawnattempt = 0;
int lastregen = 0;
int killtime = 0;
int points = 0;
int points_total = 0;
const int showtime = 1300;
char point_text[8]{"\f20000"};
float scale;

gameent* player1 = nullptr;  // our client
vector<gameent*> players;  // other clients

int following = -1;

VARFP(specmode, 0, 0, 2, {
	if (!specmode)
		stopfollowing();
	else if (following < 0)
		nextfollow();
});

auto followingplayer() -> gameent* {
	if (player1->state != CS_SPECTATOR || following < 0)
		return nullptr;
	gameent* target = getclient(following);
	if (target && target->state != CS_SPECTATOR)
		return target;
	return nullptr;
}

ICOMMAND(getfollow, "", (), {
	gameent* f = followingplayer();
	intret(f ? f->clientnum : -1);
});

void stopfollowing() {
	if (following < 0)
		return;
	following = -1;
}

void follow(char* arg) {
	int cn = -1;
	if (arg[0]) {
		if (player1->state != CS_SPECTATOR)
			return;
		cn = parseplayer(arg);
		if (cn == player1->clientnum)
			cn = -1;
	}
	if (cn < 0 && (following < 0 || specmode))
		return;
	following = cn;
}
COMMAND(follow, "s");

void nextfollow(int dir) {
	if (player1->state != CS_SPECTATOR)
		return;
	int cur = following >= 0 ? following : (dir < 0 ? clients.length() - 1 : 0);
	loopv(clients) {
		cur = (cur + dir + clients.length()) % clients.length();
		if (clients[cur] && clients[cur]->state != CS_SPECTATOR) {
			following = cur;
			return;
		}
	}
	stopfollowing();
}
ICOMMAND(nextfollow, "i", (int* dir), nextfollow(*dir < 0 ? -1 : 1));

void checkfollow() {
	if (player1->state != CS_SPECTATOR) {
		if (following >= 0)
			stopfollowing();
	} else {
		if (following >= 0) {
			gameent* d = clients.inrange(following) ? clients[following] : NULL;
			if (!d || d->state == CS_SPECTATOR)
				stopfollowing();
		}
		if (following < 0 && specmode)
			nextfollow();
	}
}

auto getclientmap() -> const char* {
	return clientmap;
}

void resetgamestate() {
	clearprojectiles();
	clearbouncers();
}

auto spawnstate(gameent* d) -> gameent*	 // reset player state not persistent accross spawns
{
	d->respawn();
	d->spawnstate(gamemode);
	return d;
}

void respawnself() {
	if (ispaused())
		return;
	if (m_mp(gamemode)) {
		int seq = (player1->lifesequence << 16) | ((lastmillis / 1000) & 0xFFFF);
		if (player1->respawned != seq) {
			addmsg(N_TRYSPAWN, "rc", player1);
			player1->respawned = seq;
		}
	} else {
		spawnplayer(player1);
		showscores(false);
		lasthit = 0;
		player1->lasthit = 0;
		lastregen = 0;
		if (cmode)
			cmode->respawned(player1);
	}
}

auto pointatplayer() -> gameent* {
	loopv(players) if (players[i] != player1 && intersect(players[i], player1->o, worldpos)) return players[i];
	return nullptr;
}

auto hudplayer() -> gameent* {
	if (thirdperson || specmode > 1)
		return player1;
	gameent* target = followingplayer();
	return target ? target : player1;
}

void setupcamera() {
	gameent* target = followingplayer();
	if (target) {
		player1->yaw = target->yaw;
		player1->pitch = target->state == CS_DEAD ? 0 : target->pitch;
		player1->o = target->o;
		player1->resetinterp();
	}
}

auto detachcamera() -> bool {
	gameent* d = followingplayer();
	if (d)
		return specmode > 1 || d->state == CS_DEAD;
	return player1->state == CS_DEAD;
}

auto collidecamera() -> bool {
	switch (player1->state) {
	case CS_EDITING:
		return false;
	case CS_SPECTATOR:
		return followingplayer() != nullptr;
	}
	return true;
}

VARP(smoothmove, 0, 75, 100);
VARP(smoothdist, 0, 32, 64);

void predictplayer(gameent* d, bool move) {
	d->o = d->newpos;
	d->yaw = d->newyaw;
	d->pitch = d->newpitch;
	d->roll = d->newroll;
	if (move) {
		moveplayer(d, 1, false);
		d->newpos = d->o;
	}
	float k = 1.0f - float(lastmillis - d->smoothmillis) / smoothmove;
	if (k > 0) {
		d->o.add(vec(d->deltapos).mul(k));
		d->yaw += d->deltayaw * k;
		if (d->yaw < 0)
			d->yaw += 360;
		else if (d->yaw >= 360)
			d->yaw -= 360;
		d->pitch += d->deltapitch * k;
		d->roll += d->deltaroll * k;
	}
}

void otherplayers(int curtime) {
	loopv(players) {
		gameent* d = players[i];
		if (d == player1 || d->ai)
			continue;

		if (d->state == CS_DEAD && d->ragdoll)
			moveragdoll(d);
		else if (!intermission) {
			if (lastmillis - d->lastaction >= d->gunwait)
				d->gunwait = 0;
		}

		const int lagtime = totalmillis - d->lastupdate;
		if (!lagtime || intermission)
			continue;
		else if (lagtime > 1000 && d->state == CS_ALIVE) {
			d->state = CS_LAGGED;
			continue;
		}
		if (d->state == CS_ALIVE || d->state == CS_EDITING) {
			crouchplayer(d, 10, false);
			if (smoothmove && d->smoothmillis > 0)
				predictplayer(d, true);
			else
				moveplayer(d, 1, false);
		} else if (d->state == CS_DEAD && !d->ragdoll && lastmillis - d->lastpain < 2000)
			moveplayer(d, 1, true);
	}
}

void updateworld()	// main game update loop
{
	if (!maptime) {
		maptime = lastmillis;
		maprealtime = totalmillis;
		return;
	}
	if (!curtime) {
		gets2c();
		if (player1->clientnum >= 0)
			c2sinfo();
		return;
	}

	physicsframe();
	ai::navigate();
	updateweapons(curtime);
	otherplayers(curtime);
	ai::update();
	moveragdolls();
	gets2c();
	if (connected) {
		if (player1->state == CS_DEAD) {
			damagealpha(0);
			if (player1->ragdoll)
				moveragdoll(player1);
			else if (lastmillis - player1->lastpain < 2000) {
				player1->move = player1->strafe = 0;
				moveplayer(player1, 10, true);
			}
		} else if (!intermission) {
			// update health
			damagealpha(
				1.0f - (
					static_cast<float>(player1->health) / 
					static_cast<float>(player1->maxhealth)
			));
			
			if (player1->ragdoll)
				cleanragdoll(player1);
			crouchplayer(player1, 10, true);
			moveplayer(player1, 10, true);
			swayhudgun(curtime);
			entities::checkitems(player1);
			if (cmode)
				cmode->checkitems(player1);
		}
	}
	if (player1->clientnum >= 0)
		c2sinfo();	// do this last, to reduce the effective frame lag
}

void spawnplayer(gameent* d)  // place at random spawn
{
	if (cmode)
		cmode->pickspawn(d);
	else
		findplayerspawn(d, -1, m_teammode ? d->team : 0);
	spawnstate(d);
	if (d == player1) {
		if (editmode)
			d->state = CS_EDITING;
		else if (d->state != CS_SPECTATOR)
			d->state = CS_ALIVE;
	} else
		d->state = CS_ALIVE;
	checkfollow();
}

VARP(spawnwait, 0, 0, 1000);

void respawn() {
	if (player1->state == CS_DEAD) {
		player1->attacking = ACT_IDLE;
		int wait = cmode ? cmode->respawnwait(player1) : 0;
		if (wait > 0) {
			lastspawnattempt = lastmillis;
			// conoutf(CON_GAMEINFO, "\f2you must wait %d second%s before respawn!", wait, wait!=1 ? "s" : "");
			return;
		}
		if (lastmillis < player1->lastpain + spawnwait)
			return;
		respawnself();
	}
}

// inputs

void doaction(int act) {
	if (!connected || intermission)
		return;
	if ((player1->attacking = act))
		respawn();
}

ICOMMAND(shoot, "D", (int* down), doaction(*down ? ACT_SHOOT : ACT_IDLE));
ICOMMAND(melee, "D", (int* down), doaction(*down ? ACT_MELEE : ACT_IDLE));

auto canjump() -> bool {
	if (!connected || intermission)
		return false;
	respawn();
	return player1->state != CS_DEAD;
}

auto cancrouch() -> bool {
	if (!connected || intermission)
		return false;
	return player1->state != CS_DEAD;
}

auto allowmove(physent* d) -> bool {
	if (d->type != ENT_PLAYER)
		return true;
	return !((gameent*)d)->lasttaunt || lastmillis - ((gameent*)d)->lasttaunt >= 1000;
}

void taunt() {
	if (player1->state != CS_ALIVE || player1->physstate < PHYS_SLOPE)
		return;
	if (lastmillis - player1->lasttaunt < 1000)
		return;
	player1->lasttaunt = lastmillis;
	addmsg(N_TAUNT, "rc", player1);
}
COMMAND(taunt, "");

VARP(hitsound, 0, 1, 1);

void damaged(int damage, gameent* d, gameent* actor, bool local) {
	if ((d->state != CS_ALIVE && d->state != CS_LAGGED && d->state != CS_SPAWNING) || intermission)
		return;

	if (local)
		damage = d->dodamage(damage);
	else if (actor == player1)
		return;

	gameent* h = hudplayer();
	if (h != player1 && actor == h && d != actor) {
		if (hitsound && lasthit != lastmillis) {
			playsound(S_HIT);
			player1->lasthit = lastmillis;
			lasthit = lastmillis;
		}
	}
	if (d == h) {
		damagecompass(damage, actor->o);
	}
	damageeffect(damage, d, d != h);

	ai::damaged(d, actor);

	if (d->health <= 0) {
		if (local)
			killed(d, actor);
	} else if (d == h)
		playsound(S_PAIN2);
	else
		playsound(S_PAIN1, &d->o);
}

VARP(deathscore, 0, 1, 1);

static auto randomdeathsound() -> int {
	static int s_lastdeathsound = 0;
	s_lastdeathsound = (s_lastdeathsound + 1) % 5;
	return S_DIE1 + s_lastdeathsound;
}

void deathstate(gameent* d, bool restore) {
	d->state = CS_DEAD;
	d->lastpain = lastmillis;
	if (!restore) {
		d->deaths++;
	}
	if (d == player1) {
		if (deathscore)
			showscores(true);
		disablezoom();
		d->attacking = ACT_IDLE;
		// d->pitch = 0;
		d->roll = 0;
		playsound(randomdeathsound());
	} else {
		d->move = d->strafe = 0;
		d->resetinterp();
		d->smoothmillis = 0;
		playsound(randomdeathsound(), &d->o);
	}
}

VARP(teamcolorfrags, 0, 1, 1);

void killed(gameent* d, gameent* actor) {
	if (d->state == CS_EDITING) {
		d->editstate = CS_DEAD;
		d->deaths++;
		if (d != player1)
			d->resetinterp();
		return;
	} else if ((d->state != CS_ALIVE && d->state != CS_LAGGED && d->state != CS_SPAWNING) || intermission) {
		return;
	}

	gameent* h = followingplayer();
	if (!h) {
		h = player1;
	}

	const char* const dname = d->name;
	const char* const aname = actor->name;

	if (d == actor)
		conoutf(CON_GAMEINFO, "\f7%s suicided...", dname);
	else {
		char aColor[]{"\f9"};
		char dColor[]{"\f9"};

		if(d == player1){
			strncpy(dColor, "\f7", 3);
		} else if (!isteam(player1->team, d->team)) {
			strncpy(dColor, "\f3", 3);
		}

		if(actor == player1){
			strncpy(aColor, "\f7", 3);
		} else if (!isteam(player1->team, actor->team)) {
			strncpy(aColor, "\f3", 3);
		}

		const char* weapon = weapon_names[actor->lastattack];
		conoutf(CON_GAMEINFO, "%s%s \f7[%s] %s%s", aColor, aname, weapon, dColor, dname);
	}

	// draw the points you got
	if (actor == player1 && d != player1 && !isteam(player1->team, d->team)) {
		killtime = lastmillis;
		points += 50;
		points_total += 50;
		sprintf(point_text, "\f2+%d", points);
		scale = 2;
	}

	deathstate(d);
	ai::killed(d, actor);
}

void timeupdate(int secs) {
	if (secs > 0) {
		// TODO: disabled for testing
		// maplimit = lastmillis + secs*1000;
	} else {
		intermission = true;
		player1->attacking = ACT_IDLE;
		if (cmode)
			cmode->gameover();
		conoutf(CON_GAMEINFO, "\f2intermission:");
		conoutf(CON_GAMEINFO, "\f2game has ended!");
		if (m_ctf)
			conoutf(CON_GAMEINFO,
					"\f2player frags: %d, flags: %d, deaths: %d",
					player1->frags,
					player1->flags,
					player1->deaths);
		else
			conoutf(CON_GAMEINFO, "\f2player frags: %d, deaths: %d", player1->frags, player1->deaths);
		int accuracy = (player1->totaldamage * 100) / max(player1->totalshots, 1);
		conoutf(CON_GAMEINFO,
				"\f2player total damage dealt: %d, damage wasted: %d, accuracy(%%): %d",
				player1->totaldamage,
				player1->totalshots - player1->totaldamage,
				accuracy);

		showscores(true);
		disablezoom();

		execident("intermission");
	}
}

ICOMMAND(getfrags, "", (), intret(player1->frags));
ICOMMAND(getflags, "", (), intret(player1->flags));
ICOMMAND(getdeaths, "", (), intret(player1->deaths));
ICOMMAND(getaccuracy, "", (), intret((player1->totaldamage * 100) / max(player1->totalshots, 1)));
ICOMMAND(gettotaldamage, "", (), intret(player1->totaldamage));
ICOMMAND(gettotalshots, "", (), intret(player1->totalshots));

vector<gameent*> clients;

auto newclient(int cn) -> gameent*	// ensure valid entity
{
	if (cn < 0 || cn > max(0xFF, MAXCLIENTS + MAXBOTS)) {
		neterr("clientnum", false);
		return nullptr;
	}

	if (cn == player1->clientnum)
		return player1;

	while (cn >= clients.length())
		clients.add(NULL);
	if (!clients[cn]) {
		auto* d = new gameent;
		d->clientnum = cn;
		clients[cn] = d;
		players.add(d);
	}
	return clients[cn];
}

auto getclient(int cn) -> gameent*	// ensure valid entity
{
	if (cn == player1->clientnum)
		return player1;
	return clients.inrange(cn) ? clients[cn] : NULL;
}

void clientdisconnected(int cn, bool notify) {
	if (!clients.inrange(cn))
		return;
	unignore(cn);
	gameent* d = clients[cn];
	if (d) {
		if (notify && d->name[0])
			conoutf("\f4leave:\f7 %s", colorname(d));
		removeweapons(d);
		removetrackedparticles(d);
		removetrackeddynlights(d);
		if (cmode)
			cmode->removeplayer(d);
		removegroupedplayer(d);
		players.removeobj(d);
		DELETEP(clients[cn]);
		cleardynentcache();
	}
	if (following == cn) {
		if (specmode)
			nextfollow();
		else
			stopfollowing();
	}
}

void clearclients(bool notify) {
	loopv(clients) if (clients[i]) clientdisconnected(i, notify);
}

void initclient() {
	player1 = spawnstate(new gameent);
	filtertext(player1->name, "unnamed", false, false, MAXNAMELEN);
	players.add(player1);
}

VARP(showmodeinfo, 0, 1, 1);

void startgame() {
	clearprojectiles();
	clearbouncers();
	clearragdolls();

	clearteaminfo();

	// reset perma-state
	loopv(players) players[i]->startgame();

	setclientmode();

	intermission = false;
	maptime = maprealtime = 0;
	maplimit = -1;

	if (cmode) {
		cmode->preload();
		cmode->setup();
	}

	conoutf(CON_GAMEINFO, "\f2game mode is %s", server::modeprettyname(gamemode));

	const char* info = m_valid(gamemode) ? gamemodes[gamemode - STARTGAMEMODE].info : nullptr;
	if (showmodeinfo && info)
		conoutf(CON_GAMEINFO, "\f0%s", info);

	syncplayer();

	showscores(false);
	disablezoom();
	lasthit = 0;
	player1->lasthit = 0;

	execident("mapstart");
}

void startmap(const char* name)	 // called just after a map load
{
	ai::savewaypoints();
	ai::clearwaypoints(true);

	if (!m_mp(gamemode))
		spawnplayer(player1);
	else
		findplayerspawn(player1, -1, m_teammode ? player1->team : 0);
	entities::resetspawns();
	copystring(clientmap, name ? name : "");

	sendmapinfo();
}

auto getmapinfo() -> const char* {
	return showmodeinfo && m_valid(gamemode) ? gamemodes[gamemode - STARTGAMEMODE].info : nullptr;
}

auto getscreenshotinfo() -> const char* {
	return server::modename(gamemode, nullptr);
}

void physicstrigger(physent* d, bool local, int floorlevel, int waterlevel, int material) {
	if (waterlevel > 0) {
		if (material != MAT_LAVA)
			playsound(S_SPLASHOUT, d == player1 ? nullptr : &d->o);
	} else if (waterlevel < 0)
		playsound(material == MAT_LAVA ? S_BURN : S_SPLASHIN, d == player1 ? nullptr : &d->o);
	if (floorlevel > 0) {
		if (d == player1 || d->type != ENT_PLAYER || ((gameent*)d)->ai)
			msgsound(S_JUMP, d);
	} else if (floorlevel < 0) {
		if (d == player1 || d->type != ENT_PLAYER || ((gameent*)d)->ai)
			msgsound(S_LAND, d);
	}
}

void dynentcollide(physent* d, physent* o, const vec& dir) {
}

void msgsound(int n, physent* d) {
	if (!d || d == player1) {
		addmsg(N_SOUND, "ci", d, n);
		playsound(n);
	} else {
		if (d->type == ENT_PLAYER && ((gameent*)d)->ai)
			addmsg(N_SOUND, "ci", d, n);
		playsound(n, &d->o);
	}
}

auto numdynents() -> int {
	return players.length();
}

auto iterdynents(int i) -> dynent* {
	if (i < players.length())
		return players[i];
	return nullptr;
}

auto duplicatename(gameent* d, const char* name = nullptr, const char* alt = nullptr) -> bool {
	if (!name)
		name = d->name;
	if (alt && d != player1 && !strcmp(name, alt))
		return true;
	loopv(players) if (d != players[i] && !strcmp(name, players[i]->name)) return true;
	return false;
}

auto colorname(gameent* d, const char* name, const char* alt, const char* color) -> const char* {
	if (!name)
		name = alt && d == player1 ? alt : d->name;
	return name;
}

VARP(teamcolortext, 0, 1, 1);

auto teamcolorname(gameent* d, const char* alt) -> const char* {
	if (!teamcolortext || !m_teammode || !validteam(d->team))
		return colorname(d, nullptr, alt);
	return colorname(d, nullptr, alt, teamtextcode[d->team]);
}

auto teamcolor(const char* prefix, const char* suffix, int team, const char* alt) -> const char* {
	if (!teamcolortext || !m_teammode || !validteam(team))
		return alt;
	return tempformatstring("\fs%s%s%s%s\fr", teamtextcode[team], prefix, teamnames[team], suffix);
}

void suicide(physent* d) {
	if (d == player1 || (d->type == ENT_PLAYER && ((gameent*)d)->ai)) {
		if (d->state != CS_ALIVE)
			return;
		auto* pl = (gameent*)d;
		if (!m_mp(gamemode))
			killed(pl, pl);
		else {
			int seq = (pl->lifesequence << 16) | ((lastmillis / 1000) & 0xFFFF);
			if (pl->suicided != seq) {
				addmsg(N_SUICIDE, "rc", pl);
				pl->suicided = seq;
			}
		}
	}
}
ICOMMAND(suicide, "", (), suicide(player1));

auto needminimap() -> bool {
	return m_ctf;
}

void drawicon(int icon, float x, float y, float sz) {
	settexture("media/interface/hud/items.png");
	float tsz = 0.25f, tx = tsz * (icon % 4), ty = tsz * (icon / 4);
	gle::defvertex(2);
	gle::deftexcoord0();
	gle::begin(GL_TRIANGLE_STRIP);
	gle::attribf(x, y);
	gle::attribf(tx, ty);
	gle::attribf(x + sz, y);
	gle::attribf(tx + tsz, ty);
	gle::attribf(x, y + sz);
	gle::attribf(tx, ty + tsz);
	gle::attribf(x + sz, y + sz);
	gle::attribf(tx + tsz, ty + tsz);
	gle::end();
}

auto abovegameplayhud(int w, int h) -> float {
	switch (hudplayer()->state) {
	case CS_EDITING:
	case CS_SPECTATOR:
		return 1;
	default:
		return 1650.0f / 1800.0f;
	}
}

void drawhudicons(gameent* d) {
	resethudshader();
	if (d->state != CS_DEAD) {
		// drawicon(d->gunselect, HICON_X + 2*HICON_STEP, HICON_Y);
	}

	//spread = (player1->lastaction - lastmillis)*100;
	//spread = player1->vel.magnitude();

	if(d->state == CS_ALIVE) {
		//spread = (d->lastaction - lastmillis)*100;
		/*	
        loopv(flags) if(flags[i].owner == d)
        {
            float x = 1800*w/h*0.5f-HICON_SIZE/2, y = 1800*0.95f-HICON_SIZE/2;
            drawicon(flags[i].team==1 ? HICON_BLUE_FLAG : HICON_RED_FLAG, x, y);
            break;
        }
		*/
    }

	// minimap
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    int s = 1800/5;
	int x = s/10;
	int y = s/10;
    
    gle::colorf(1, 1, 1);
    float margin = 0.04f, roffset = s*margin, rsize = s + 2*roffset;
    setradartex();
    pushhudmatrix();
    hudmatrix.translate(x - roffset + 0.5f*rsize, y - roffset + 0.5f*rsize, 0);
	//gle::colorf(0.3, 0.5, 0.4, 0.8f);
	//bindminimap();
	//drawMinimapSquare(d, x, y, s);
	gle::colorf(1, 1, 1, 1);
	drawradar(x - roffset, y - roffset, rsize);
	
    hudmatrix.rotate_around_z((camera1->yaw + 180)*-RAD);
    flushhudmatrix();
    pophudmatrix();
    
	drawteammates(d, x, y, s);
	drawplayerblip(d, x, y, s, 1.5f);

	settexture("assets/res/img/ui/overlay.png", 3);
	drawradar(x - roffset, y - roffset, rsize);
    
	/*
	loopv(flags) {
        flag &f = flags[i];
        if(!validteam(f.team)) continue;
        if(f.owner) {
            if(lastmillis%1000 >= 500) continue;
        } else if(f.droptime && (f.droploc.x < 0 || lastmillis%300 >= 150)) continue;
            drawblip(d, x, y, s, i, true);
    }*/
    
	/*
    if(d->state == CS_DEAD) {
        int wait = respawnwait(d);
        if(wait>=0) {
            pushhudscale(2);
            bool flash = wait>0 && d==player1 && lastspawnattempt>=d->lastpain && lastmillis < lastspawnattempt+100;
            draw_textf("%s%d", (x+s/2)/2-(wait>=10 ? 28 : 16), (y+s/2)/2-32, flash ? "\f3" : "", wait);
            pophudmatrix();
            resethudshader();
        }
    }
	*/

	if (d->state != CS_DEAD) {
		resethudshader();

		//Team color
		gle::colorf(0.2, 0.6, 1, 0.7);

		// background image left
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		static Texture* bg_tex_left = nullptr;
		if (!bg_tex_left) {
			bg_tex_left = textureload("assets/res/img/ui/left.png", 3);
		}
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindTexture(GL_TEXTURE_2D, bg_tex_left->id);
		hudquad(HICON_X + HICON_SPACE + 80, HICON_Y, 300, 120);

		// background image right
		static Texture* bg_tex_right = nullptr;
		if (!bg_tex_right) {
			bg_tex_right = textureload("assets/res/img/ui/right.png", 3);
		}
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindTexture(GL_TEXTURE_2D, bg_tex_right->id);
		hudquad((hudw*1.65)-300, HICON_Y, 330, 150);

		// Score and Team Icon
		drawicon(d->team, HICON_X, HICON_Y, 120);
		draw_textf("Score:", HICON_X + HICON_SPACE + 120, HICON_Y);
		draw_textf("%d", HICON_X + HICON_SPACE + 120, HICON_Y + HICON_SIZE / 2, points_total);

		// ammo text
	}
}

void gameplayhud(int w, int h) {
	pushhudscale(h / 1800.0f);

	if (player1->state == CS_SPECTATOR) {
		float pw, ph, tw, th, fw, fh;
		text_boundsf("  ", pw, ph);
		text_boundsf("SPECTATOR", tw, th);
		th = max(th, ph);
		gameent* f = followingplayer();
		text_boundsf(f ? colorname(f) : " ", fw, fh);
		fh = max(fh, ph);
		draw_text("SPECTATOR", w * 1800 / h - tw - pw, 1650 - th - fh);
		if (f) {
			int color = f->state != CS_DEAD ? 0xFFFFFF : 0x606060;
			if (f->privilege) {
				color = f->privilege >= PRIV_ADMIN ? 0xFF8000 : 0x40FF80;
				if (f->state == CS_DEAD)
					color = (color >> 1) & 0x7F7F7F;
			}
			draw_text(colorname(f),
					  w * 1800 / h - fw - pw,
					  1650 - fh,
					  (color >> 16) & 0xFF,
					  (color >> 8) & 0xFF,
					  color & 0xFF);
		}
		resethudshader();
	}

	gameent* d = hudplayer();
	if (d->state != CS_EDITING) {
		if (d->state != CS_SPECTATOR) {
			int alpha = 255 - max(0, lastmillis - killtime - showtime);

			if (alpha > 1) {
				if (scale > 1.5) {
					int delta_time = lastmillis - killtime;
					scale -= delta_time / 1200.0f;
				}
				pushhudscale(scale);

				float fw, fh;
				text_boundsf(point_text, fw, fh);
				draw_text(point_text,
						  ((w / scale * (1800.f / h)) - fw) / 2,
						  ((h / scale * (1650.f / w)) - fh) / 2 + 200 / scale,
						  0xFF,
						  0xFF,
						  0XFF,
						  alpha);

				// reset the hud scale
				pophudmatrix();
			} else {
				killtime = 0;
				points = 0;
			}
			
			drawhudicons(d);
		}
		if (cmode)
			cmode->drawhud(d, w, h);
	}

	pophudmatrix();
}

auto clipconsole(float w, float h) -> float {
	if (cmode)
		return cmode->clipconsole(w, h);
	return 0;
}

VARP(teamcrosshair, 0, 1, 1);
VARP(hitcrosshair, 0, 425, 1000);

auto defaultcrosshair(int index) -> const char* {
	return "assets/res/img/ui/dot.png";
}

auto selectcrosshair(vec& col) -> int {
	gameent* d = hudplayer();
	if (d->state == CS_SPECTATOR || d->state == CS_DEAD || UI::uivisible("scoreboard"))
		return -1;

	if (d->state != CS_ALIVE)
		return 0;

	int crosshair = 0;
	if (lasthit && lastmillis - lasthit < hitcrosshair)
		crosshair = 2;
	else if (teamcrosshair && m_teammode) {
		dynent* o = intersectclosest(d->o, worldpos, d);
		if (o && o->type == ENT_PLAYER && validteam(d->team) && ((gameent*)o)->team == d->team) {
			crosshair = 1;

			col = vec::hexcolor(teamtextcolor[d->team]);
		}
	}

#if 0
        if(crosshair!=1 && !editmode)
        {
            if(d->health<=25) { r = 1.0f; g = b = 0; }
            else if(d->health<=50) { r = 1.0f; g = 0.5f; b = 0; }
        }
#endif
	if (d->gunwait)
		col.mul(0.25f);
	return crosshair;
}

auto mastermodecolor(int n, const char* unknown) -> const char* {
	return (n >= MM_START && size_t(n - MM_START) < sizeof(mastermodecolors) / sizeof(mastermodecolors[0]))
		? mastermodecolors[n - MM_START]
		: unknown;
}

auto mastermodeicon(int n, const char* unknown) -> const char* {
	return (n >= MM_START && size_t(n - MM_START) < sizeof(mastermodeicons) / sizeof(mastermodeicons[0]))
		? mastermodeicons[n - MM_START]
		: unknown;
}

ICOMMAND(servinfomode, "i", (int* i), GETSERVINFOATTR(*i, 0, mode, intret(mode)));
ICOMMAND(servinfomodename, "i", (int* i), GETSERVINFOATTR(*i, 0, mode, {
			 const char* name = server::modeprettyname(mode, nullptr);
			 if (name)
				 result(name);
		 }));
ICOMMAND(servinfomastermode, "i", (int* i), GETSERVINFOATTR(*i, 2, mm, intret(mm)));
ICOMMAND(servinfomastermodename, "i", (int* i), GETSERVINFOATTR(*i, 2, mm, {
			 const char* name = server::mastermodename(mm, nullptr);
			 if (name)
				 stringret(newconcatstring(mastermodecolor(mm, ""), name));
		 }));
ICOMMAND(servinfotime, "ii", (int* i, int* raw), GETSERVINFOATTR(*i, 1, secs, {
			 secs = clamp(secs, 0, 59 * 60 + 59);
			 if (*raw)
				 intret(secs);
			 else {
				 int mins = secs / 60;
				 secs %= 60;
				 result(tempformatstring("%d:%02d", mins, secs));
			 }
		 }));
ICOMMAND(servinfoicon, "i", (int* i), GETSERVINFO(*i, si, {
			 int mm = si->attr.inrange(2) ? si->attr[2] : MM_INVALID;
			 result(si->maxplayers > 0 && si->numplayers >= si->maxplayers ? "serverfull"
																		   : mastermodeicon(mm, "serverunk"));
		 }));

// any data written into this vector will get saved with the map data. Must take care to do own versioning, and
// endianess if applicable. Will not get called when loading maps from other games, so provide defaults.
void writegamedata(vector<char>& extras) {
}
void readgamedata(vector<char>& extras) {
}

auto gameconfig() -> const char* {
	return "config/game.cfg";
}
auto savedconfig() -> const char* {
	return "config/saved.cfg";
}
auto restoreconfig() -> const char* {
	return "config/restore.cfg";
}
auto defaultconfig() -> const char* {
	return "config/default.cfg";
}
auto autoexec() -> const char* {
	return "config/autoexec.cfg";
}
auto savedservers() -> const char* {
	return "config/servers.cfg";
}

void loadconfigs() {
	execfile("config/auth.cfg", false);
}

auto clientoption(const char* arg) -> bool {
	return false;
}
}  // namespace game
