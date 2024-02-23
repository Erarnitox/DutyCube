#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "shared/cube.h"
#include "engine/world.h"

#ifndef STANDALONE

#include "engine/octa.h"
#include "engine/light.h"
#include "engine/texture.h"
#include "engine/bih.h"
#include "engine/model.h"

extern dynent *player;
extern physent *camera1;  // Special ent that acts as camera, same object as player1 in FPS mode.

extern int worldscale;
extern int worldsize;
extern int mapversion;
extern char *maptitle;
extern vector<ushort> texmru;
extern int xtraverts;
extern int xtravertsva;
extern const ivec cubecoords[8];
extern const ivec facecoords[6][4];
extern const uchar fv[6][4];
extern const uchar fvmasks[64];
extern const uchar faceedgesidx[6][4];
extern bool inbetweenframes;
extern bool renderedframe;

extern SDL_Window *screen;
extern int screenw;
extern int screenh;
extern int renderw;
extern int renderh;
extern int hudw;
extern int hudh;
extern float spread;
extern float weaponspread;

extern vector<int> entgroup;

// rendertext
struct font
{
    struct charinfo
    {
        float x, y, w, h, offsetx, offsety, advance;
        int tex;
    };

    char *name{nullptr};
    vector<Texture *> texs;
    vector<charinfo> chars;
    int charoffset, defaultw, defaulth, scale;
    float bordermin, bordermax, outlinemin, outlinemax;

    font()  
    = default;
    ~font()
    {
        DELETEA(name);
    }
};

#define FONTH (curfont->scale)
#define FONTW (FONTH / 2)
#define MINRESW 640
#define MINRESH 480

extern font *curfont;
extern Shader *textshader;
extern const matrix4x3 *textmatrix;
extern float textscale;

extern auto findfont(const char *name) -> font *;
extern void reloadfonts();

static inline void setfont(font *f)
{
    if (f)
    {
        curfont = f;
    }
}

// texture
extern int hwtexsize;
extern int hwcubetexsize;
extern int hwmaxaniso;
extern int maxtexsize;
extern int hwtexunits;
extern int hwvtexunits;

extern auto textureload(const char *name, int clamp = 0, bool mipit = true, bool msg = true) -> Texture *;
extern auto texalign(const void *data, int w, int bpp) -> int;
extern auto floatformat(GLenum format) -> bool;
extern void cleanuptexture(Texture *t);
extern auto loadalphamask(Texture *t) -> uchar *;
extern auto cubemapload(const char *name, bool mipit = true, bool msg = true, bool transient = false) -> Texture *;
extern void drawcubemap(int size, const vec &o, float yaw, float pitch, const cubemapside &side, bool onlysky = false);
extern void loadshaders();
extern void setuptexparameters(int tnum,
                               const void *pixels,
                               int clamp,
                               int filter,
                               GLenum format = GL_RGB,
                               GLenum target = GL_TEXTURE_2D,
                               bool swizzle = false);
extern void createtexture(int tnum,
                          int w,
                          int h,
                          const void *pixels,
                          int clamp,
                          int filter,
                          GLenum component = GL_RGB,
                          GLenum target = GL_TEXTURE_2D,
                          int pw = 0,
                          int ph = 0,
                          int pitch = 0,
                          bool resize = true,
                          GLenum format = GL_FALSE,
                          bool swizzle = false);
extern void create3dtexture(int tnum,
                            int w,
                            int h,
                            int d,
                            const void *pixels,
                            int clamp,
                            int filter,
                            GLenum component = GL_RGB,
                            GLenum target = GL_TEXTURE_3D,
                            bool swizzle = false);
extern void blurtexture(int n, int bpp, int w, int h, uchar *dst, const uchar *src, int margin = 0);
extern void blurnormals(int n, int w, int h, bvec *dst, const bvec *src, int margin = 0);
extern auto setuppostfx(int w, int h, GLuint outfbo = 0) -> GLuint;
extern void cleanuppostfx(bool fullclean = false);
extern void renderpostfx(GLuint outfbo = 0);
extern void initenvmaps();
extern void genenvmaps();
extern auto closestenvmap(const vec &o) -> ushort;
extern auto closestenvmap(int orient, const ivec &o, int size) -> ushort;
extern auto lookupenvmap(ushort emid) -> GLuint;
extern auto lookupenvmap(Slot &slot) -> GLuint;
extern auto reloadtexture(Texture &tex) -> bool;
extern auto reloadtexture(const char *name) -> bool;
extern void setuptexcompress();
extern void clearslots();
extern void compacteditvslots();
extern void compactmruvslots();
extern void compactvslots(cube *c, int n = 8);
extern void compactvslot(int &index);
extern void compactvslot(VSlot &vs);
extern auto compactvslots(bool cull = false) -> int;
extern void reloadtextures();
extern void cleanuptextures();

