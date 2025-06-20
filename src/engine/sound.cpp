// sound.cpp: basic positional sound using sdl_mixer

#include "engine/engine.h"

#ifdef __APPLE__
#include "SDL2_mixer/SDL_mixer.h"
#else
#include "SDL_mixer.h"
#endif

bool nosound = true;

struct soundsample {
	char* name{NULL};
	Mix_Chunk* chunk{NULL};

	soundsample()  {
	}
	~soundsample() {
		DELETEA(name);
	}

	void cleanup() {
		if (chunk) {
			Mix_FreeChunk(chunk);
			chunk = nullptr;
		}
	}
	auto load(const char* dir, bool msg = false) -> bool;
};

struct soundslot {
	soundsample* sample;
	int volume;
};

struct soundconfig {
	int slots, numslots;
	int maxuses;

	auto hasslot(const soundslot* p, const vector<soundslot>& v) const -> bool {
		return p >= v.getbuf() + slots && p < v.getbuf() + slots + numslots && slots + numslots < v.length();
	}

	[[nodiscard]] auto chooseslot() const -> int {
		return numslots > 1 ? slots + rnd(numslots) : slots;
	}
};

struct soundchannel {
	int id;
	bool inuse;
	vec loc;
	soundslot* slot;
	extentity* ent;
	int radius, volume, pan, flags;
	bool dirty;

	soundchannel(int id) : id(id) {
		reset();
	}

	[[nodiscard]] auto hasloc() const -> bool {
		return loc.x >= -1e15f;
	}
	void clearloc() {
		loc = vec(-1e16f, -1e16f, -1e16f);
	}

	void reset() {
		inuse = false;
		clearloc();
		slot = nullptr;
		ent = nullptr;
		radius = 0;
		volume = -1;
		pan = -1;
		flags = 0;
		dirty = false;
	}
};
vector<soundchannel> channels;
int maxchannels = 0;

auto newchannel(int n,
						 soundslot* slot,
						 const vec* loc = nullptr,
						 extentity* ent = nullptr,
						 int flags = 0,
						 int radius = 0) -> soundchannel& {
	if (ent) {
		loc = &ent->o;
		ent->flags |= EF_SOUND;
	}
	while (!channels.inrange(n))
		channels.add(channels.length());
	soundchannel& chan = channels[n];
	chan.reset();
	chan.inuse = true;
	if (loc)
		chan.loc = *loc;
	chan.slot = slot;
	chan.ent = ent;
	chan.flags = 0;
	chan.radius = radius;
	return chan;
}

void freechannel(int n) {
	if (!channels.inrange(n) || !channels[n].inuse)
		return;
	soundchannel& chan = channels[n];
	chan.inuse = false;
	if (chan.ent)
		chan.ent->flags &= ~EF_SOUND;
}

void syncchannel(soundchannel& chan) {
	if (!chan.dirty)
		return;
	if (!Mix_FadingChannel(chan.id))
		Mix_Volume(chan.id, chan.volume);
	Mix_SetPanning(chan.id, 255 - chan.pan, chan.pan);
	chan.dirty = false;
}

void stopchannels() {
	loopv(channels) {
		soundchannel& chan = channels[i];
		if (!chan.inuse)
			continue;
		Mix_HaltChannel(i);
		freechannel(i);
	}
}

void setmusicvol(int musicvol);
VARFP(
	soundvol,
	0,
	255,
	255,
	if (!soundvol) {
		stopchannels();
		setmusicvol(0);
	});
VARFP(musicvol, 0, 60, 255, setmusicvol(soundvol ? musicvol : 0));

char *musicfile = nullptr, *musicdonecmd = nullptr;

Mix_Music* music = nullptr;
SDL_RWops* musicrw = nullptr;
stream* musicstream = nullptr;

void setmusicvol(int musicvol) {
	if (nosound)
		return;
	if (music)
		Mix_VolumeMusic((musicvol * MIX_MAX_VOLUME) / 255);
}

void stopmusic() {
	if (nosound)
		return;
	DELETEA(musicfile);
	DELETEA(musicdonecmd);
	if (music) {
		Mix_HaltMusic();
		Mix_FreeMusic(music);
		music = nullptr;
	}
	if (musicrw) {
		SDL_FreeRW(musicrw);
		musicrw = nullptr;
	}
	DELETEP(musicstream);
}

