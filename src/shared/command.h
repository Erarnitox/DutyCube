// script binding functionality

enum VALUES {
	VAL_NULL = 0,
	VAL_INT,
	VAL_FLOAT,
	VAL_STR,
	VAL_ANY,
	VAL_CODE,
	VAL_MACRO,
	VAL_IDENT,
	VAL_CSTR,
	VAL_CANY,
	VAL_WORD,
	VAL_POP,
	VAL_COND
};

enum CODE {
	CODE_START = 0,
	CODE_OFFSET,
	CODE_NULL,
	CODE_TRUE,
	CODE_FALSE,
	CODE_NOT,
	CODE_POP,
	CODE_ENTER,
	CODE_ENTER_RESULT,
	CODE_EXIT,
	CODE_RESULT_ARG,
	CODE_VAL,
	CODE_VALI,
	CODE_DUP,
	CODE_MACRO,
	CODE_BOOL,
	CODE_BLOCK,
	CODE_EMPTY,
	CODE_COMPILE,
	CODE_COND,
	CODE_FORCE,
	CODE_RESULT,
	CODE_IDENT,
	CODE_IDENTU,
	CODE_IDENTARG,
	CODE_COM,
	CODE_COMD,
	CODE_COMC,
	CODE_COMV,
	CODE_CONC,
	CODE_CONCW,
	CODE_CONCM,
	CODE_DOWN,
	CODE_SVAR,
	CODE_SVARM,
	CODE_SVAR1,
	CODE_IVAR,
	CODE_IVAR1,
	CODE_IVAR2,
	CODE_IVAR3,
	CODE_FVAR,
	CODE_FVAR1,
	CODE_LOOKUP,
	CODE_LOOKUPU,
	CODE_LOOKUPARG,
	CODE_LOOKUPM,
	CODE_LOOKUPMU,
	CODE_LOOKUPMARG,
	CODE_ALIAS,
	CODE_ALIASU,
	CODE_ALIASARG,
	CODE_CALL,
	CODE_CALLU,
	CODE_CALLARG,
	CODE_PRINT,
	CODE_LOCAL,
	CODE_DO,
	CODE_DOARGS,
	CODE_JUMP,
	CODE_JUMP_TRUE,
	CODE_JUMP_FALSE,
	CODE_JUMP_RESULT_TRUE,
	CODE_JUMP_RESULT_FALSE,

	CODE_OP_MASK = 0x3F,
	CODE_RET = 6,
	CODE_RET_MASK = 0xC0,

	/* return type flags */
	RET_NULL = VAL_NULL << CODE_RET,
	RET_STR = VAL_STR << CODE_RET,
	RET_INT = VAL_INT << CODE_RET,
	RET_FLOAT = VAL_FLOAT << CODE_RET,
};

enum ID {
	ID_VAR,
	ID_FVAR,
	ID_SVAR,
	ID_COMMAND,
	ID_ALIAS,
	ID_LOCAL,
	ID_DO,
	ID_DOARGS,
	ID_IF,
	ID_RESULT,
	ID_NOT,
	ID_AND,
	ID_OR
};

enum IDF {
	IDF_PERSIST = 1 << 0,
	IDF_OVERRIDE = 1 << 1,
	IDF_HEX = 1 << 2,
	IDF_READONLY = 1 << 3,
	IDF_OVERRIDDEN = 1 << 4,
	IDF_UNKNOWN = 1 << 5,
	IDF_ARG = 1 << 6
};

struct ident;

struct identval {
	union {
		int i;	// ID_VAR, VAL_INT
		float f;  // ID_FVAR, VAL_FLOAT
		char* s;  // ID_SVAR, VAL_STR
		const uint* code;  // VAL_CODE
		ident* id;	// VAL_IDENT
		const char* cstr;  // VAL_CSTR
	};
};

struct tagval : identval {
	int type;