// pvs
extern void clearpvs();
extern auto pvsoccluded(const ivec &bbmin, const ivec &bbmax) -> bool;
extern auto pvsoccludedsphere(const vec &center, float radius) -> bool;
extern auto waterpvsoccluded(int height) -> bool;
extern void setviewcell(const vec &p);
extern void savepvs(stream *f);
extern void loadpvs(stream *f, int numpvs);
extern auto getnumviewcells() -> int;

static inline auto pvsoccluded(const ivec &bborigin, int size) -> bool
{
    return pvsoccluded(bborigin, ivec(bborigin).add(size));
}

// rendergl
extern bool hasVAO;
extern bool hasTR;
extern bool hasTSW;
extern bool hasPBO;
extern bool hasFBO;
extern bool hasAFBO;
extern bool hasDS;
extern bool hasTF;
extern bool hasCBF;
extern bool hasS3TC;
extern bool hasFXT1;
extern bool hasLATC;
extern bool hasRGTC;
extern bool hasAF;
extern bool hasFBB;
extern bool hasFBMS;
extern bool hasTMS;
extern bool hasMSS;
extern bool hasFBMSBS;
extern bool hasUBO;
extern bool hasMBR;
extern bool hasDB2;
extern bool hasDBB;
extern bool hasTG;
extern bool hasTQ;
extern bool hasPF;
extern bool hasTRG;
extern bool hasTI;
extern bool hasHFV;
extern bool hasHFP;
extern bool hasDBT;
extern bool hasDC;
extern bool hasDBGO;
extern bool hasEGPU4;
extern bool hasGPU4;
extern bool hasGPU5;
extern bool hasBFE;
extern bool hasEAL;
extern bool hasCR;
extern bool hasOQ2;
extern bool hasES3;
extern bool hasCB;
extern bool hasCI;
extern int glversion;
extern int glslversion;
extern int glcompat;
extern int maxdrawbufs;
extern int maxdualdrawbufs;

enum
{
    DRAWTEX_NONE = 0,
    DRAWTEX_ENVMAP,
    DRAWTEX_MINIMAP,
    DRAWTEX_MODELPREVIEW
};

extern int vieww;
extern int viewh;
extern int fov;
extern float curfov;
extern float fovy;
extern float aspect;
extern float forceaspect;
extern float nearplane;
extern int farplane;
extern bool hdrfloat;
extern float ldrscale;
extern float ldrscaleb;
extern int drawtex;
extern const matrix4 viewmatrix;
extern const matrix4 invviewmatrix;
extern matrix4 cammatrix;
extern matrix4 projmatrix;
extern matrix4 camprojmatrix;
extern matrix4 invcammatrix;
extern matrix4 invcamprojmatrix;
extern matrix4 invprojmatrix;
extern int fog;
extern bvec fogcolour;
extern vec curfogcolor;
extern int wireframe;

extern int glerr;
extern void glerror(const char *file, int line, GLenum error);

#define GLERROR                                     \
    do                                              \
    {                                               \
        if (glerr)                                  \
        {                                           \
            GLenum error = glGetError();            \
            if (error != GL_NO_ERROR)               \
            {                                       \
                glerror(__FILE__, __LINE__, error); \
            }                                       \
        }                                           \
    } while (0)

extern void gl_checkextensions();
extern void gl_init();
extern void gl_resize();
extern void gl_drawview();
extern void gl_drawmainmenu();
extern void gl_drawhud();
extern void gl_setupframe(bool force = false);
extern void gl_drawframe();
extern void cleanupgl();
extern void drawminimap();
extern void enablepolygonoffset(GLenum type);
extern void disablepolygonoffset(GLenum type);
extern auto calcspherescissor(const vec &center,
                              float size,
                              float &sx1,
                              float &sy1,
                              float &sx2,
                              float &sy2,
                              float &sz1,
                              float &sz2) -> bool;
extern auto calcbbscissor(const ivec &bbmin, const ivec &bbmax, float &sx1, float &sy1, float &sx2, float &sy2) -> bool;
extern auto calcspotscissor(const vec &origin,
                            float radius,
                            const vec &dir,
                            int spot,
                            const vec &spotx,
                            const vec &spoty,
                            float &sx1,
                            float &sy1,
                            float &sx2,
                            float &sy2,
                            float &sz1,
                            float &sz2) -> bool;
