// console.cpp: the console buffer, its display, and command line control

#include "SDL_shape.h"
#include "engine/engine.h"

enum {
MAXCONLINES = 200
};
struct cline {
	char* line;
	int type, outtime;
};
reversequeue<cline, MAXCONLINES> conlines;

int commandmillis = -1;
string commandbuf;
char *commandaction = nullptr, *commandprompt = nullptr;
enum { CF_COMPLETE = 1 << 0, CF_EXECUTE = 1 << 1 };
int commandflags = 0, commandpos = -1;

VARFP(maxcon, 10, 200, MAXCONLINES, {
	while (conlines.length() > maxcon)
		delete[] conlines.pop().line;
});

#define CONSTRLEN 512

void conline(int type, const char* sf)	// add a line to the console buffer
{
	char* buf = conlines.length() >= maxcon ? conlines.remove().line : newstring("", CONSTRLEN - 1);
	cline& cl = conlines.add();
	cl.line = buf;
	cl.type = type;
	cl.outtime = totalmillis;  // for how long to keep line on screen
	copystring(cl.line, sf, CONSTRLEN);
}

void conoutfv(int type, const char* fmt, va_list args) {
	static char buf[CONSTRLEN];
	vformatstring(buf, fmt, args, sizeof(buf));
	conline(type, buf);
	logoutf("%s", buf);
}

void conoutf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	conoutfv(CON_INFO, fmt, args);
	va_end(args);
}

void conoutf(int type, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	conoutfv(type, fmt, args);
	va_end(args);
}

ICOMMAND(fullconsole, "iN$", (int* val, int* numargs, ident* id), {
	if (*numargs > 0)
		UI::holdui("fullconsole", *val != 0);
	else {
		int vis = UI::uivisible("fullconsole") ? 1 : 0;
		if (*numargs < 0)
			intret(vis);
		else
			printvar(id, vis);
	}
});
ICOMMAND(toggleconsole, "", (), UI::toggleui("fullconsole"));

auto rendercommand(float x, float y, float w) -> float {
	if (commandmillis < 0)
		return 0;

	char buf[CONSTRLEN];
	const char* prompt = commandprompt ? commandprompt : ">";
	formatstring(buf, "%s %s", prompt, commandbuf);

	float width, height;
	text_boundsf(buf, width, height, w);
	y -= height;
	draw_text(buf, x, y, 0xFF, 0xFF, 0xFF, 0xFF, commandpos >= 0 ? commandpos + 1 + strlen(prompt) : strlen(buf), w);
	return height;
}

VARP(consize, 0, 5, 100);
VARP(miniconsize, 0, 5, 100);
VARP(miniconwidth, 0, 40, 100);
VARP(confade, 0, 30, 60);
VARP(miniconfade, 0, 30, 60);
VARP(fullconsize, 0, 75, 100);
HVARP(confilter, 0, 0xFFFFFF, 0xFFFFFF);
HVARP(fullconfilter, 0, 0xFFFFFF, 0xFFFFFF);
HVARP(miniconfilter, 0, 0, 0xFFFFFF);

int conskip = 0, miniconskip = 0;

void setconskip(int& skip, int filter, int n) {
	int offset = abs(n), dir = n < 0 ? -1 : 1;
	skip = clamp(skip, 0, conlines.length() - 1);
	while (offset) {
		skip += dir;
		if (!conlines.inrange(skip)) {
			skip = clamp(skip, 0, conlines.length() - 1);
			return;
		}
		if (conlines[skip].type & filter)
			--offset;
	}
}

ICOMMAND(conskip, "i", (int* n), setconskip(conskip, UI::uivisible("fullconsole") ? fullconfilter : confilter, *n));
ICOMMAND(miniconskip, "i", (int* n), setconskip(miniconskip, miniconfilter, *n));

ICOMMAND(clearconsole, "", (), {
	while (conlines.length())
		delete[] conlines.pop().line;
});