	void setint(int val) {
		type = VAL_INT;
		i = val;
	}
	void setfloat(float val) {
		type = VAL_FLOAT;
		f = val;
	}
	void setnumber(double val) {
		i = static_cast<int>(val);
		if (val == i) {
			type = VAL_INT;
		} else {
			type = VAL_FLOAT;
			f = val;
		}
	}
	void setstr(char* val) {
		type = VAL_STR;
		s = val;
	}
	void setnull() {
		type = VAL_NULL;
		i = 0;
	}
	void setcode(const uint* val) {
		type = VAL_CODE;
		code = val;
	}
	void setmacro(const uint* val) {
		type = VAL_MACRO;
		code = val;
	}
	void setcstr(const char* val) {
		type = VAL_CSTR;
		cstr = val;
	}
	void setident(ident* val) {
		type = VAL_IDENT;
		id = val;
	}

	const char* getstr() const;
	int getint() const;
	float getfloat() const;
	double getnumber() const;
	bool getbool() const;
	void getval(tagval& r) const;

	void cleanup();
};

struct identstack {
	identval val;
	int valtype;
	identstack* next;
};

union identvalptr {
	void* p;  // ID_*VAR
	int* i;	 // ID_VAR
	float* f;  // ID_FVAR
	char** s;  // ID_SVAR
};

typedef void(__cdecl* identfun)(ident* id);

struct ident {
	uchar type;	 // one of ID_* above
	union {
		uchar valtype;	// ID_ALIAS
		uchar numargs;	// ID_COMMAND
	};
	ushort flags;
	int index;
	const char* name;
	union {
		struct	// ID_VAR, ID_FVAR, ID_SVAR
		{
			union {
				struct {
					int minval, maxval;
				};	// ID_VAR
				struct {
					float minvalf, maxvalf;
				};	// ID_FVAR
			};
			identvalptr storage;
			identval overrideval;
		};
		struct	// ID_ALIAS
		{
			uint* code;
			identval val;
			identstack* stack;
		};
		struct	// ID_COMMAND
		{
			const char* args;
			uint argmask;
		};
	};
	identfun fun;  // ID_VAR, ID_FVAR, ID_SVAR, ID_COMMAND

	ident() {
	}
	// ID_VAR
	ident(int t, const char* n, int m, int x, int* s, void* f = NULL, int flags = 0)
		: type(t), flags(flags | (m > x ? IDF_READONLY : 0)), name(n), minval(m), maxval(x), fun((identfun)f) {
		storage.i = s;
	}
	// ID_FVAR
	ident(int t, const char* n, float m, float x, float* s, void* f = NULL, int flags = 0)
		: type(t), flags(flags | (m > x ? IDF_READONLY : 0)), name(n), minvalf(m), maxvalf(x), fun((identfun)f) {
		storage.f = s;
	}
	// ID_SVAR
	ident(int t, const char* n, char** s, void* f = NULL, int flags = 0)
		: type(t), flags(flags), name(n), fun((identfun)f) {
		storage.s = s;
	}
	// ID_ALIAS
	ident(int t, const char* n, char* a, int flags)
		: type(t), valtype(VAL_STR), flags(flags), name(n), code(NULL), stack(NULL) {
		val.s = a;
	}
	ident(int t, const char* n, int a, int flags)
		: type(t), valtype(VAL_INT), flags(flags), name(n), code(NULL), stack(NULL) {
		val.i = a;
	}
	ident(int t, const char* n, float a, int flags)
		: type(t), valtype(VAL_FLOAT), flags(flags), name(n), code(NULL), stack(NULL) {
		val.f = a;
	}
	ident(int t, const char* n, int flags)
		: type(t), valtype(VAL_NULL), flags(flags), name(n), code(NULL), stack(NULL) {
	}
	ident(int t, const char* n, const tagval& v, int flags)
		: type(t), valtype(v.type), flags(flags), name(n), code(NULL), stack(NULL) {
		val = v;
	}
	// ID_COMMAND
	ident(int t, const char* n, const char* args, uint argmask, int numargs, void* f = NULL, int flags = 0)
		: type(t), numargs(numargs), flags(flags), name(n), args(args), argmask(argmask), fun((identfun)f) {
	}

	void changed() {
		if (fun) {
			fun(this);
		}
	}

	void setval(const tagval& v) {
		valtype = v.type;
		val = v;
	}

