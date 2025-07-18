#include "engine/engine.h"

static hashnameset<font> fonts;
static font* fontdef = nullptr;
static int fontdeftex = 0;

font* curfont = nullptr;
int curfonttex = 0;

void newfont(char* name, char* tex, int* defaultw, int* defaulth, int* scale) {
	font* f = &fonts[name];
	if (!f->name)
		f->name = newstring(name);
	f->texs.shrink(0);
	f->texs.add(textureload(tex));
	f->chars.shrink(0);
	f->charoffset = '!';
	f->defaultw = *defaultw;
	f->defaulth = *defaulth;
	f->scale = *scale > 0 ? *scale : f->defaulth;
	f->bordermin = 0.49f;
	f->bordermax = 0.5f;
	f->outlinemin = -1;
	f->outlinemax = 0;

	fontdef = f;
	fontdeftex = 0;
}

void fontborder(float* bordermin, float* bordermax) {
	if (!fontdef)
		return;

	fontdef->bordermin = *bordermin;
	fontdef->bordermax = max(*bordermax, *bordermin + 0.01f);
}

void fontoutline(float* outlinemin, float* outlinemax) {
	if (!fontdef)
		return;

	fontdef->outlinemin = min(*outlinemin, *outlinemax - 0.01f);
	fontdef->outlinemax = *outlinemax;
}

void fontoffset(char* c) {
	if (!fontdef)
		return;

	fontdef->charoffset = c[0];
}

void fontscale(int* scale) {
	if (!fontdef)
		return;

	fontdef->scale = *scale > 0 ? *scale : fontdef->defaulth;
}

void fonttex(char* s) {
	if (!fontdef)
		return;

	Texture* t = textureload(s);
	loopv(fontdef->texs) if (fontdef->texs[i] == t) {
		fontdeftex = i;
		return;
	}
	fontdeftex = fontdef->texs.length();
	fontdef->texs.add(t);
}

void fontchar(float* x, float* y, float* w, float* h, float* offsetx, float* offsety, float* advance) {
	if (!fontdef)
		return;

	font::charinfo& c = fontdef->chars.add();
	c.x = *x;
	c.y = *y;
	c.w = *w ? *w : fontdef->defaultw;
	c.h = *h ? *h : fontdef->defaulth;
	c.offsetx = *offsetx;
	c.offsety = *offsety;
	c.advance = *advance ? *advance : c.offsetx + c.w;
	c.tex = fontdeftex;
}

void fontskip(int* n) {
	if (!fontdef)
		return;
	loopi(max(*n, 1)) {
		font::charinfo& c = fontdef->chars.add();
		c.x = c.y = c.w = c.h = c.offsetx = c.offsety = c.advance = 0;
		c.tex = 0;
	}
}

COMMANDN(font, newfont, "ssiii");
COMMAND(fontborder, "ff");
COMMAND(fontoutline, "ff");
COMMAND(fontoffset, "s");
COMMAND(fontscale, "i");
COMMAND(fonttex, "s");
COMMAND(fontchar, "fffffff");
COMMAND(fontskip, "i");

void fontalias(const char* dst, const char* src) {
	font* s = fonts.access(src);
	if (!s)
		return;
	font* d = &fonts[dst];
	if (!d->name)
		d->name = newstring(dst);
	d->texs = s->texs;
	d->chars = s->chars;
	d->charoffset = s->charoffset;
	d->defaultw = s->defaultw;
	d->defaulth = s->defaulth;
	d->scale = s->scale;
	d->bordermin = s->bordermin;
	d->bordermax = s->bordermax;
	d->outlinemin = s->outlinemin;
	d->outlinemax = s->outlinemax;

	fontdef = d;
	fontdeftex = d->texs.length() - 1;
}

COMMAND(fontalias, "ss");

auto findfont(const char* name) -> font* {
	return fonts.access(name);
}

auto setfont(const char* name) -> bool {
	font* f = fonts.access(name);
	if (!f)
		return false;
	curfont = f;
	return true;
}

static vector<font*> fontstack;

void pushfont() {
	fontstack.add(curfont);
}