extern void screenquad();
extern void screenquad(float sw, float sh);
extern void screenquadflipped(float sw, float sh);
extern void screenquad(float sw, float sh, float sw2, float sh2);
extern void screenquadoffset(float x, float y, float w, float h);
extern void screenquadoffset(float x, float y, float w, float h, float x2, float y2, float w2, float h2);
extern void hudquad(float x, float y, float w, float h, float tx = 0, float ty = 0, float tw = 1, float th = 1);
extern void debugquad(float x, float y, float w, float h, float tx = 0, float ty = 0, float tw = 1, float th = 1);
extern void recomputecamera();
extern auto calcfrustumboundsphere(float nearplane, float farplane, const vec &pos, const vec &view, vec &center) -> float;
extern void setfogcolor(const vec &v);
extern void zerofogcolor();
extern void resetfogcolor();
extern auto calcfogdensity(float dist) -> float;
extern auto calcfogcull() -> float;
extern void writecrosshairs(stream *f);
extern void renderavatar();

namespace modelpreview
{
    extern void start(int x, int y, int w, int h, bool background = true, bool scissor = false);
    extern void end();
}

struct timer;
extern auto begintimer(const char *name, bool gpu = true) -> timer *;
extern void endtimer(timer *t);

// renderextras
extern void render3dbox(vec &o, float tofloor, float toceil, float xradius, float yradius = 0);

// octa
extern auto newcubes(uint face = F_EMPTY, int mat = MAT_AIR) -> cube *;
extern auto growcubeext(cubeext *ext, int maxverts) -> cubeext *;
extern void setcubeext(cube &c, cubeext *ext);
extern auto newcubeext(cube &c, int maxverts = 0, bool init = true) -> cubeext *;
extern void getcubevector(cube &c, int d, int x, int y, int z, ivec &p);
extern void setcubevector(cube &c, int d, int x, int y, int z, const ivec &p);
extern auto familysize(const cube &c) -> int;
extern void freeocta(cube *c);
extern void discardchildren(cube &c, bool fixtex = false, int depth = 0);
extern void optiface(uchar *p, cube &c);
extern void validatec(cube *c, int size = 0);
extern auto isvalidcube(const cube &c) -> bool;
extern ivec lu;
extern int lusize;
extern auto lookupcube(const ivec &to, int tsize = 0, ivec &ro = lu, int &rsize = lusize) -> cube &;
extern const cube *neighbourstack[32];
extern int neighbourdepth;
extern auto neighbourcube(const cube &c,
                                 int orient,
                                 const ivec &co,
                                 int size,
                                 ivec &ro = lu,
                                 int &rsize = lusize) -> const cube &;
extern void resetclipplanes();
extern auto getmippedtexture(const cube &p, int orient) -> int;
extern void forcemip(cube &c, bool fixtex = true);
extern auto subdividecube(cube &c, bool fullcheck = true, bool brighten = true) -> bool;
extern auto faceconvexity(const ivec v[4]) -> int;
extern auto faceconvexity(const ivec v[4], int &vis) -> int;
extern auto faceconvexity(const vertinfo *verts, int numverts, int size) -> int;
extern auto faceconvexity(const cube &c, int orient) -> int;
extern void calcvert(const cube &c, const ivec &co, int size, ivec &vert, int i, bool solid = false);
extern void calcvert(const cube &c, const ivec &co, int size, vec &vert, int i, bool solid = false);
extern auto faceedges(const cube &c, int orient) -> uint;
extern auto collapsedface(const cube &c, int orient) -> bool;
extern auto touchingface(const cube &c, int orient) -> bool;
extern auto flataxisface(const cube &c, int orient) -> bool;
extern auto collideface(const cube &c, int orient) -> bool;
extern auto genclipplane(const cube &c, int i, vec *v, plane *clip) -> int;
extern void genclipplanes(const cube &c, const ivec &co, int size, clipplanes &p, bool collide = true);
extern auto visibleface(const cube &c,
                        int orient,
                        const ivec &co,
                        int size,
                        ushort mat = MAT_AIR,
                        ushort nmat = MAT_AIR,
                        ushort matmask = MATF_VOLUME) -> bool;
extern auto classifyface(const cube &c, int orient, const ivec &co, int size) -> int;
extern auto visibletris(const cube &c,
                       int orient,
                       const ivec &co,
                       int size,
                       ushort nmat = MAT_ALPHA,
                       ushort matmask = MAT_ALPHA) -> int;
extern auto visibleorient(const cube &c, int orient) -> int;
extern void genfaceverts(const cube &c, int orient, ivec v[4]);
extern auto calcmergedsize(int orient, const ivec &co, int size, const vertinfo *verts, int numverts) -> int;
extern void invalidatemerges(cube &c, const ivec &co, int size, bool msg);
extern void calcmerges();
extern auto mergefaces(int orient, facebounds *m, int sz) -> int;
extern void mincubeface(const cube &cu,
                        int orient,
                        const ivec &o,
                        int size,
                        const facebounds &orig,
                        facebounds &cf,
                        ushort nmat = MAT_AIR,
                        ushort matmask = MATF_VOLUME);
extern void remip();

static inline auto ext(cube &c) -> cubeext &
{
    return *(c.ext ? c.ext : newcubeext(c));
}

// renderlights