auto drawconlines(int conskip,
				   int confade,
				   float conwidth,
				   float conheight,
				   float conoff,
				   int filter,
				   float y = 0,
				   int dir = 1) -> float {
	int numl = conlines.length();
	int offset = min(conskip, numl);

	if (confade) {
		if (!conskip) {
			numl = 0;
			loopvrev(conlines) if (totalmillis - conlines[i].outtime < confade * 300) {
				numl = i + 1;
				break;
			}
		} else
			offset--;
	}

	int totalheight = 0;
	loopi(numl)	 // determine visible height
	{
		// shuffle backwards to fill if necessary
		int idx = offset + i < numl ? offset + i : --offset;
		if (!(conlines[idx].type & filter))
			continue;
		char* line = conlines[idx].line;
		float width, height;
		text_boundsf(line, width, height, conwidth);
		if (totalheight + height > conheight) {
			numl = i;
			if (offset == idx)
				++offset;
			break;
		}
		totalheight += height;
	}
	if (dir > 0)
		y = conoff;
	loopi(numl) {
		int idx = offset + (dir > 0 ? i : numl - i - 1);
		if (!(conlines[idx].type & filter))
			continue;
		char* line = conlines[idx].line;
		float width, height;
		text_boundsf(line, width, height, conwidth);
		if (dir >= 0)
			y -= height;
		float stath = conheight * 1.5f;
		draw_text(line, conoff, (hudh / conscale) + (y - stath), 0xFF, 0xFF, 0xFF, 0xFF, -1, conwidth);
	}
	return y + conoff;
}

auto renderfullconsole(float w, float h) -> float {
	float conpad = FONTH * 2, conheight = h - 2 * conpad, conwidth = w - 2 * conpad;
	drawconlines(conskip, 0, conwidth, conheight, conpad, fullconfilter);
	return conheight + 2 * conpad;
}

auto renderconsole(float w, float h, float abovehud) -> float {
	float conpad = FONTH * 2, conheight = min(float(FONTH * consize), h - 2 * conpad),
		  conwidth = w - 2 * conpad - game::clipconsole(w, h);
	float y = drawconlines(conskip, confade, conwidth, conheight, conpad, confilter);
	if (miniconsize && miniconwidth)
		drawconlines(miniconskip,
					 miniconfade,
					 (miniconwidth * (w - 2 * conpad)) / 100,
					 min(float(FONTH * miniconsize), abovehud - y),
					 conpad,
					 miniconfilter,
					 abovehud,
					 -1);
	return y;
}

// keymap is defined externally in keymap.cfg

struct keym {
	enum { ACTION_DEFAULT = 0, ACTION_SPECTATOR, ACTION_EDITING, NUMACTIONS };

	int code{-1};
	char* name{NULL};
	char* actions[NUMACTIONS];
	bool pressed{false};

	keym()  {
		loopi(NUMACTIONS) actions[i] = newstring("");
	}
	~keym() {
		DELETEA(name);
		loopi(NUMACTIONS) DELETEA(actions[i]);
	}

	void clear(int type);
	void clear() {
		loopi(NUMACTIONS) clear(i);
	}
};

hashtable<int, keym> keyms(128);

void keymap(int* code, char* key) {
	if (identflags & IDF_OVERRIDDEN) {
		conoutf(CON_ERROR, "cannot override keymap %d", *code);
		return;
	}
	keym& km = keyms[*code];
	km.code = *code;
	DELETEA(km.name);
	km.name = newstring(key);
}

COMMAND(keymap, "is");

keym* keypressed = nullptr;
char* keyaction = nullptr;

auto getkeyname(int code) -> const char* {
	keym* km = keyms.access(code);
	return km ? km->name : nullptr;
}

void searchbinds(char* action, int type) {
	vector<char> names;
	enumerate(keyms, keym, km, {
		if (!strcmp(km.actions[type], action)) {
			if (names.length())
				names.add(' ');
			names.put(km.name, strlen(km.name));
		}
	});
	names.add('\0');
	result(names.getbuf());
}

auto findbind(char* key) -> keym* {
	enumerate(keyms, keym, km, {
		if (!strcasecmp(km.name, key))
			return &km;
	});
	return nullptr;
}

void getbind(char* key, int type) {
	keym* km = findbind(key);
	result(km ? km->actions[type] : "");
}

void bindkey(char* key, char* action, int state, const char* cmd) {
	if (identflags & IDF_OVERRIDDEN) {
		conoutf(CON_ERROR, "cannot override %s \"%s\"", cmd, key);
		return;
	}
	keym* km = findbind(key);
	if (!km) {
		conoutf(CON_ERROR, "unknown key \"%s\"", key);
		return;
	}
	char*& binding = km->actions[state];
	if (!keypressed || keyaction != binding)
		delete[] binding;
	// trim white-space to make searchbinds more reliable
	while (iscubespace(*action))
		action++;
	int len = strlen(action);
	while (len > 0 && iscubespace(action[len - 1]))
		len--;
	binding = newstring(action, len);
}