auto popfont() -> bool {
	if (fontstack.empty())
		return false;
	curfont = fontstack.pop();
	return true;
}

void gettextres(int& w, int& h) {
	if (w < MINRESW || h < MINRESH) {
		if (MINRESW > w * MINRESH / h) {
			h = h * MINRESW / w;
			w = MINRESW;
		} else {
			w = w * MINRESH / h;
			h = MINRESH;
		}
	}
}

auto text_widthf(const char* str) -> float {
	float width, height;
	text_boundsf(str, width, height);
	return width;
}

#define FONTTAB (4 * FONTW)
#define TEXTTAB(x) ((int((x) / FONTTAB) + 1.0f) * FONTTAB)

void tabify(const char* str, int* numtabs) {
	int tw = max(*numtabs, 0) * FONTTAB - 1, tabs = 0;
	for (float w = text_widthf(str); w <= tw; w = TEXTTAB(w))
		++tabs;
	int len = strlen(str);
	char* tstr = newstring(len + tabs);
	memcpy(tstr, str, len);
	memset(&tstr[len], '\t', tabs);
	tstr[len + tabs] = '\0';
	stringret(tstr);
}

COMMAND(tabify, "si");

void draw_textf(const char* fstr, float left, float top, ...) {
	defvformatstring(str, top, fstr);
	draw_text(str, left, top);
}

const matrix4x3* textmatrix = nullptr;
float textscale = 1;

static auto draw_char(Texture*& tex, int c, float x, float y, float scale) -> float {
	font::charinfo& info = curfont->chars[c - curfont->charoffset];
	if (tex != curfont->texs[info.tex]) {
		xtraverts += gle::end();
		tex = curfont->texs[info.tex];
		glBindTexture(GL_TEXTURE_2D, tex->id);
	}

	x *= textscale;
	y *= textscale;
	scale *= textscale;

	float x1 = x + scale * info.offsetx, y1 = y + scale * info.offsety, x2 = x + scale * (info.offsetx + info.w),
		  y2 = y + scale * (info.offsety + info.h), tx1 = info.x / tex->xs, ty1 = info.y / tex->ys,
		  tx2 = (info.x + info.w) / tex->xs, ty2 = (info.y + info.h) / tex->ys;

	if (textmatrix) {
		gle::attrib(textmatrix->transform(vec2(x1, y1)));
		gle::attribf(tx1, ty1);
		gle::attrib(textmatrix->transform(vec2(x2, y1)));
		gle::attribf(tx2, ty1);
		gle::attrib(textmatrix->transform(vec2(x2, y2)));
		gle::attribf(tx2, ty2);
		gle::attrib(textmatrix->transform(vec2(x1, y2)));
		gle::attribf(tx1, ty2);
	} else {
		gle::attribf(x1, y1);
		gle::attribf(tx1, ty1);
		gle::attribf(x2, y1);
		gle::attribf(tx2, ty1);
		gle::attribf(x2, y2);
		gle::attribf(tx2, ty2);
		gle::attribf(x1, y2);
		gle::attribf(tx1, ty2);
	}

	return scale * info.advance;
}

VARP(textbright, 0, 85, 100);

// stack[sp] is current color index
static void text_color(char c, char* stack, int size, int& sp, bvec color, int a) {
	if (c == 's')  // save color
	{
		c = stack[sp];
		if (sp < size - 1)
			stack[++sp] = c;
	} else {
		xtraverts += gle::end();
		if (c == 'r') {
			if (sp > 0)
				--sp;
			c = stack[sp];
		}  // restore color
		else
			stack[sp] = c;
		switch (c) {
		case '0':
			//#55a56a green
			color = bvec(0x55, 0xa5, 0x6a);
			break;	// green: player talk
		case '1':
			//#1167a0 blue
			color = bvec(0x11, 0x67, 0xa0);
			break;	// blue: "echo" command
		case '2':
			//#f5eb8a yellow
			color = bvec(0xf5, 0xeb, 0x8a);
			break;	// yellow: gameplay messages
		case '3':
			// #ec501d red
			color = bvec(0xec, 0x50, 0x1d);
			break;	// red: important errors
		case '4':
			// #4f585d gray
			color = bvec(0x4f, 0x58, 0x5d);
			break;	// gray
		case '5':
			color = bvec(192, 64, 192);
			break;	// magenta
		case '6':
			color = bvec(255, 128, 0);
			break;	// orange
		case '7':
			color = bvec(255, 255, 255);
			break;	// white
		case '8':
			// #6fcdf1
			color = bvec(0x6f, 0xcd, 0xf1);
			break;	// "DropEngine Blue" (#2B51FC)
		case '9':
			// #bfe4f6 Ice-Blue
			color = bvec(0xbf, 0xe4, 0xf6);
			break;
		default:
			gle::color(color, a);
			return;	 // provided color: everything else
		}
		if (textbright != 100)
			color.scale(textbright, 100);
		gle::color(color, a);
	}
}