#define LIGHTTILE_MAXW 16
#define LIGHTTILE_MAXH 16

extern int lighttilealignw;
extern int lighttilealignh;
extern int lighttilevieww;
extern int lighttileviewh;
extern int lighttilew;
extern int lighttileh;

template <class T>
static inline void calctilebounds(float sx1, float sy1, float sx2, float sy2, T &bx1, T &by1, T &bx2, T &by2)
{
    int tx1 = max(int(floor(((sx1 + 1) * 0.5f * vieww) / lighttilealignw)), 0),
        ty1 = max(int(floor(((sy1 + 1) * 0.5f * viewh) / lighttilealignh)), 0),
        tx2 = min(int(ceil(((sx2 + 1) * 0.5f * vieww) / lighttilealignw)), lighttilevieww),
        ty2 = min(int(ceil(((sy2 + 1) * 0.5f * viewh) / lighttilealignh)), lighttileviewh);
    bx1 = T((tx1 * lighttilew) / lighttilevieww);
    by1 = T((ty1 * lighttileh) / lighttileviewh);
    bx2 = T((tx2 * lighttilew + lighttilevieww - 1) / lighttilevieww);
    by2 = T((ty2 * lighttileh + lighttileviewh - 1) / lighttileviewh);
}

static inline void masktiles(uint *tiles, float sx1, float sy1, float sx2, float sy2)
{
    int tx1, ty1, tx2, ty2;
    calctilebounds(sx1, sy1, sx2, sy2, tx1, ty1, tx2, ty2);
    for (int ty = ty1; ty < ty2; ty++)
        tiles[ty] |= ((1 << (tx2 - tx1)) - 1) << tx1;
}

enum
{
    SM_NONE = 0,
    SM_REFLECT,
    SM_CUBEMAP,
    SM_CASCADE,
    SM_SPOT
};

extern int shadowmapping;

extern vec shadoworigin;
extern vec shadowdir;
extern float shadowradius;
extern float shadowbias;
extern int shadowside;
extern int shadowspot;
extern matrix4 shadowmatrix;

extern void loaddeferredlightshaders();
extern void cleardeferredlightshaders();
extern void clearshadowcache();

extern void rendervolumetric();
extern void cleanupvolumetric();

extern void findshadowvas();
extern void findshadowmms();

extern auto calcshadowinfo(const extentity &e, vec &origin, float &radius, vec &spotloc, int &spotangle, float &bias) -> int;
extern auto dynamicshadowvabounds(int mask, vec &bbmin, vec &bbmax) -> int;
extern void rendershadowmapworld();
extern void batchshadowmapmodels(bool skipmesh = false);
extern void rendershadowatlas();
extern void renderrsmgeom(bool dyntex = false);
extern auto useradiancehints() -> bool;
extern void renderradiancehints();
extern void clearradiancehintscache();
extern void cleanuplights();
extern void workinoq();

extern auto calcbbsidemask(const vec &bbmin, const vec &bbmax, const vec &lightpos, float lightradius, float bias) -> int;
extern auto calcspheresidemask(const vec &p, float radius, float bias) -> int;
extern auto calctrisidemask(const vec &p1, const vec &p2, const vec &p3, float bias) -> int;
extern auto cullfrustumsides(const vec &lightpos, float lightradius, float size, float border) -> int;
extern auto calcbbcsmsplits(const ivec &bbmin, const ivec &bbmax) -> int;
extern auto calcspherecsmsplits(const vec &center, float radius) -> int;
extern auto calcbbrsmsplits(const ivec &bbmin, const ivec &bbmax) -> int;
extern auto calcspherersmsplits(const vec &center, float radius) -> int;

static inline auto sphereinsidespot(const vec &dir, int spot, const vec &center, float radius) -> bool
{
    const vec2 &sc = sincos360[spot];
    float cdist = dir.dot(center), cradius = radius + sc.y * cdist;
    return sc.x * sc.x * (center.dot(center) - cdist * cdist) <= cradius * cradius;
}
static inline auto bbinsidespot(const vec &origin, const vec &dir, int spot, const ivec &bbmin, const ivec &bbmax) -> bool
{
    vec radius = vec(ivec(bbmax).sub(bbmin)).mul(0.5f), center = vec(bbmin).add(radius);
    return sphereinsidespot(dir, spot, center.sub(origin), radius.magnitude());
}

extern matrix4 worldmatrix, screenmatrix;

extern int gw;
extern int gh;
extern int gdepthformat;
extern int ghasstencil;
extern GLuint gdepthtex;
extern GLuint gcolortex;
extern GLuint gnormaltex;
extern GLuint gglowtex;
extern GLuint gdepthrb;
extern GLuint gstencilrb;
extern int msaasamples;
extern int msaalight;
extern GLuint msdepthtex;
extern GLuint mscolortex;
extern GLuint msnormaltex;
extern GLuint msglowtex;
extern GLuint msdepthrb;
extern GLuint msstencilrb;
extern vector<vec2> msaapositions;
enum
{
    AA_UNUSED = 0,
    AA_LUMA,
    AA_MASKED,
    AA_SPLIT,
    AA_SPLIT_LUMA,
    AA_SPLIT_MASKED
};