VARF(sound, 0, 1, 1, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
VARF(soundchans, 1, 32, 128, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
VARF(soundfreq, 0, 44100, 44100, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
VARF(soundbufferlen, 128, 1024, 4096, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));

void initsound() {
	if (!sound || Mix_OpenAudio(soundfreq, MIX_DEFAULT_FORMAT, 2, soundbufferlen) < 0) {
		nosound = true;
		if (sound)
			conoutf(CON_ERROR, "sound init failed (SDL_mixer): %s", Mix_GetError());
		return;
	}
	Mix_AllocateChannels(soundchans);
	maxchannels = soundchans;
	nosound = false;
}

void musicdone() {
	if (music) {
		Mix_HaltMusic();
		Mix_FreeMusic(music);
		music = nullptr;
	}
	if (musicrw) {
		SDL_FreeRW(musicrw);
		musicrw = nullptr;
	}
	DELETEP(musicstream);
	DELETEA(musicfile);
	if (!musicdonecmd)
		return;
	char* cmd = musicdonecmd;
	musicdonecmd = nullptr;
	execute(cmd);
	delete[] cmd;
}

auto loadmusic(const char* name) -> Mix_Music* {
	if (!musicstream)
		musicstream = openzipfile(name, "rb");
	if (musicstream) {
		if (!musicrw)
			musicrw = musicstream->rwops();
		if (!musicrw)
			DELETEP(musicstream);
	}
	if (musicrw)
		music = Mix_LoadMUSType_RW(musicrw, MUS_NONE, 0);
	else
		music = Mix_LoadMUS(findfile(name, "rb"));
	if (!music) {
		if (musicrw) {
			SDL_FreeRW(musicrw);
			musicrw = nullptr;
		}
		DELETEP(musicstream);
	}
	return music;
}

void startmusic(char* name, char* cmd) {
	if (nosound)
		return;
	stopmusic();
	if (soundvol && musicvol && *name) {
		defformatstring(file, "media/%s", name);
		path(file);
		if (loadmusic(file)) {
			DELETEA(musicfile);
			DELETEA(musicdonecmd);
			musicfile = newstring(file);
			if (cmd[0])
				musicdonecmd = newstring(cmd);
			Mix_PlayMusic(music, cmd[0] ? 0 : -1);
			Mix_VolumeMusic((musicvol * MIX_MAX_VOLUME) / 255);
			intret(1);
		} else {
			conoutf(CON_ERROR, "could not play music: %s", file);
			intret(0);
		}
	}
}

COMMANDN(music, startmusic, "ss");

static auto loadwav(const char* name) -> Mix_Chunk* {
	Mix_Chunk* c = nullptr;
	stream* z = openzipfile(name, "rb");
	if (z) {
		SDL_RWops* rw = z->rwops();
		if (rw) {
			c = Mix_LoadWAV_RW(rw, 0);
			SDL_FreeRW(rw);
		}
		delete z;
	}
	if (!c)
		c = Mix_LoadWAV(findfile(name, "rb"));
	return c;
}

auto soundsample::load(const char* dir, bool msg) -> bool {
	if (chunk)
		return true;
	if (!name[0])
		return false;

	static const char* const exts[] = {"", ".wav", ".ogg"};
	string filename;
	loopi(sizeof(exts) / sizeof(exts[0])) {
		formatstring(filename, "media/sound/%s%s%s", dir, name, exts[i]);
		if (msg && !i)
			renderprogress(0, filename);
		path(filename);
		chunk = loadwav(filename);
		if (chunk)
			return true;
	}

	conoutf(CON_ERROR, "failed to load sample: media/sound/%s%s", dir, name);
	return false;
}

static struct soundtype {
	hashnameset<soundsample> samples;
	vector<soundslot> slots;
	vector<soundconfig> configs;
	const char* dir;

	soundtype(const char* dir) : dir(dir) {
	}

	auto findsound(const char* name, int vol) -> int {
		loopv(configs) {
			soundconfig& s = configs[i];
			loopj(s.numslots) {
				soundslot& c = slots[s.slots + j];
				if (!strcmp(c.sample->name, name) && (!vol || c.volume == vol))
					return i;
			}
		}
		return -1;
	}

	auto addslot(const char* name, int vol) -> int {
		soundsample* s = samples.access(name);
		if (!s) {
			char* n = newstring(name);
			s = &samples[n];
			s->name = n;
			s->chunk = nullptr;
		}
		soundslot* oldslots = slots.getbuf();
		int oldlen = slots.length();
		soundslot& slot = slots.add();
		// soundslots.add() may relocate slot pointers
		if (slots.getbuf() != oldslots)
			loopv(channels) {
				soundchannel& chan = channels[i];
				if (chan.inuse && chan.slot >= oldslots && chan.slot < &oldslots[oldlen])
					chan.slot = &slots[chan.slot - oldslots];
			}
		slot.sample = s;
		slot.volume = vol ? vol : 100;
		return oldlen;
	}

	auto addsound(const char* name, int vol, int maxuses = 0) -> int {
		soundconfig& s = configs.add();
		s.slots = addslot(name, vol);
		s.numslots = 1;
		s.maxuses = maxuses;
		return configs.length() - 1;
	}

	void addalt(const char* name, int vol) {
		if (configs.empty())
			return;
		addslot(name, vol);
		configs.last().numslots++;
	}

	void clear() {
		slots.setsize(0);
		configs.setsize(0);
	}

	void reset() {
		loopv(channels) {
			soundchannel& chan = channels[i];
			if (chan.inuse && slots.inbuf(chan.slot)) {
				Mix_HaltChannel(i);
				freechannel(i);
			}
		}
		clear();
	}

	void cleanupsamples() {
		enumerate(samples, soundsample, s, s.cleanup());
	}

	void cleanup() {
		cleanupsamples();
		slots.setsize(0);
		configs.setsize(0);
		samples.clear();
	}

	void preloadsound(int n) {
		if (nosound || !configs.inrange(n))
			return;
		soundconfig& config = configs[n];
		loopk(config.numslots) slots[config.slots + k].sample->load(dir, true);
	}

	[[nodiscard]] auto playing(const soundchannel& chan, const soundconfig& config) const -> bool {
		return chan.inuse && config.hasslot(chan.slot, slots);
	}
} gamesounds("game/"), mapsounds("mapsound/");

void registersound(char* name, int* vol) {
	intret(gamesounds.addsound(name, *vol, 0));
}
COMMAND(registersound, "si");

void mapsound(char* name, int* vol, int* maxuses) {
	intret(mapsounds.addsound(name, *vol, *maxuses < 0 ? 0 : max(1, *maxuses)));
}
COMMAND(mapsound, "sii");

void altsound(char* name, int* vol) {
	gamesounds.addalt(name, *vol);
}
COMMAND(altsound, "si");

void altmapsound(char* name, int* vol) {
	mapsounds.addalt(name, *vol);
}
COMMAND(altmapsound, "si");

ICOMMAND(numsounds, "", (), intret(gamesounds.configs.length()));
ICOMMAND(nummapsounds, "", (), intret(mapsounds.configs.length()));

void soundreset() {
	gamesounds.reset();
}
COMMAND(soundreset, "");

void mapsoundreset() {
	mapsounds.reset();
}
COMMAND(mapsoundreset, "");

void resetchannels() {
	loopv(channels) if (channels[i].inuse) freechannel(i);
	channels.shrink(0);
}

void clear_sound() {
	closemumble();
	if (nosound)
		return;
	stopmusic();

	gamesounds.cleanup();
	mapsounds.cleanup();
	Mix_CloseAudio();
	resetchannels();
}

void stopmapsounds() {
	loopv(channels) if (channels[i].inuse && channels[i].ent) {
		Mix_HaltChannel(i);
		freechannel(i);
	}
}

void clearmapsounds() {
	stopmapsounds();
	mapsounds.clear();
}

void stopmapsound(extentity* e) {
	loopv(channels) {
		soundchannel& chan = channels[i];
		if (chan.inuse && chan.ent == e) {
			Mix_HaltChannel(i);
			freechannel(i);
		}
	}
}

void checkmapsounds() {
	const vector<extentity*>& ents = entities::getents();
	loopv(ents) {
		extentity& e = *ents[i];
		if (e.type != ET_SOUND)
			continue;
		if (camera1->o.dist(e.o) < e.attr2) {
			if (!(e.flags & EF_SOUND))
				playsound(e.attr1, nullptr, &e, SND_MAP, -1);
		} else if (e.flags & EF_SOUND)
			stopmapsound(&e);
	}
}

VAR(stereo, 0, 1, 1);

VARP(maxsoundradius, 0, 340, 10000);

auto updatechannel(soundchannel& chan) -> bool {
	if (!chan.slot)
		return false;
	int vol = soundvol, pan = 255 / 2;
	if (chan.hasloc()) {
		vec v;
		float dist = chan.loc.dist(camera1->o, v);
		int rad = maxsoundradius;
		if (chan.ent) {
			rad = chan.ent->attr2;
			if (chan.ent->attr3) {
				rad -= chan.ent->attr3;
				dist -= chan.ent->attr3;
			}
		} else if (chan.radius > 0)
			rad = maxsoundradius ? min(maxsoundradius, chan.radius) : chan.radius;
		if (rad > 0)
			vol -= int(clamp(dist / rad, 0.0f, 1.0f) * soundvol);  // simple mono distance attenuation
		if (stereo && (v.x != 0 || v.y != 0) && dist > 0) {
			v.rotate_around_z(-camera1->yaw * RAD);
			pan = int(255.9f * (0.5f - 0.5f * v.x / v.magnitude2()));  // range is from 0 (left) to 255 (right)
		}
	}
	vol = (vol * MIX_MAX_VOLUME * chan.slot->volume) / 255 / 255;
	vol = min(vol, MIX_MAX_VOLUME);
	if (vol == chan.volume && pan == chan.pan)
		return false;
	chan.volume = vol;
	chan.pan = pan;
	chan.dirty = true;
	return true;
}

void reclaimchannels() {
	loopv(channels) {
		soundchannel& chan = channels[i];
		if (chan.inuse && !Mix_Playing(i))
			freechannel(i);
	}
}

void syncchannels() {
	loopv(channels) {
		soundchannel& chan = channels[i];
		if (chan.inuse && chan.hasloc() && updatechannel(chan))
			syncchannel(chan);
	}
}

void updatesounds() {
	updatemumble();
	if (nosound)
		return;
	if (minimized)
		stopsounds();
	else {
		reclaimchannels();
		if (mainmenu)
			stopmapsounds();
		else
			checkmapsounds();
		syncchannels();
	}
	if (music) {
		if (!Mix_PlayingMusic())
			musicdone();
		else if (Mix_PausedMusic())
			Mix_ResumeMusic();
	}
}

VARP(maxsoundsatonce, 0, 7, 100);

VAR(dbgsound, 0, 0, 1);

void preloadsound(int n) {
	gamesounds.preloadsound(n);
}

void preloadmapsound(int n) {
	mapsounds.preloadsound(n);
}

void preloadmapsounds() {
	const vector<extentity*>& ents = entities::getents();
	loopv(ents) {
		extentity& e = *ents[i];
		if (e.type == ET_SOUND)
			mapsounds.preloadsound(e.attr1);
	}
}

auto playsound(int n,
			  const vec* loc,
			  extentity* ent,
			  int flags,
			  int loops,
			  int fade,
			  int chanid,
			  int radius,
			  int expire) -> int {
	if (nosound || !soundvol || minimized)
		return -1;

	soundtype& sounds = ent || flags & SND_MAP ? mapsounds : gamesounds;
	if (!sounds.configs.inrange(n)) {
		conoutf(CON_WARN, "unregistered sound: %d", n);
		return -1;
	}
	soundconfig& config = sounds.configs[n];

	if (loc && (maxsoundradius || radius > 0)) {
		// cull sounds that are unlikely to be heard
		int rad = radius > 0 ? (maxsoundradius ? min(maxsoundradius, radius) : radius) : maxsoundradius;
		if (camera1->o.dist(*loc) > 1.5f * rad) {
			if (channels.inrange(chanid) && sounds.playing(channels[chanid], config)) {
				Mix_HaltChannel(chanid);
				freechannel(chanid);
			}
			return -1;
		}
	}

	if (chanid < 0) {
		if (config.maxuses) {
			int uses = 0;
			loopv(channels) if (sounds.playing(channels[i], config) && ++uses >= config.maxuses) return -1;
		}

		// avoid bursts of sounds with heavy packetloss and in sp
		static int soundsatonce = 0, lastsoundmillis = 0;
		if (totalmillis == lastsoundmillis)
			soundsatonce++;
		else
			soundsatonce = 1;
		lastsoundmillis = totalmillis;
		if (maxsoundsatonce && soundsatonce > maxsoundsatonce)
			return -1;
	}

	if (channels.inrange(chanid)) {
		soundchannel& chan = channels[chanid];
		if (sounds.playing(chan, config)) {
			if (loc)
				chan.loc = *loc;
			else if (chan.hasloc())
				chan.clearloc();
			return chanid;
		}
	}
	if (fade < 0)
		return -1;

	soundslot& slot = sounds.slots[config.chooseslot()];
	if (!slot.sample->chunk && !slot.sample->load(sounds.dir))
		return -1;

	if (dbgsound)
		conoutf("sound: %s%s", sounds.dir, slot.sample->name);

	chanid = -1;
	loopv(channels) if (!channels[i].inuse) {
		chanid = i;
		break;
	}
	if (chanid < 0 && channels.length() < maxchannels)
		chanid = channels.length();
	if (chanid < 0)
		loopv(channels) if (!channels[i].volume) {
			Mix_HaltChannel(i);
			freechannel(i);
			chanid = i;
			break;
		}
	if (chanid < 0)
		return -1;

	soundchannel& chan = newchannel(chanid, &slot, loc, ent, flags, radius);
	updatechannel(chan);
	int playing = -1;
	if (fade) {
		Mix_Volume(chanid, chan.volume);
		playing = expire >= 0 ? Mix_FadeInChannelTimed(chanid, slot.sample->chunk, loops, fade, expire)
							  : Mix_FadeInChannel(chanid, slot.sample->chunk, loops, fade);
	} else
		playing = expire >= 0 ? Mix_PlayChannelTimed(chanid, slot.sample->chunk, loops, expire)
							  : Mix_PlayChannel(chanid, slot.sample->chunk, loops);
	if (playing >= 0)
		syncchannel(chan);
	else
		freechannel(chanid);
	return playing;
}

void stopsounds() {
	loopv(channels) if (channels[i].inuse) {
		Mix_HaltChannel(i);
		freechannel(i);
	}
}

auto stopsound(int n, int chanid, int fade) -> bool {
	if (!gamesounds.configs.inrange(n) || !channels.inrange(chanid) ||
		!gamesounds.playing(channels[chanid], gamesounds.configs[n]))
		return false;
	if (dbgsound)
		conoutf("stopsound: %s%s", gamesounds.dir, channels[chanid].slot->sample->name);
	if (!fade || !Mix_FadeOutChannel(chanid, fade)) {
		Mix_HaltChannel(chanid);
		freechannel(chanid);
	}
	return true;
}

auto playsoundname(const char* s,
				  const vec* loc,
				  int vol,
				  int flags,
				  int loops,
				  int fade,
				  int chanid,
				  int radius,
				  int expire) -> int {
	if (!vol)
		vol = 100;
	int id = gamesounds.findsound(s, vol);
	if (id < 0)
		id = gamesounds.addsound(s, vol);
	return playsound(id, loc, nullptr, flags, loops, fade, chanid, radius, expire);
}

ICOMMAND(playsound, "i", (int* n), playsound(*n));

void resetsound() {
	clearchanges(CHANGE_SOUND);
	if (!nosound) {
		gamesounds.cleanupsamples();
		mapsounds.cleanupsamples();
		if (music) {
			Mix_HaltMusic();
			Mix_FreeMusic(music);
		}
		if (musicstream)
			musicstream->seek(0, SEEK_SET);
		Mix_CloseAudio();
	}
	initsound();
	resetchannels();
	if (nosound) {
		DELETEA(musicfile);
		DELETEA(musicdonecmd);
		music = nullptr;
		gamesounds.cleanupsamples();
		mapsounds.cleanupsamples();
		return;
	}
	if (music && loadmusic(musicfile)) {
		Mix_PlayMusic(music, musicdonecmd ? 0 : -1);
		Mix_VolumeMusic((musicvol * MIX_MAX_VOLUME) / 255);
	} else {
		DELETEA(musicfile);
		DELETEA(musicdonecmd);
	}
}

COMMAND(resetsound, "");

#ifdef WIN32

#include <wchar.h>

#else

#include <unistd.h>

#ifdef _POSIX_SHARED_MEMORY_OBJECTS
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cwchar>
#endif

#endif

#if defined(WIN32) || defined(_POSIX_SHARED_MEMORY_OBJECTS)
struct MumbleInfo {
	int version, timestamp;
	vec pos, front, top;
	wchar_t name[256];
};
#endif

#ifdef WIN32
static HANDLE mumblelink = NULL;
static MumbleInfo* mumbleinfo = NULL;
#define VALID_MUMBLELINK (mumblelink && mumbleinfo)
#elif defined(_POSIX_SHARED_MEMORY_OBJECTS)
static int mumblelink = -1;
static MumbleInfo* mumbleinfo = (MumbleInfo*)-1;
#define VALID_MUMBLELINK (mumblelink >= 0 && mumbleinfo != (MumbleInfo*)-1)
#endif

#ifdef VALID_MUMBLELINK
VARFP(mumble, 0, 1, 1, {
	if (mumble)
		initmumble();
	else
		closemumble();
});
#else
VARFP(mumble, 0, 0, 1, {
	if (mumble)
		initmumble();
	else
		closemumble();
});
#endif

void initmumble() {
	if (!mumble)
		return;
#ifdef VALID_MUMBLELINK
	if (VALID_MUMBLELINK)
		return;

#ifdef WIN32
	mumblelink = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "MumbleLink");
	if (mumblelink) {
		mumbleinfo = (MumbleInfo*)MapViewOfFile(mumblelink, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MumbleInfo));
		if (mumbleinfo)
			wcsncpy(mumbleinfo->name, L"DropEngine", 256);
	}
#elif defined(_POSIX_SHARED_MEMORY_OBJECTS)
	defformatstring(shmname, "/MumbleLink.%d", getuid());
	mumblelink = shm_open(shmname, O_RDWR, 0);
	if (mumblelink >= 0) {
		mumbleinfo = (MumbleInfo*)mmap(nullptr, sizeof(MumbleInfo), PROT_READ | PROT_WRITE, MAP_SHARED, mumblelink, 0);
		if (mumbleinfo != (MumbleInfo*)-1)
			wcsncpy(mumbleinfo->name, L"DropEngine", 256);
	}
#endif
	if (!VALID_MUMBLELINK)
		closemumble();
#else
	conoutf(CON_ERROR, "Mumble positional audio is not available on this platform.");
#endif
}