ICOMMAND(bind, "ss", (char* key, char* action), bindkey(key, action, keym::ACTION_DEFAULT, "bind"));
ICOMMAND(specbind, "ss", (char* key, char* action), bindkey(key, action, keym::ACTION_SPECTATOR, "specbind"));
ICOMMAND(editbind, "ss", (char* key, char* action), bindkey(key, action, keym::ACTION_EDITING, "editbind"));
ICOMMAND(getbind, "s", (char* key), getbind(key, keym::ACTION_DEFAULT));
ICOMMAND(getspecbind, "s", (char* key), getbind(key, keym::ACTION_SPECTATOR));
ICOMMAND(geteditbind, "s", (char* key), getbind(key, keym::ACTION_EDITING));
ICOMMAND(searchbinds, "s", (char* action), searchbinds(action, keym::ACTION_DEFAULT));
ICOMMAND(searchspecbinds, "s", (char* action), searchbinds(action, keym::ACTION_SPECTATOR));
ICOMMAND(searcheditbinds, "s", (char* action), searchbinds(action, keym::ACTION_EDITING));

void keym::clear(int type) {
	char*& binding = actions[type];
	if (binding[0]) {
		if (!keypressed || keyaction != binding)
			delete[] binding;
		binding = newstring("");
	}
}

ICOMMAND(clearbinds, "", (), enumerate(keyms, keym, km, km.clear(keym::ACTION_DEFAULT)));
ICOMMAND(clearspecbinds, "", (), enumerate(keyms, keym, km, km.clear(keym::ACTION_SPECTATOR)));
ICOMMAND(cleareditbinds, "", (), enumerate(keyms, keym, km, km.clear(keym::ACTION_EDITING)));
ICOMMAND(clearallbinds, "", (), enumerate(keyms, keym, km, km.clear()));

void inputcommand(char* init,
				  char* action = nullptr,
				  char* prompt = nullptr,
				  char* flags = nullptr)  // turns input to the command line on or off
{
	commandmillis = init ? totalmillis : -1;
	textinput(commandmillis >= 0, TI_CONSOLE);
	keyrepeat(commandmillis >= 0, KR_CONSOLE);
	copystring(commandbuf, init ? init : "");
	DELETEA(commandaction);
	DELETEA(commandprompt);
	commandpos = -1;
	if (action && action[0])
		commandaction = newstring(action);
	if (prompt && prompt[0])
		commandprompt = newstring(prompt);
	commandflags = 0;
	if (flags)
		while (*flags)
			switch (*flags++) {
			case 'c':
				commandflags |= CF_COMPLETE;
				break;
			case 'x':
				commandflags |= CF_EXECUTE;
				break;
			case 's':
				commandflags |= CF_COMPLETE | CF_EXECUTE;
				break;
			}
	else if (init)
		commandflags |= CF_COMPLETE | CF_EXECUTE;
}

ICOMMAND(saycommand, "C", (char* init), inputcommand(init));
COMMAND(inputcommand, "ssss");

void pasteconsole() {
	if (!SDL_HasClipboardText())
		return;
	char* cb = SDL_GetClipboardText();
	if (!cb)
		return;
	size_t cblen = strlen(cb), commandlen = strlen(commandbuf),
		   decoded = decodeutf8(
			   (uchar*)&commandbuf[commandlen], sizeof(commandbuf) - 1 - commandlen, (const uchar*)cb, cblen);
	commandbuf[commandlen + decoded] = '\0';
	SDL_free(cb);
}

struct hline {
	char *buf{NULL}, *action{NULL}, *prompt{NULL};
	int flags{0};

	hline()  {
	}
	~hline() {
		DELETEA(buf);
		DELETEA(action);
		DELETEA(prompt);
	}

	void restore() {
		copystring(commandbuf, buf);
		if (commandpos >= (int)strlen(commandbuf))
			commandpos = -1;
		DELETEA(commandaction);
		DELETEA(commandprompt);
		if (action)
			commandaction = newstring(action);
		if (prompt)
			commandprompt = newstring(prompt);
		commandflags = flags;
	}

	auto shouldsave() -> bool {
		return strcmp(commandbuf, buf) || (commandaction ? !action || strcmp(commandaction, action) : action != nullptr) ||
			(commandprompt ? !prompt || strcmp(commandprompt, prompt) : prompt != nullptr) || commandflags != flags;
	}