extern void cleanupgbuffer();
extern void initgbuffer();
extern auto usepacknorm() -> bool;
extern void maskgbuffer(const char *mask);
extern void bindgdepth();
extern void preparegbuffer(bool depthclear = true);
extern void rendergbuffer(bool depthclear = true);
extern void shadesky();
extern void shadegbuffer();
extern void shademinimap(const vec &color = vec(-1, -1, -1));
extern void shademodelpreview(int x, int y, int w, int h, bool background = true, bool scissor = false);
extern void rendertransparent();
extern void renderao();
extern void loadhdrshaders(int aa = AA_UNUSED);
extern void processhdr(GLuint outfbo = 0, int aa = AA_UNUSED);
extern void copyhdr(int sw,
                    int sh,
                    GLuint fbo,
                    int dw = 0,
                    int dh = 0,
                    bool flipx = false,
                    bool flipy = false,
                    bool swapxy = false);
extern void setuplights();
extern void setupgbuffer();
extern auto shouldscale() -> GLuint;
extern void doscale(GLuint outfbo = 0);
extern auto debuglights() -> bool;
extern void cleanuplights();

extern int avatarmask;
extern auto useavatarmask() -> bool;
extern void enableavatarmask();
extern void disableavatarmask();

// aa
extern matrix4 nojittermatrix;

extern void setupaa(int w, int h);
extern void jitteraa();
extern auto maskedaa() -> bool;
extern auto multisampledaa() -> bool;
extern void setaavelocityparams(GLenum tmu = GL_TEXTURE0);
extern void setaamask(bool val);
extern void enableaamask(int stencil = 0);
extern void disableaamask();
extern void doaa(GLuint outfbo, void (*resolve)(GLuint, int));
extern auto debugaa() -> bool;
extern void cleanupaa();

// ents
extern auto entname(entity &e) -> char *;
extern auto haveselent() -> bool;
extern auto copyundoents(undoblock *u) -> undoblock *;
extern void pasteundoent(int idx, const entity &ue);
extern void pasteundoents(undoblock *u);

// octaedit
extern void cancelsel();
extern void rendertexturepanel(int w, int h);
extern void addundo(undoblock *u);
extern void commitchanges(bool force = false);
extern void changed(const ivec &bbmin, const ivec &bbmax, bool commit = true);
extern void changed(const block3 &sel, bool commit = true);
extern void rendereditcursor();
extern void tryedit();

extern void renderprefab(const char *name,
                         const vec &o,
                         float yaw,
                         float pitch,
                         float roll,
                         float size = 1,
                         const vec &color = vec(1, 1, 1));
extern void previewprefab(const char *name, const vec &color);
extern void cleanupprefabs();

// octarender
extern ivec worldmin;
extern ivec worldmax;
extern ivec nogimin;
extern ivec nogimax;
extern vector<tjoint> tjoints;
extern vector<vtxarray *> varoot;
extern vector<vtxarray *> valist;

extern auto encodenormal(const vec &n) -> ushort;
extern auto decodenormal(ushort norm) -> vec;
extern void guessnormals(const vec *pos, int numverts, vec *normals);
extern void reduceslope(ivec &n);
extern void findtjoints();
extern void octarender();
extern void allchanged(bool load = false);
extern void clearvas(cube *c);
extern void destroyva(vtxarray *va, bool reparent = true);
extern void updatevabb(vtxarray *va, bool force = false);
extern void updatevabbs(bool force = false);

// renderva

extern int oqfrags;
extern float alphafrontsx1;
extern float alphafrontsx2;
extern float alphafrontsy1;
extern float alphafrontsy2;
extern float alphabacksx1;
extern float alphabacksx2;
extern float alphabacksy1;
extern float alphabacksy2;
extern float alpharefractsx1;
extern float alpharefractsx2;
extern float alpharefractsy1;
extern float alpharefractsy2;
extern uint alphatiles[LIGHTTILE_MAXH];
extern vtxarray *visibleva;

extern void visiblecubes(bool cull = true);
extern void setvfcP(const vec &bbmin = vec(-1, -1, -1), const vec &bbmax = vec(1, 1, 1));
extern void savevfcP();
extern void restorevfcP();
extern void rendergeom();
extern auto findalphavas() -> int;
extern void renderrefractmask();
extern void renderalphageom(int side);
extern void rendermapmodels();
extern void renderoutline();
extern void cleanupva();