	void setval(const identstack& v) {
		valtype = v.valtype;
		val = v.val;
	}

	void forcenull() {
		if (valtype == VAL_STR) {
			delete[] val.s;
		}
		valtype = VAL_NULL;
	}

	float getfloat() const;
	int getint() const;
	double getnumber() const;
	const char* getstr() const;
	void getval(tagval& r) const;
	void getcstr(tagval& v) const;
	void getcval(tagval& v) const;
};

extern void addident(ident* id);

extern tagval* commandret;
extern const char* intstr(int v);
extern void intret(int v);
extern const char* floatstr(float v);
extern void floatret(float v);
extern const char* numberstr(double v);
extern void numberret(double v);
extern void stringret(char* s);
extern void result(tagval& v);
extern void result(const char* s);

static inline int parseint(const char* s) {
	return int(strtoul(s, NULL, 0));
}

#define PARSEFLOAT(name, type)                                                                  \
	static inline type parse##name(const char* s) {                                             \
		/* not all platforms (windows) can parse hexadecimal integers via strtod */             \
		char* end;                                                                              \
		double val = strtod(s, &end);                                                           \
		return val || end == s || (*end != 'x' && *end != 'X') ? type(val) : type(parseint(s)); \
	}
PARSEFLOAT(float, float)
PARSEFLOAT(number, double)

static inline void intformat(char* buf, int v, int len = 20) {
	nformatstring(buf, len, "%d", v);
}
static inline void floatformat(char* buf, float v, int len = 20) {
	nformatstring(buf, len, v == int(v) ? "%.1f" : "%.7g", v);
}
static inline void numberformat(char* buf, double v, int len = 20) {
	int i = int(v);
	if (v == i) {
		nformatstring(buf, len, "%d", i);
	} else {
		nformatstring(buf, len, "%.7g", v);
	}
}

static inline const char* getstr(const identval& v, int type) {
	switch (type) {
	case VAL_STR:
	case VAL_MACRO:
	case VAL_CSTR:
		return v.s;
	case VAL_INT:
		return intstr(v.i);
	case VAL_FLOAT:
		return floatstr(v.f);
	default:
		return "";
	}
}
inline const char* tagval::getstr() const {
	return ::getstr(*this, type);
}
inline const char* ident::getstr() const {
	return ::getstr(val, valtype);
}

#define GETNUMBER(name, ret)                                   \
	static inline ret get##name(const identval& v, int type) { \
		switch (type) {                                        \
		case VAL_FLOAT:                                        \
			return ret(v.f);                                   \
		case VAL_INT:                                          \
			return ret(v.i);                                   \
		case VAL_STR:                                          \
		case VAL_MACRO:                                        \
		case VAL_CSTR:                                         \
			return parse##name(v.s);                           \
		default:                                               \
			return ret(0);                                     \
		}                                                      \
	}                                                          \
	inline ret tagval::get##name() const {                     \
		return ::get##name(*this, type);                       \
	}                                                          \
	inline ret ident::get##name() const {                      \
		return ::get##name(val, valtype);                      \
	}
GETNUMBER(int, int)
GETNUMBER(float, float)
GETNUMBER(number, double)

static inline void getval(const identval& v, int type, tagval& r) {
	switch (type) {
	case VAL_STR:
	case VAL_MACRO:
	case VAL_CSTR:
		r.setstr(newstring(v.s));
		break;
	case VAL_INT:
		r.setint(v.i);
		break;
	case VAL_FLOAT:
		r.setfloat(v.f);
		break;
	default:
		r.setnull();
		break;
	}
}

inline void tagval::getval(tagval& r) const {
	::getval(*this, type, r);
}
inline void ident::getval(tagval& r) const {
	::getval(val, valtype, r);
}

inline void ident::getcstr(tagval& v) const {
	switch (valtype) {
	case VAL_MACRO:
		v.setmacro(val.code);
		break;
	case VAL_STR:
	case VAL_CSTR:
		v.setcstr(val.s);
		break;
	case VAL_INT:
		v.setstr(newstring(intstr(val.i)));
		break;
	case VAL_FLOAT:
		v.setstr(newstring(floatstr(val.f)));
		break;
	default:
		v.setcstr("");
		break;
	}
}