#define TEXTSKELETON                                                                    \
	float y = 0, x = 0, scale = curfont->scale / float(curfont->defaulth);              \
	int i;                                                                              \
	for (i = 0; str[i]; i++) {                                                          \
		TEXTINDEX(i)                                                                    \
		int c = uchar(str[i]);                                                          \
		if (c == '\t') {                                                                \
			x = TEXTTAB(x);                                                             \
			TEXTWHITE(i)                                                                \
		} else if (c == ' ') {                                                          \
			x += scale * curfont->defaultw;                                             \
			TEXTWHITE(i)                                                                \
		} else if (c == '\n') {                                                         \
			TEXTLINE(i) x = 0;                                                          \
			y += FONTH;                                                                 \
		} else if (c == '\f') {                                                         \
			if (str[i + 1]) {                                                           \
				i++;                                                                    \
				TEXTCOLOR(i)                                                            \
			}                                                                           \
		} else if (curfont->chars.inrange(c - curfont->charoffset)) {                   \
			float cw = scale * curfont->chars[c - curfont->charoffset].advance;         \
			if (cw <= 0)                                                                \
				continue;                                                               \
			if (maxwidth >= 0) {                                                        \
				int j = i;                                                              \
				float w = cw;                                                           \
				for (; str[i + 1]; i++) {                                               \
					int c = uchar(str[i + 1]);                                          \
					if (c == '\f') {                                                    \
						if (str[i + 2])                                                 \
							i++;                                                        \
						continue;                                                       \
					}                                                                   \
					if (!curfont->chars.inrange(c - curfont->charoffset))               \
						break;                                                          \
					float cw = scale * curfont->chars[c - curfont->charoffset].advance; \
					if (cw <= 0 || w + cw > maxwidth)                                   \
						break;                                                          \
					w += cw;                                                            \
				}                                                                       \
				if (x + w > maxwidth && x > 0) {                                        \
					(void)j;                                                            \
					TEXTLINE(j - 1) x = 0;                                              \
					y += FONTH;                                                         \
				}                                                                       \
				TEXTWORD                                                                \
			} else {                                                                    \
				TEXTCHAR(i)                                                             \
			}                                                                           \
		}                                                                               \
	}

// all the chars are guaranteed to be either drawable or color commands
#define TEXTWORDSKELETON                                                        \
	for (; j <= i; j++) {                                                       \
		TEXTINDEX(j)                                                            \
		int c = uchar(str[j]);                                                  \
		if (c == '\f') {                                                        \
			if (str[j + 1]) {                                                   \
				j++;                                                            \
				TEXTCOLOR(j)                                                    \
			}                                                                   \
		} else {                                                                \
			float cw = scale * curfont->chars[c - curfont->charoffset].advance; \
			TEXTCHAR(j)                                                         \
		}                                                                       \
	}

#define TEXTEND(cursor)        \
	if (cursor >= i) {         \
		do {                   \
			TEXTINDEX(cursor); \
		} while (0);           \
	}