extern auto isfoggedsphere(float rad, const vec &cv) -> bool;
extern auto isvisiblesphere(float rad, const vec &cv) -> int;
extern auto isvisiblebb(const ivec &bo, const ivec &br) -> int;
extern auto bboccluded(const ivec &bo, const ivec &br) -> bool;

extern int deferquery;
extern void flipqueries();
extern auto newquery(void *owner) -> occludequery *;
extern void startquery(occludequery *query);
extern void endquery(occludequery *query);
extern auto checkquery(occludequery *query, bool nowait = false) -> bool;
extern void resetqueries();
extern auto getnumqueries() -> int;
extern void startbb(bool mask = true);
extern void endbb(bool mask = true);
extern void drawbb(const ivec &bo, const ivec &br);

extern void renderdecals();

struct shadowmesh;
extern void clearshadowmeshes();
extern void genshadowmeshes();
extern auto findshadowmesh(int idx, extentity &e) -> shadowmesh *;
extern void rendershadowmesh(shadowmesh *m);

// dynlight

extern void updatedynlights();
extern auto finddynlights() -> int;
extern auto getdynlight(int n, vec &o, float &radius, vec &color, vec &dir, int &spot, int &flags) -> bool;

// material

extern float matliquidsx1;
extern float matliquidsy1;
extern float matliquidsx2;
extern float matliquidsy2;
extern float matsolidsx1;
extern float matsolidsy1;
extern float matsolidsx2;
extern float matsolidsy2;
extern float matrefractsx1;
extern float matrefractsy1;
extern float matrefractsx2;
extern float matrefractsy2;
extern uint matliquidtiles[LIGHTTILE_MAXH];
extern uint matsolidtiles[LIGHTTILE_MAXH];
extern vector<materialsurface> editsurfs;
extern vector<materialsurface> glasssurfs[4];
extern vector<materialsurface> watersurfs[4];
extern vector<materialsurface> waterfallsurfs[4];
extern vector<materialsurface> lavasurfs[4];
extern vector<materialsurface> lavafallsurfs[4];
extern const vec matnormals[6];

extern int showmat;

extern auto findmaterial(const char *name) -> int;
extern auto findmaterialname(int mat) -> const char *;
extern auto getmaterialdesc(int mat, const char *prefix = "") -> const char *;
extern void genmatsurfs(const cube &c, const ivec &co, int size, vector<materialsurface> &matsurfs);
extern void calcmatbb(vtxarray *va, const ivec &co, int size, vector<materialsurface> &matsurfs);
extern auto optimizematsurfs(materialsurface *matbuf, int matsurfs) -> int;
extern void setupmaterials(int start = 0, int len = 0);
extern auto findmaterials() -> int;
extern void rendermaterialmask();
extern void renderliquidmaterials();
extern void rendersolidmaterials();
extern void rendereditmaterials();
extern void renderminimapmaterials();
extern auto visiblematerial(const cube &c, int orient, const ivec &co, int size, ushort matmask = MATF_VOLUME) -> int;

// water
extern int vertwater;
extern int waterreflect;
extern int caustics;
extern float watersx1;
extern float watersy1;
extern float watersx2;
extern float watersy2;

#define GETMATIDXVAR(name, var, type) \
    type get##name##var(int mat)      \
    {                                 \
        switch (mat & MATF_INDEX)     \
        {                             \
            default:                  \
            case 0:                   \
                return name##var;     \
            case 1:                   \
                return name##2##var;  \
            case 2:                   \
                return name##3##var;  \
            case 3:                   \
                return name##4##var;  \
        }                             \
    }

extern auto getwatercolour(int mat) -> const bvec &;
extern auto getwaterdeepcolour(int mat) -> const bvec &;
extern auto getwaterdeepfade(int mat) -> const bvec &;
extern auto getwaterrefractcolour(int mat) -> const bvec &;
extern auto getwaterfallcolour(int mat) -> const bvec &;
extern auto getwaterfallrefractcolour(int mat) -> const bvec &;
extern auto getwaterfog(int mat) -> int;
extern auto getwaterdeep(int mat) -> int;
extern auto getwaterspec(int mat) -> int;
extern auto getwaterrefract(int mat) -> float;
extern auto getwaterfallspec(int mat) -> int;
extern auto getwaterfallrefract(int mat) -> float;

extern auto getlavacolour(int mat) -> const bvec &;
extern auto getlavafog(int mat) -> int;
extern auto getlavaglowmin(int mat) -> float;
extern auto getlavaglowmax(int mat) -> float;
extern auto getlavaspec(int mat) -> int;

extern auto getglasscolour(int mat) -> const bvec &;
extern auto getglassrefract(int mat) -> float;
extern auto getglassspec(int mat) -> int;

extern void renderwater();
extern void renderwaterfalls();
extern void renderlava();
extern void renderlava(const materialsurface &m, Texture *tex, float scale);
extern void loadcaustics(bool force = false);
extern void renderwaterfog(int mat, float blend);
extern void preloadwatershaders(bool force = false);