inline void ident::getcval(tagval& v) const {
	switch (valtype) {
	case VAL_MACRO:
		v.setmacro(val.code);
		break;
	case VAL_STR:
	case VAL_CSTR:
		v.setcstr(val.s);
		break;
	case VAL_INT:
		v.setint(val.i);
		break;
	case VAL_FLOAT:
		v.setfloat(val.f);
		break;
	default:
		v.setnull();
		break;
	}
}

// nasty macros for registering script functions, abuses globals to avoid excessive infrastructure
#define KEYWORD(name, type) UNUSED static bool __dummy_##type = addcommand(#name, (identfun)NULL, NULL, type)
#define COMMANDKN(name, type, fun, nargs) \
	UNUSED static bool __dummy_##fun = addcommand(#name, (identfun)fun, nargs, type)
#define COMMANDK(name, type, nargs) COMMANDKN(name, type, name, nargs)
#define COMMANDN(name, fun, nargs) COMMANDKN(name, ID_COMMAND, fun, nargs)
#define COMMAND(name, nargs) COMMANDN(name, name, nargs)

#define _VAR(name, global, min, cur, max, persist) int global = variable(#name, min, cur, max, &global, NULL, persist)
#define VARN(name, global, min, cur, max) _VAR(name, global, min, cur, max, 0)
#define VARNP(name, global, min, cur, max) _VAR(name, global, min, cur, max, IDF_PERSIST)
#define VARNR(name, global, min, cur, max) _VAR(name, global, min, cur, max, IDF_OVERRIDE)
#define VAR(name, min, cur, max) _VAR(name, name, min, cur, max, 0)
#define VARP(name, min, cur, max) _VAR(name, name, min, cur, max, IDF_PERSIST)
#define VARR(name, min, cur, max) _VAR(name, name, min, cur, max, IDF_OVERRIDE)
#define _VARF(name, global, min, cur, max, body, persist)                      \
	void var_##name(ident* id);                                                \
	int global = variable(#name, min, cur, max, &global, var_##name, persist); \
	void var_##name(ident* id) {                                               \
		body;                                                                  \
	}