	void save() {
		buf = newstring(commandbuf);
		if (commandaction)
			action = newstring(commandaction);
		if (commandprompt)
			prompt = newstring(commandprompt);
		flags = commandflags;
	}

	void run() {
		if (flags & CF_EXECUTE && buf[0] == '/')
			execute(buf + 1);
		else if (action) {
			alias("commandbuf", buf);
			execute(action);
		} else
			game::toserver(buf);
	}
};
vector<hline*> history;
int histpos = 0;

VARP(maxhistory, 0, 1000, 10000);

void history_(int* n) {
	static bool inhistory = false;
	if (!inhistory && history.inrange(*n)) {
		inhistory = true;
		history[history.length() - *n - 1]->run();
		inhistory = false;
	}
}

COMMANDN(history, history_, "i");

struct releaseaction {
	keym* key;
	union {
		char* action;
		ident* id;
	};
	int numargs;
	tagval args[3];
};
vector<releaseaction> releaseactions;

auto addreleaseaction(char* s) -> const char* {
	if (!keypressed) {
		delete[] s;
		return nullptr;
	}
	releaseaction& ra = releaseactions.add();
	ra.key = keypressed;
	ra.action = s;
	ra.numargs = -1;
	return keypressed->name;
}

auto addreleaseaction(ident* id, int numargs) -> tagval* {
	if (!keypressed || numargs > 3)
		return nullptr;
	releaseaction& ra = releaseactions.add();
	ra.key = keypressed;
	ra.id = id;
	ra.numargs = numargs;
	return ra.args;
}

void onrelease(const char* s) {
	addreleaseaction(newstring(s));
}

COMMAND(onrelease, "s");

void execbind(keym& k, bool isdown) {
	loopv(releaseactions) {
		releaseaction& ra = releaseactions[i];
		if (ra.key == &k) {
			if (ra.numargs < 0) {
				if (!isdown)
					execute(ra.action);
				delete[] ra.action;
			} else
				execute(isdown ? nullptr : ra.id, ra.args, ra.numargs);
			releaseactions.remove(i--);
		}
	}
	if (isdown) {
		int state = keym::ACTION_DEFAULT;
		if (!mainmenu) {
			if (editmode)
				state = keym::ACTION_EDITING;
			else if (player->state == CS_SPECTATOR)
				state = keym::ACTION_SPECTATOR;
		}
		char*& action = k.actions[state][0] ? k.actions[state] : k.actions[keym::ACTION_DEFAULT];
		keyaction = action;
		keypressed = &k;
		execute(keyaction);
		keypressed = nullptr;
		if (keyaction != action)
			delete[] keyaction;
	}
	k.pressed = isdown;
}

auto consoleinput(const char* str, int len) -> bool {
	if (commandmillis < 0)
		return false;

	resetcomplete();
	int cmdlen = (int)strlen(commandbuf), cmdspace = int(sizeof(commandbuf)) - (cmdlen + 1);
	len = min(len, cmdspace);
	if (commandpos < 0) {
		memcpy(&commandbuf[cmdlen], str, len);
	} else {
		memmove(&commandbuf[commandpos + len], &commandbuf[commandpos], cmdlen - commandpos);
		memcpy(&commandbuf[commandpos], str, len);
		commandpos += len;
	}
	commandbuf[cmdlen + len] = '\0';

	return true;
}