// server
extern vector<const char *> gameargs;

extern void initserver(bool listen, bool dedicated);
extern void cleanupserver();
extern void serverslice(bool dedicated, uint timeout);
extern void updatetime();

extern auto connectmaster(bool wait) -> ENetSocket;
extern void localclienttoserver(int chan, ENetPacket *);
extern void localconnect();
extern auto serveroption(char *opt) -> bool;

// serverbrowser
extern auto resolverwait(const char *name, ENetAddress *address) -> bool;
extern auto connectwithtimeout(ENetSocket sock, const char *hostname, const ENetAddress &address) -> int;
extern void addserver(const char *name, int port = 0, const char *password = nullptr, bool keep = false);
extern void writeservercfg();

// client
extern void localdisconnect(bool cleanup = true);
extern void localservertoclient(int chan, ENetPacket *packet);
extern void connectserv(const char *servername, int port, const char *serverpassword);
extern void abortconnect();
extern void clientkeepalive();

// command
extern hashnameset<ident> idents;
extern int identflags;

extern void clearoverrides();
extern void writecfg(const char *name = nullptr);

extern void checksleep(int millis);
extern void clearsleep(bool clearoverrides = true);

// console
extern float conscale;

extern void processkey(int code, bool isdown);
extern void processtextinput(const char *str, int len);
extern auto rendercommand(float x, float y, float w) -> float;
extern auto renderfullconsole(float w, float h) -> float;
extern auto renderconsole(float w, float h, float abovehud) -> float;
extern void conoutf(const char *s, ...) PRINTFARGS(1, 2);
extern void conoutf(int type, const char *s, ...) PRINTFARGS(2, 3);
extern void resetcomplete();
extern void complete(char *s, int maxlen, const char *cmdprefix);
auto getkeyname(int code) -> const char *;
extern auto addreleaseaction(char *s) -> const char *;
extern auto addreleaseaction(ident *id, int numargs) -> tagval *;
extern void writebinds(stream *f);
extern void writecompletions(stream *f);

// main
enum
{
    NOT_INITING = 0,
    INIT_GAME,
    INIT_LOAD,
    INIT_RESET
};
extern int initing;
extern int numcpus;

enum
{
    CHANGE_GFX = 1 << 0,
    CHANGE_SOUND = 1 << 1,
    CHANGE_SHADERS = 1 << 2
};
extern auto initwarning(const char *desc, int level = INIT_RESET, int type = CHANGE_GFX) -> bool;

extern bool grabinput;
extern bool minimized;

extern auto interceptkey(int sym) -> bool;

extern float loadprogress;
extern void renderbackground(const char *caption = nullptr,
                             Texture *mapshot = nullptr,
                             const char *mapname = nullptr,
                             const char *mapinfo = nullptr,
                             bool force = false);
extern void renderprogress(float bar, const char *text, bool background = false);

extern void getfps(int &fps, int &bestdiff, int &worstdiff);
extern auto getclockmillis() -> int;

enum
{
    KR_CONSOLE = 1 << 0,
    KR_GUI = 1 << 1,
    KR_EDITMODE = 1 << 2
};

extern void keyrepeat(bool on, int mask = ~0);

enum
{
    TI_CONSOLE = 1 << 0,
    TI_GUI = 1 << 1
};

extern void textinput(bool on, int mask = ~0);

// physics
extern void modifyorient(float yaw, float pitch);
extern void mousemove(int dx, int dy);
extern auto pointincube(const clipplanes &p, const vec &v) -> bool;
extern auto overlapsdynent(const vec &o, float radius) -> bool;
extern void rotatebb(vec &center, vec &radius, int yaw, int pitch, int roll = 0);
extern auto shadowray(const vec &o, const vec &ray, float radius, int mode, extentity *t = nullptr) -> float;

// world

extern vector<int> outsideents;

extern void entcancel();
extern void entitiesinoctanodes();
extern void attachentities();
extern void freeoctaentities(cube &c);
extern auto pointinsel(const selinfo &sel, const vec &o) -> bool;

extern void resetmap();
extern void startmap(const char *name);

// rendermodel
struct mapmodelinfo
{
    string name;
    model *m, *collide;
};

extern vector<mapmodelinfo> mapmodels;

extern float transmdlsx1;
extern float transmdlsy1;
extern float transmdlsx2;
extern float transmdlsy2;
extern uint transmdltiles[LIGHTTILE_MAXH];