auto text_visible(const char* str, float hitx, float hity, int maxwidth) -> int {
#define TEXTINDEX(idx)
#define TEXTWHITE(idx)                 \
	if (y + FONTH > hity && x >= hitx) \
		return idx;
#define TEXTLINE(idx)     \
	if (y + FONTH > hity) \
		return idx;
#define TEXTCOLOR(idx)
#define TEXTCHAR(idx) \
	x += cw;          \
	TEXTWHITE(idx)
#define TEXTWORD TEXTWORDSKELETON
	TEXTSKELETON
#undef TEXTINDEX
#undef TEXTWHITE
#undef TEXTLINE
#undef TEXTCOLOR
#undef TEXTCHAR
#undef TEXTWORD
	return i;
}

// inverse of text_visible
void text_posf(const char* str, int cursor, float& cx, float& cy, int maxwidth) {
#define TEXTINDEX(idx)   \
	if (idx == cursor) { \
		cx = x;          \
		cy = y;          \
		break;           \
	}
#define TEXTWHITE(idx)
#define TEXTLINE(idx)
#define TEXTCOLOR(idx)
#define TEXTCHAR(idx) x += cw;
#define TEXTWORD TEXTWORDSKELETON if (i >= cursor) break;
	cx = cy = 0;
	TEXTSKELETON
	TEXTEND(cursor)
#undef TEXTINDEX
#undef TEXTWHITE
#undef TEXTLINE
#undef TEXTCOLOR
#undef TEXTCHAR
#undef TEXTWORD
}

void text_boundsf(const char* str, float& width, float& height, int maxwidth) {
#define TEXTINDEX(idx)
#define TEXTWHITE(idx)
#define TEXTLINE(idx) \
	if (x > width)    \
		width = x;
#define TEXTCOLOR(idx)
#define TEXTCHAR(idx) x += cw;
#define TEXTWORD x += w;
	width = 0;
	TEXTSKELETON
	height = y + FONTH;
	TEXTLINE(_)
#undef TEXTINDEX
#undef TEXTWHITE
#undef TEXTLINE
#undef TEXTCOLOR
#undef TEXTCHAR
#undef TEXTWORD
}

Shader* textshader = nullptr;

void draw_text(const char* str, float left, float top, int r, int g, int b, int a, int cursor, int maxwidth) {
#define TEXTINDEX(idx)   \
	if (idx == cursor) { \
		cx = x;          \
		cy = y;          \
	}
#define TEXTWHITE(idx)
#define TEXTLINE(idx)
#define TEXTCOLOR(idx) \
	if (usecolor)      \
		text_color(str[idx], colorstack, sizeof(colorstack), colorpos, color, a);
#define TEXTCHAR(idx)                            \
	draw_char(tex, c, left + x, top + y, scale); \
	x += cw;
#define TEXTWORD TEXTWORDSKELETON
	char colorstack[10];
	colorstack[0] = '\0';  // indicate user color
	bvec color(r, g, b);
	if (textbright != 100)
		color.scale(textbright, 100);
	int colorpos = 0;
	float cx = -FONTW, cy = 0;
	bool usecolor = true;
	if (a < 0) {
		usecolor = false;
		a = -a;
	}
	Texture* tex = curfont->texs[0];
	(textshader ? textshader : hudtextshader)->set();
	LOCALPARAMF(textparams, curfont->bordermin, curfont->bordermax, curfont->outlinemin, curfont->outlinemax);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, tex->id);
	gle::color(color, a);
	gle::defvertex(textmatrix ? 3 : 2);
	gle::deftexcoord0();
	gle::begin(GL_QUADS);
	TEXTSKELETON
	TEXTEND(cursor)
	xtraverts += gle::end();
	if (cursor >= 0 && (totalmillis / 250) & 1) {
		gle::color(color, a);
		if (maxwidth >= 0 && cx >= maxwidth && cx > 0) {
			cx = 0;
			cy += FONTH;
		}
		draw_char(tex, '_', left + cx, top + cy, scale);
		xtraverts += gle::end();
	}
#undef TEXTINDEX
#undef TEXTWHITE
#undef TEXTLINE
#undef TEXTCOLOR
#undef TEXTCHAR
#undef TEXTWORD
}

void reloadfonts() {
	enumerate(fonts, font, f, loopv(f.texs) if (!reloadtexture(*f.texs[i])) fatal("failed to reload font texture"););
}