auto consolekey(int code, bool isdown) -> bool {
	if (commandmillis < 0)
		return false;

#ifdef __APPLE__
#define MOD_KEYS (KMOD_LGUI | KMOD_RGUI)
#else
#define MOD_KEYS (KMOD_LCTRL | KMOD_RCTRL)
#endif

	if (isdown) {
		switch (code) {
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			break;

		case SDLK_HOME:
			if (strlen(commandbuf))
				commandpos = 0;
			break;

		case SDLK_END:
			commandpos = -1;
			break;

		case SDLK_DELETE: {
			int len = (int)strlen(commandbuf);
			if (commandpos < 0)
				break;
			memmove(&commandbuf[commandpos], &commandbuf[commandpos + 1], len - commandpos);
			resetcomplete();
			if (commandpos >= len - 1)
				commandpos = -1;
			break;
		}

		case SDLK_BACKSPACE: {
			int len = (int)strlen(commandbuf), i = commandpos >= 0 ? commandpos : len;
			if (i < 1)
				break;
			memmove(&commandbuf[i - 1], &commandbuf[i], len - i + 1);
			resetcomplete();
			if (commandpos > 0)
				commandpos--;
			else if (!commandpos && len <= 1)
				commandpos = -1;
			break;
		}

		case SDLK_LEFT:
			if (commandpos > 0)
				commandpos--;
			else if (commandpos < 0)
				commandpos = (int)strlen(commandbuf) - 1;
			break;

		case SDLK_RIGHT:
			if (commandpos >= 0 && ++commandpos >= (int)strlen(commandbuf))
				commandpos = -1;
			break;

		case SDLK_UP:
			if (histpos > history.length())
				histpos = history.length();
			if (histpos > 0)
				history[--histpos]->restore();
			break;

		case SDLK_DOWN:
			if (histpos + 1 < history.length())
				history[++histpos]->restore();
			break;

		case SDLK_TAB:
			if (commandflags & CF_COMPLETE) {
				complete(commandbuf, sizeof(commandbuf), commandflags & CF_EXECUTE ? "/" : nullptr);
				if (commandpos >= 0 && commandpos >= (int)strlen(commandbuf))
					commandpos = -1;
			}
			break;

		case SDLK_v:
			if (SDL_GetModState() & MOD_KEYS)
				pasteconsole();
			break;
		}
	} else {
		if (code == SDLK_RETURN || code == SDLK_KP_ENTER) {
			hline* h = nullptr;
			if (commandbuf[0]) {
				if (history.empty() || history.last()->shouldsave()) {
					if (maxhistory && history.length() >= maxhistory) {
						loopi(history.length() - maxhistory + 1) delete history[i];
						history.remove(0, history.length() - maxhistory + 1);
					}
					history.add(h = new hline)->save();
				} else
					h = history.last();
			}
			histpos = history.length();
			inputcommand(nullptr);
			if (h)
				h->run();
		} else if (code == SDLK_ESCAPE) {
			histpos = history.length();
			inputcommand(nullptr);
		}
	}

	return true;
}

void processtextinput(const char* str, int len) {
	if (!UI::textinput(str, len))
		consoleinput(str, len);
}

void processkey(int code, bool isdown) {
	keym* haskey = keyms.access(code);
	if (haskey && haskey->pressed)
		execbind(*haskey, isdown);	// allow pressed keys to release
	else if (!UI::keypress(code, isdown))  // UI key intercept
	{
		if (!consolekey(code, isdown)) {
			if (haskey)
				execbind(*haskey, isdown);
		}
	}
}

void clear_console() {
	keyms.clear();
}

void writebinds(stream* f) {
	static const char* const cmds[3] = {"bind", "specbind", "editbind"};
	vector<keym*> binds;
	enumerate(keyms, keym, km, binds.add(&km));
	binds.sortname();
	loopj(3) {
		loopv(binds) {
			keym& km = *binds[i];
			if (*km.actions[j]) {
				if (validateblock(km.actions[j]))
					f->printf("%s %s [%s]\n", cmds[j], escapestring(km.name), km.actions[j]);
				else
					f->printf("%s %s %s\n", cmds[j], escapestring(km.name), escapestring(km.actions[j]));
			}
		}
	}
}

// tab-completion of all idents and base maps

enum { FILES_DIR = 0, FILES_LIST };

struct fileskey {
	int type;
	const char *dir, *ext;

	fileskey() = default;
	fileskey(int type, const char* dir, const char* ext) : type(type), dir(dir), ext(ext) {
	}
};

struct filesval {
	int type;
	char *dir, *ext;
	vector<char*> files;
	int millis{-1};

	filesval(int type, const char* dir, const char* ext)
		: type(type), dir(newstring(dir)), ext(ext && ext[0] ? newstring(ext) : nullptr) {
	}
	~filesval() {
		DELETEA(dir);
		DELETEA(ext);
		files.deletearrays();
	}

	void update() {
		if (type != FILES_DIR || millis >= commandmillis)
			return;
		files.deletearrays();
		listfiles(dir, ext, files);
		files.sort();
		loopv(files) if (i && !strcmp(files[i], files[i - 1])) delete[] files.remove(i--);
		millis = totalmillis;
	}
};

static inline auto htcmp(const fileskey& x, const fileskey& y) -> bool {
	return x.type == y.type && !strcmp(x.dir, y.dir) && (x.ext == y.ext || (x.ext && y.ext && !strcmp(x.ext, y.ext)));
}

static inline auto hthash(const fileskey& k) -> uint {
	return hthash(k.dir);
}

static hashtable<fileskey, filesval*> completefiles;
static hashtable<char*, filesval*> completions;