#define VARFN(name, global, min, cur, max, body) _VARF(name, global, min, cur, max, body, 0)
#define VARF(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, 0)
#define VARFP(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, IDF_PERSIST)
#define VARFR(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, IDF_OVERRIDE)
#define VARFNP(name, global, min, cur, max, body) _VARF(name, global, min, cur, max, body, IDF_PERSIST)
#define VARFNR(name, global, min, cur, max, body) _VARF(name, global, min, cur, max, body, IDF_OVERRIDE)
#define _VARM(name, min, cur, max, scale, persist) \
	int name = cur * scale;                        \
	_VARF(                                         \
		name, _##name, min, cur, max, { name = _##name * scale; }, persist)
#define VARMP(name, min, cur, max, scale) _VARM(name, min, cur, max, scale, IDF_PERSIST)
#define VARMR(name, min, cur, max, scale) _VARM(name, min, cur, max, scale, IDF_OVERRIDE)

#define _HVAR(name, global, min, cur, max, persist) \
	int global = variable(#name, min, cur, max, &global, NULL, persist | IDF_HEX)
#define HVARN(name, global, min, cur, max) _HVAR(name, global, min, cur, max, 0)
#define HVARNP(name, global, min, cur, max) _HVAR(name, global, min, cur, max, IDF_PERSIST)
#define HVARNR(name, global, min, cur, max) _HVAR(name, global, min, cur, max, IDF_OVERRIDE)
#define HVAR(name, min, cur, max) _HVAR(name, name, min, cur, max, 0)
#define HVARP(name, min, cur, max) _HVAR(name, name, min, cur, max, IDF_PERSIST)
#define HVARR(name, min, cur, max) _HVAR(name, name, min, cur, max, IDF_OVERRIDE)
#define _HVARF(name, global, min, cur, max, body, persist)                               \
	void var_##name(ident* id);                                                          \
	int global = variable(#name, min, cur, max, &global, var_##name, persist | IDF_HEX); \
	void var_##name(ident* id) {                                                         \
		body;                                                                            \
	}
#define HVARFN(name, global, min, cur, max, body) _HVARF(name, global, min, cur, max, body, 0)
#define HVARF(name, min, cur, max, body) _HVARF(name, name, min, cur, max, body, 0)
#define HVARFP(name, min, cur, max, body) _HVARF(name, name, min, cur, max, body, IDF_PERSIST)
#define HVARFR(name, min, cur, max, body) _HVARF(name, name, min, cur, max, body, IDF_OVERRIDE)
#define HVARFNP(name, global, min, cur, max, body) _HVARF(name, global, min, cur, max, body, IDF_PERSIST)
#define HVARFNR(name, global, min, cur, max, body) _HVARF(name, global, min, cur, max, body, IDF_OVERRIDE)

#define _CVAR(name, cur, init, body, persist) \
	bvec name = bvec::hexcolor(cur);          \
	_HVARF(                                   \
		name,                                 \
		_##name,                              \
		0,                                    \
		cur,                                  \
		0xFFFFFF,                             \
		{                                     \
			init;                             \
			name = bvec::hexcolor(_##name);   \
			body;                             \
		},                                    \
		persist)
#define CVARP(name, cur) _CVAR(name, cur, , , IDF_PERSIST)
#define CVARR(name, cur) _CVAR(name, cur, , , IDF_OVERRIDE)
#define CVARFP(name, cur, body) _CVAR(name, cur, , body, IDF_PERSIST)
#define CVARFR(name, cur, body) _CVAR(name, cur, , body, IDF_OVERRIDE)
#define _CVAR0(name, cur, body, persist) \
	_CVAR(                               \
		name,                            \
		cur,                             \
		{                                \
			if (!_##name) {              \
				_##name = cur;           \
			}                            \
		},                               \
		body,                            \
		persist)
#define CVAR0P(name, cur) _CVAR0(name, cur, , IDF_PERSIST)
#define CVAR0R(name, cur) _CVAR0(name, cur, , IDF_OVERRIDE)
#define CVAR0FP(name, cur, body) _CVAR0(name, cur, body, IDF_PERSIST)
#define CVAR0FR(name, cur, body) _CVAR0(name, cur, body, IDF_OVERRIDE)
#define _CVAR1(name, cur, body, persist)                     \
	_CVAR(                                                   \
		name,                                                \
		cur,                                                 \
		{                                                    \
			if (_##name <= 255) {                            \
				_##name |= (_##name << 8) | (_##name << 16); \
			}                                                \
		},                                                   \
		body,                                                \
		persist)
#define CVAR1P(name, cur) _CVAR1(name, cur, , IDF_PERSIST)
#define CVAR1R(name, cur) _CVAR1(name, cur, , IDF_OVERRIDE)
#define CVAR1FP(name, cur, body) _CVAR1(name, cur, body, IDF_PERSIST)
#define CVAR1FR(name, cur, body) _CVAR1(name, cur, body, IDF_OVERRIDE)

#define _FVAR(name, global, min, cur, max, persist) \
	float global = fvariable(#name, min, cur, max, &global, NULL, persist)
#define FVARN(name, global, min, cur, max) _FVAR(name, global, min, cur, max, 0)
#define FVARNP(name, global, min, cur, max) _FVAR(name, global, min, cur, max, IDF_PERSIST)
#define FVARNR(name, global, min, cur, max) _FVAR(name, global, min, cur, max, IDF_OVERRIDE)
#define FVAR(name, min, cur, max) _FVAR(name, name, min, cur, max, 0)
#define FVARP(name, min, cur, max) _FVAR(name, name, min, cur, max, IDF_PERSIST)
#define FVARR(name, min, cur, max) _FVAR(name, name, min, cur, max, IDF_OVERRIDE)
#define _FVARF(name, global, min, cur, max, body, persist)                        \
	void var_##name(ident* id);                                                   \
	float global = fvariable(#name, min, cur, max, &global, var_##name, persist); \
	void var_##name(ident* id) {                                                  \
		body;                                                                     \
	}
#define FVARFN(name, global, min, cur, max, body) _FVARF(name, global, min, cur, max, body, 0)
#define FVARF(name, min, cur, max, body) _FVARF(name, name, min, cur, max, body, 0)
#define FVARFP(name, min, cur, max, body) _FVARF(name, name, min, cur, max, body, IDF_PERSIST)
#define FVARFR(name, min, cur, max, body) _FVARF(name, name, min, cur, max, body, IDF_OVERRIDE)
#define FVARFNP(name, global, min, cur, max, body) _FVARF(name, global, min, cur, max, body, IDF_PERSIST)
#define FVARFNR(name, global, min, cur, max, body) _FVARF(name, global, min, cur, max, body, IDF_OVERRIDE)

#define _SVAR(name, global, cur, persist) char* global = svariable(#name, cur, &global, NULL, persist)
#define SVARN(name, global, cur) _SVAR(name, global, cur, 0)
#define SVARNP(name, global, cur) _SVAR(name, global, cur, IDF_PERSIST)
#define SVARNR(name, global, cur) _SVAR(name, global, cur, IDF_OVERRIDE)
#define SVAR(name, cur) _SVAR(name, name, cur, 0)
#define SVARP(name, cur) _SVAR(name, name, cur, IDF_PERSIST)
#define SVARR(name, cur) _SVAR(name, name, cur, IDF_OVERRIDE)
#define _SVARF(name, global, cur, body, persist)                        \
	void var_##name(ident* id);                                         \
	char* global = svariable(#name, cur, &global, var_##name, persist); \
	void var_##name(ident* id) {                                        \
		body;                                                           \
	}
#define SVARFN(name, global, cur, body) _SVARF(name, global, cur, body, 0)
#define SVARF(name, cur, body) _SVARF(name, name, cur, body, 0)
#define SVARFP(name, cur, body) _SVARF(name, name, cur, body, IDF_PERSIST)
#define SVARFR(name, cur, body) _SVARF(name, name, cur, body, IDF_OVERRIDE)
#define SVARFNP(name, global, cur, body) _SVARF(name, global, cur, body, IDF_PERSIST)
#define SVARFNR(name, global, cur, body) _SVARF(name, global, cur, body, IDF_OVERRIDE)

// anonymous inline commands, uses nasty template trick with line numbers to keep names unique
#define ICOMMANDNAME(name) _icmd_##name
#define ICOMMANDSNAME _icmds_
#define ICOMMANDKNS(name, type, cmdname, nargs, proto, b)                                           \
	template <int N>                                                                                \
	struct cmdname;                                                                                 \
	template <>                                                                                     \
	struct cmdname<__LINE__> {                                                                      \
		static bool init;                                                                           \
		static void run proto;                                                                      \
	};                                                                                              \
	bool cmdname<__LINE__>::init = addcommand(name, (identfun)cmdname<__LINE__>::run, nargs, type); \
	void cmdname<__LINE__>::run proto {                                                             \
		b;                                                                                          \
	}
#define ICOMMANDKN(name, type, cmdname, nargs, proto, b) ICOMMANDKNS(#name, type, cmdname, nargs, proto, b)
#define ICOMMANDK(name, type, nargs, proto, b) ICOMMANDKN(name, type, ICOMMANDNAME(name), nargs, proto, b)
#define ICOMMANDKS(name, type, nargs, proto, b) ICOMMANDKNS(name, type, ICOMMANDSNAME, nargs, proto, b)
#define ICOMMANDNS(name, cmdname, nargs, proto, b) ICOMMANDKNS(name, ID_COMMAND, cmdname, nargs, proto, b)
#define ICOMMANDN(name, cmdname, nargs, proto, b) ICOMMANDNS(#name, cmdname, nargs, proto, b)
#define ICOMMAND(name, nargs, proto, b) ICOMMANDN(name, ICOMMANDNAME(name), nargs, proto, b)
#define ICOMMANDS(name, nargs, proto, b) ICOMMANDNS(name, ICOMMANDSNAME, nargs, proto, b)