extern void loadskin(const char *dir, const char *altdir, Texture *&skin, Texture *&masks);
extern void resetmodelbatches();
extern void startmodelquery(occludequery *query);
extern void endmodelquery();
extern void rendershadowmodelbatches(bool dynmodel = true);
extern void shadowmaskbatchedmodels(bool dynshadow = true);
extern void rendermapmodelbatches();
extern void rendermodelbatches();
extern void rendertransparentmodelbatches(int stencil = 0);
extern void rendermapmodel(int idx,
                           int anim,
                           const vec &o,
                           float yaw = 0,
                           float pitch = 0,
                           float roll = 0,
                           int flags = MDL_CULL_VFC | MDL_CULL_DIST,
                           int basetime = 0,
                           float size = 1);
extern void clearbatchedmapmodels();
extern void preloadusedmapmodels(bool msg = false, bool bih = false);
extern auto batcheddynamicmodels() -> int;
extern auto batcheddynamicmodelbounds(int mask, vec &bbmin, vec &bbmax) -> int;
extern void cleanupmodels();

static inline auto loadmapmodel(int n) -> model *
{
    if (mapmodels.inrange(n))
    {
        model *m = mapmodels[n].m;
        return m ? m : loadmodel(nullptr, n);
    }
    return nullptr;
}

static inline auto getmminfo(int n) -> mapmodelinfo *
{
    return mapmodels.inrange(n) ? &mapmodels[n] : nullptr;
}

// renderparticles
extern int particlelayers;

enum
{
    PL_ALL = 0,
    PL_UNDER,
    PL_OVER,
    PL_NOLAYER
};

extern void initparticles();
extern void clearparticles();
extern void clearparticleemitters();
extern void seedparticles();
extern void updateparticles();
extern void debugparticles();
extern void renderparticles(int layer = PL_ALL);
extern auto printparticles(extentity &e, char *buf, int len) -> bool;
extern void cleanupparticles();

// stain
enum
{
    STAINBUF_OPAQUE = 0,
    STAINBUF_TRANSPARENT,
    STAINBUF_MAPMODEL,
    NUMSTAINBUFS
};

struct stainrenderer;

extern void initstains();
extern void clearstains();
extern auto renderstains(int sbuf, bool gbuf, int layer = 0) -> bool;
extern void cleanupstains();
extern void genstainmmtri(stainrenderer *s, const vec v[3]);

// rendersky
extern int skytexture;
extern int skyshadow;
extern int explicitsky;

extern void drawskybox(bool clear = false);
extern auto hasskybox() -> bool;
extern auto limitsky() -> bool;
extern auto renderexplicitsky(bool outline = false) -> bool;
extern void cleanupsky();

// ui

namespace UI
{
    auto hascursor() -> bool;
    void getcursorpos(float &x, float &y);
    void resetcursor();
    auto movecursor(int dx, int dy) -> bool;
    auto keypress(int code, bool isdown) -> bool;
    auto textinput(const char *str, int len) -> bool;
    auto abovehud() -> float;

    void setup();
    void update();
    void render();
    void cleanup();
}

// menus

extern int mainmenu;

extern void addchange(const char *desc, int type);
extern void clearchanges(int type);
extern void menuprocess();
extern void clearmainmenu();

// sound
extern void clearmapsounds();
extern void checkmapsounds();
extern void updatesounds();
extern void preloadmapsounds();

extern void initmumble();
extern void closemumble();
extern void updatemumble();

// grass
extern void loadgrassshaders();
extern void generategrass();
extern void rendergrass();
extern void cleanupgrass();

// blendmap
extern int blendpaintmode;

struct BlendMapCache;
extern auto newblendmapcache() -> BlendMapCache *;
extern void freeblendmapcache(BlendMapCache *&cache);
extern auto setblendmaporigin(BlendMapCache *cache, const ivec &o, int size) -> bool;
extern auto hasblendmap(BlendMapCache *cache) -> bool;
extern auto lookupblendmap(BlendMapCache *cache, const vec &pos) -> uchar;
extern void resetblendmap();
extern void enlargeblendmap();
extern void shrinkblendmap(int octant);
extern void optimizeblendmap();
extern void stoppaintblendmap();
extern void trypaintblendmap();
extern void renderblendbrush(GLuint tex, float x, float y, float w, float h);
extern void renderblendbrush();
extern auto loadblendmap(stream *f, int info) -> bool;
extern void saveblendmap(stream *f);
extern auto shouldsaveblendmap() -> uchar;
extern auto usesblendmap(int x1 = 0, int y1 = 0, int x2 = worldsize, int y2 = worldsize) -> bool;
extern auto calcblendlayer(int x1, int y1, int x2, int y2) -> int;
extern void updateblendtextures(int x1 = 0, int y1 = 0, int x2 = worldsize, int y2 = worldsize);
extern void bindblendtexture(const ivec &p);
extern void clearblendtextures();
extern void cleanupblendmap();

// recorder

namespace recorder
{
    extern void stop();
    extern void capture(bool overlay = true);
    extern void cleanup();
}

#endif  // STANDALONE

#endif  // __ENGINE_H__