int completesize = 0;
char* lastcomplete = nullptr;

void resetcomplete() {
	completesize = 0;
}

void addcomplete(char* command, int type, char* dir, char* ext) {
	if (identflags & IDF_OVERRIDDEN) {
		conoutf(CON_ERROR, "cannot override complete %s", command);
		return;
	}
	if (!dir[0]) {
		filesval** hasfiles = completions.access(command);
		if (hasfiles)
			*hasfiles = nullptr;
		return;
	}
	if (type == FILES_DIR) {
		int dirlen = (int)strlen(dir);
		while (dirlen > 0 && (dir[dirlen - 1] == '/' || dir[dirlen - 1] == '\\'))
			dir[--dirlen] = '\0';
		if (ext) {
			if (strchr(ext, '*'))
				ext[0] = '\0';
			if (!ext[0])
				ext = nullptr;
		}
	}
	fileskey key(type, dir, ext);
	filesval** val = completefiles.access(key);
	if (!val) {
		auto* f = new filesval(type, dir, ext);
		if (type == FILES_LIST)
			explodelist(dir, f->files);
		val = &completefiles[fileskey(type, f->dir, f->ext)];
		*val = f;
	}
	filesval** hasfiles = completions.access(command);
	if (hasfiles)
		*hasfiles = *val;
	else
		completions[newstring(command)] = *val;
}

void addfilecomplete(char* command, char* dir, char* ext) {
	addcomplete(command, FILES_DIR, dir, ext);
}

void addlistcomplete(char* command, char* list) {
	addcomplete(command, FILES_LIST, list, nullptr);
}

COMMANDN(complete, addfilecomplete, "sss");
COMMANDN(listcomplete, addlistcomplete, "ss");

void complete(char* s, int maxlen, const char* cmdprefix) {
	int cmdlen = 0;
	if (cmdprefix) {
		cmdlen = strlen(cmdprefix);
		if (strncmp(s, cmdprefix, cmdlen))
			prependstring(s, cmdprefix, maxlen);
	}
	if (!s[cmdlen])
		return;
	if (!completesize) {
		completesize = (int)strlen(&s[cmdlen]);
		DELETEA(lastcomplete);
	}

	filesval* f = nullptr;
	if (completesize) {
		char* end = strchr(&s[cmdlen], ' ');
		if (end)
			f = completions.find(stringslice(&s[cmdlen], end), NULL);
	}

	const char* nextcomplete = nullptr;
	if (f)	// complete using filenames
	{
		int commandsize = strchr(&s[cmdlen], ' ') + 1 - s;
		f->update();
		loopv(f->files) {
			if (strncmp(f->files[i], &s[commandsize], completesize + cmdlen - commandsize) == 0 &&
				(!lastcomplete || strcmp(f->files[i], lastcomplete) > 0) &&
				(!nextcomplete || strcmp(f->files[i], nextcomplete) < 0))
				nextcomplete = f->files[i];
		}
		cmdprefix = s;
		cmdlen = commandsize;
	} else	// complete using command names
	{
		enumerate(idents,
				  ident,
				  id,
				  if (strncmp(id.name, &s[cmdlen], completesize) == 0 &&
					  (!lastcomplete || strcmp(id.name, lastcomplete) > 0) &&
					  (!nextcomplete || strcmp(id.name, nextcomplete) < 0)) nextcomplete = id.name;);
	}
	DELETEA(lastcomplete);
	if (nextcomplete) {
		cmdlen = min(cmdlen, maxlen - 1);
		if (cmdlen)
			memmove(s, cmdprefix, cmdlen);
		copystring(&s[cmdlen], nextcomplete, maxlen - cmdlen);
		lastcomplete = newstring(nextcomplete);
	}
}

void writecompletions(stream* f) {
	vector<char*> cmds;
	enumeratekt(completions, char*, k, filesval*, v, {
		if (v)
			cmds.add(k);
	});
	cmds.sort();
	loopv(cmds) {
		char* k = cmds[i];
		filesval* v = completions[k];
		if (v->type == FILES_LIST) {
			if (validateblock(v->dir))
				f->printf("listcomplete %s [%s]\n", escapeid(k), v->dir);
			else
				f->printf("listcomplete %s %s\n", escapeid(k), escapestring(v->dir));
		} else
			f->printf("complete %s %s %s\n", escapeid(k), escapestring(v->dir), escapestring(v->ext ? v->ext : "*"));
	}
}
