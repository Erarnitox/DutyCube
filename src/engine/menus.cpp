#include "engine/engine.h"

void notifywelcome() {
	UI::hideui("servers");
}

struct change {
	int type;
	const char* desc;

	change() = default;
	change(int type, const char* desc) : type(type), desc(desc) {
	}
};
static vector<change> needsapply;

VARP(applydialog, 0, 1, 1);
VAR(hidechanges, 0, 0, 1);

void addchange(const char* desc, int type) {
	if (!applydialog)
		return;
	loopv(needsapply) if (!strcmp(needsapply[i].desc, desc)) return;
	needsapply.add(change(type, desc));
	if (!hidechanges)
		UI::showui("changes");
}

void clearchanges(int type) {
	loopvrev(needsapply) {
		change& c = needsapply[i];
		if (c.type & type) {
			c.type &= ~type;
			if (!c.type)
				needsapply.remove(i);
		}
	}
	if (needsapply.empty())
		UI::hideui("changes");
}

void applychanges() {
	int changetypes = 0;
	loopv(needsapply) changetypes |= needsapply[i].type;
	if (changetypes & CHANGE_GFX)
		execident("resetgl");
	else if (changetypes & CHANGE_SHADERS)
		execident("resetshaders");
	if (changetypes & CHANGE_SOUND)
		execident("resetsound");
}

COMMAND(applychanges, "");
ICOMMAND(pendingchanges, "b", (int* idx), {
	if (needsapply.inrange(*idx))
		result(needsapply[*idx].desc);
	else if (*idx < 0)
		intret(needsapply.length());
});

static int lastmainmenu = -1;

void menuprocess() {
	if (lastmainmenu != mainmenu) {
		lastmainmenu = mainmenu;
		execident("mainmenutoggled");
	}
	if (mainmenu && !isconnected(true) && !UI::hascursor())
		UI::showui("main");
}

VAR(mainmenu, 1, 1, 0);

void clearmainmenu() {
	hidechanges = 0;
	if (mainmenu && isconnected()) {
		mainmenu = 0;
		UI::hideui(nullptr);
	}
}