void closemumble() {
#ifdef WIN32
	if (mumbleinfo) {
		UnmapViewOfFile(mumbleinfo);
		mumbleinfo = NULL;
	}
	if (mumblelink) {
		CloseHandle(mumblelink);
		mumblelink = NULL;
	}
#elif defined(_POSIX_SHARED_MEMORY_OBJECTS)
	if (mumbleinfo != (MumbleInfo*)-1) {
		munmap(mumbleinfo, sizeof(MumbleInfo));
		mumbleinfo = (MumbleInfo*)-1;
	}
	if (mumblelink >= 0) {
		close(mumblelink);
		mumblelink = -1;
	}
#endif
}

static inline auto mumblevec(const vec& v, bool pos = false) -> vec {
	// change from X left, Z up, Y forward to X right, Y up, Z forward
	// 8 cube units = 1 meter
	vec m(-v.x, v.z, v.y);
	if (pos)
		m.div(8);
	return m;
}

void updatemumble() {
#ifdef VALID_MUMBLELINK
	if (!VALID_MUMBLELINK)
		return;

	static int timestamp = 0;

	mumbleinfo->version = 1;
	mumbleinfo->timestamp = ++timestamp;

	mumbleinfo->pos = mumblevec(player->o, true);
	mumbleinfo->front = mumblevec(vec(player->yaw * RAD, player->pitch * RAD));
	mumbleinfo->top = mumblevec(vec(player->yaw * RAD, (player->pitch + 90) * RAD));
#endif
}
