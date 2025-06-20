#include "engine/engine.h"

VAR(oqdynent, 0, 1, 1);
VAR(animationinterpolationtime, 0, 200, 1000);

model *loadingmodel = nullptr;

#include "engine/ragdoll.h"
#include "engine/animmodel.h"
#include "engine/vertmodel.h"
#include "engine/skelmodel.h"
#include "engine/hitzone.h"

static model *(__cdecl *modeltypes[NUMMODELTYPES])(const char *);

static auto addmodeltype(int type, model *(__cdecl *loader)(const char *)) -> int
{
    modeltypes[type] = loader;
    return type;
}

#define MODELTYPE(modeltype, modelclass) \
static model *__loadmodel__##modelclass(const char *filename) \
{ \
    return new modelclass(filename); \
} \
UNUSED static int __dummy__##modelclass = addmodeltype((modeltype), __loadmodel__##modelclass);

#include "engine/md5.h"
#include "engine/obj.h"

MODELTYPE(MDL_MD5, md5);
MODELTYPE(MDL_OBJ, obj);

#define checkmdl if(!loadingmodel) { conoutf(CON_ERROR, "not loading a model"); return; }

void mdlcullface(int *cullface)
{
    checkmdl;
    loadingmodel->setcullface(*cullface);
}
COMMAND(mdlcullface, "i");

void mdlcolor(float *r, float *g, float *b)
{
    checkmdl;
    loadingmodel->setcolor(vec(*r, *g, *b));
}
COMMAND(mdlcolor, "fff");

void mdlcollide(int *collide)
{
    checkmdl;
    loadingmodel->collide = *collide!=0 ? (loadingmodel->collide ? loadingmodel->collide : COLLIDE_OBB) : COLLIDE_NONE;
}
COMMAND(mdlcollide, "i");

void mdlellipsecollide(int *collide)
{
    checkmdl;
    loadingmodel->collide = *collide!=0 ? COLLIDE_ELLIPSE : COLLIDE_NONE;
}
COMMAND(mdlellipsecollide, "i");

void mdltricollide(char *collide)
{
    checkmdl;
    DELETEA(loadingmodel->collidemodel);
    char *end = nullptr;
    int val = strtol(collide, &end, 0);
    if(*end) { val = 1; loadingmodel->collidemodel = newstring(collide); }
    loadingmodel->collide = val ? COLLIDE_TRI : COLLIDE_NONE;
}
COMMAND(mdltricollide, "s");

void mdlspec(float *percent)
{
    checkmdl;
    float spec = *percent > 0 ? *percent/100.0f : 0.0f;
    loadingmodel->setspec(spec);
}
COMMAND(mdlspec, "f");

void mdlgloss(int *gloss)
{
    checkmdl;
    loadingmodel->setgloss(clamp(*gloss, 0, 2));
}
COMMAND(mdlgloss, "i");

void mdlalphatest(float *cutoff)
{
    checkmdl;
    loadingmodel->setalphatest(max(0.0f, min(1.0f, *cutoff)));
}
COMMAND(mdlalphatest, "f");

void mdldepthoffset(int *offset)
{
    checkmdl;
    loadingmodel->depthoffset = *offset!=0;
}
COMMAND(mdldepthoffset, "i");

void mdlglow(float *percent, float *delta, float *pulse)
{
    checkmdl;
    float glow = *percent > 0 ? *percent/100.0f : 0.0f, glowdelta = *delta/100.0f, glowpulse = *pulse > 0 ? *pulse/1000.0f : 0;
    glowdelta -= glow;
    loadingmodel->setglow(glow, glowdelta, glowpulse);
}
COMMAND(mdlglow, "fff");

void mdlenvmap(float *envmapmax, float *envmapmin, char *envmap)
{
    checkmdl;
    loadingmodel->setenvmap(*envmapmin, *envmapmax, envmap[0] ? cubemapload(envmap) : nullptr);
}
COMMAND(mdlenvmap, "ffs");

void mdlfullbright(float *fullbright)
{
    checkmdl;
    loadingmodel->setfullbright(*fullbright);
}
COMMAND(mdlfullbright, "f");

void mdlshader(char *shader)
{
    checkmdl;
    loadingmodel->setshader(lookupshaderbyname(shader));
}
COMMAND(mdlshader, "s");

void mdlspin(float *yaw, float *pitch, float *roll)
{
    checkmdl;
    loadingmodel->spinyaw = *yaw;
    loadingmodel->spinpitch = *pitch;
    loadingmodel->spinroll = *roll;
}
COMMAND(mdlspin, "fff");

void mdlscale(float *percent)
{
    checkmdl;
    float scale = *percent > 0 ? *percent/100.0f : 1.0f;
    loadingmodel->scale = scale;
}
COMMAND(mdlscale, "f");

void mdltrans(float *x, float *y, float *z)
{
    checkmdl;
    loadingmodel->translate = vec(*x, *y, *z);
}
COMMAND(mdltrans, "fff");

void mdlyaw(float *angle)
{
    checkmdl;
    loadingmodel->offsetyaw = *angle;
}
COMMAND(mdlyaw, "f");

void mdlpitch(float *angle)
{
    checkmdl;
    loadingmodel->offsetpitch = *angle;
}
COMMAND(mdlpitch, "f");

void mdlroll(float *angle)
{
    checkmdl;
    loadingmodel->offsetroll = *angle;
}
COMMAND(mdlroll, "f");

void mdlshadow(int *shadow)
{
    checkmdl;
    loadingmodel->shadow = *shadow!=0;
}
COMMAND(mdlshadow, "i");

void mdlalphashadow(int *alphashadow)
{
    checkmdl;
    loadingmodel->alphashadow = *alphashadow!=0;
}
COMMAND(mdlalphashadow, "i");

void mdlbb(float *rad, float *h, float *eyeheight)
{
    checkmdl;
    loadingmodel->collidexyradius = *rad;
    loadingmodel->collideheight = *h;
    loadingmodel->eyeheight = *eyeheight;
}
COMMAND(mdlbb, "fff");

void mdlextendbb(float *x, float *y, float *z)
{
    checkmdl;
    loadingmodel->bbextend = vec(*x, *y, *z);
}
COMMAND(mdlextendbb, "fff");

void mdlname()
{
    checkmdl;
    result(loadingmodel->name);
}
COMMAND(mdlname, "");

#define checkragdoll \
    if(!loadingmodel->skeletal()) { conoutf(CON_ERROR, "not loading a skeletal model"); return; } \
    skelmodel *m = (skelmodel *)loadingmodel; \
    if(m->parts.empty()) return; \
    skelmodel::skelmeshgroup *meshes = (skelmodel::skelmeshgroup *)m->parts.last()->meshes; \
    if(!meshes) return; \
    skelmodel::skeleton *skel = meshes->skel; \
    if(!skel->ragdoll) skel->ragdoll = new ragdollskel; \
    ragdollskel *ragdoll = skel->ragdoll; \
    if(ragdoll->loaded) return;


void rdvert(float *x, float *y, float *z, float *radius)
{
    checkragdoll;
    ragdollskel::vert &v = ragdoll->verts.add();
    v.pos = vec(*x, *y, *z);
    v.radius = *radius > 0 ? *radius : 1;
}
COMMAND(rdvert, "ffff");

void rdeye(int *v)
{
    checkragdoll;
    ragdoll->eye = *v;
}
COMMAND(rdeye, "i");

void rdtri(int *v1, int *v2, int *v3)
{
    checkragdoll;
    ragdollskel::tri &t = ragdoll->tris.add();
    t.vert[0] = *v1;
    t.vert[1] = *v2;
    t.vert[2] = *v3;
}
COMMAND(rdtri, "iii");

void rdjoint(int *n, int *t, int *v1, int *v2, int *v3)
{
    checkragdoll;
    if(*n < 0 || *n >= skel->numbones) return;
    ragdollskel::joint &j = ragdoll->joints.add();
    j.bone = *n;
    j.tri = *t;
    j.vert[0] = *v1;
    j.vert[1] = *v2;
    j.vert[2] = *v3;
}
COMMAND(rdjoint, "iibbb");

void rdlimitdist(int *v1, int *v2, float *mindist, float *maxdist)
{
    checkragdoll;
    ragdollskel::distlimit &d = ragdoll->distlimits.add();
    d.vert[0] = *v1;
    d.vert[1] = *v2;
    d.mindist = *mindist;
    d.maxdist = max(*maxdist, *mindist);
}
COMMAND(rdlimitdist, "iiff");

void rdlimitrot(int *t1, int *t2, float *maxangle, float *qx, float *qy, float *qz, float *qw)
{
    checkragdoll;
    ragdollskel::rotlimit &r = ragdoll->rotlimits.add();
    r.tri[0] = *t1;
    r.tri[1] = *t2;
    r.maxangle = *maxangle * RAD;
    r.maxtrace = 1 + 2*cos(r.maxangle);
    r.middle = matrix3(quat(*qx, *qy, *qz, *qw));
}
COMMAND(rdlimitrot, "iifffff");

void rdanimjoints(int *on)
{
    checkragdoll;
    ragdoll->animjoints = *on!=0;
}
COMMAND(rdanimjoints, "i");

// mapmodels

vector<mapmodelinfo> mapmodels;
static const char * const mmprefix = "mapmodel/";
static const int mmprefixlen = strlen(mmprefix);

void mapmodel(char *name)
{
    mapmodelinfo &mmi = mapmodels.add();
    if(name[0]) formatstring(mmi.name, "%s%s", mmprefix, name);
    else mmi.name[0] = '\0';
    mmi.m = mmi.collide = nullptr;
}

void mapmodelreset(int *n)
{
    if(!(identflags&IDF_OVERRIDDEN) && !game::allowedittoggle()) return;
    mapmodels.shrink(clamp(*n, 0, mapmodels.length()));
}

auto mapmodelname(int i) -> const char * { return mapmodels.inrange(i) ? mapmodels[i].name : nullptr; }

ICOMMAND(mmodel, "s", (char *name), mapmodel(name));
COMMAND(mapmodel, "s");
COMMAND(mapmodelreset, "i");
ICOMMAND(mapmodelname, "ii", (int *index, int *prefix), { if(mapmodels.inrange(*index)) result(mapmodels[*index].name[0] ? mapmodels[*index].name + (*prefix ? 0 : mmprefixlen) : ""); });
ICOMMAND(nummapmodels, "", (), { intret(mapmodels.length()); });

// model registry

hashnameset<model *> models;
vector<const char *> preloadmodels;
hashset<char *> failedmodels;

void preloadmodel(const char *name)
{
    if(!name || !name[0] || models.access(name) || preloadmodels.htfind(name) >= 0) return;
    preloadmodels.add(newstring(name));
}

void flushpreloadedmodels(bool msg)
{
    loopv(preloadmodels)
    {
        loadprogress = float(i+1)/preloadmodels.length();
        model *m = loadmodel(preloadmodels[i], -1, msg);
        if(!m) { if(msg) conoutf(CON_WARN, "could not load model: %s", preloadmodels[i]); }
        else
        {
            m->preloadmeshes();
            m->preloadshaders();
        }
    }
    preloadmodels.deletearrays();
    loadprogress = 0;
}

void preloadusedmapmodels(bool msg, bool bih)
{
    vector<extentity *> &ents = entities::getents();
    vector<int> used;
    loopv(ents)
    {
        extentity &e = *ents[i];
        if(e.type==ET_MAPMODEL && e.attr1 >= 0 && used.find(e.attr1) < 0) used.add(e.attr1);
    }

    vector<const char *> col;
    loopv(used)
    {
        loadprogress = float(i+1)/used.length();
        int mmindex = used[i];
        if(!mapmodels.inrange(mmindex)) { if(msg) conoutf(CON_WARN, "could not find map model: %d", mmindex); continue; }
        mapmodelinfo &mmi = mapmodels[mmindex];
        if(!mmi.name[0]) continue;
        model *m = loadmodel(nullptr, mmindex, msg);
        if(!m) { if(msg) conoutf(CON_WARN, "could not load map model: %s", mmi.name); }
        else
        {
            if(bih) m->preloadBIH();
            else if(m->collide == COLLIDE_TRI && !m->collidemodel && m->bih) m->setBIH();
            m->preloadmeshes();
            m->preloadshaders();
            if(m->collidemodel && col.htfind(m->collidemodel) < 0) col.add(m->collidemodel);
        }
    }

    loopv(col)
    {
        loadprogress = float(i+1)/col.length();
        model *m = loadmodel(col[i], -1, msg);
        if(!m) { if(msg) conoutf(CON_WARN, "could not load collide model: %s", col[i]); }
        else if(!m->bih) m->setBIH();
    }

    loadprogress = 0;
}

auto loadmodel(const char *name, int i, bool msg) -> model *
{
    if(!name)
    {
        if(!mapmodels.inrange(i)) return nullptr;
        mapmodelinfo &mmi = mapmodels[i];
        if(mmi.m) return mmi.m;
        name = mmi.name;
    }
    model **mm = models.access(name);
    model *m;
    if(mm) m = *mm;
    else
    {
        if(!name[0] || loadingmodel || failedmodels.find(name, NULL)) return nullptr;
        if(msg)
        {
            defformatstring(filename, "media/model/%s", name);
            renderprogress(loadprogress, filename);
        }
        loopi(NUMMODELTYPES)
        {
            m = modeltypes[i](name);
            if(!m) continue;
            loadingmodel = m;
            if(m->load()) break;
            DELETEP(m);
        }
        loadingmodel = nullptr;
        if(!m)
        {
            failedmodels.add(newstring(name));
            return nullptr;
        }
        models.access(m->name, m);
    }
    if(mapmodels.inrange(i) && !mapmodels[i].m) mapmodels[i].m = m;
    return m;
}

void clear_models()
{
    enumerate(models, model *, m, delete m);
}

void cleanupmodels()
{
    enumerate(models, model *, m, m->cleanup());
}

void clearmodel(char *name)
{
    model *m = models.find(name, NULL);
    if(!m) { conoutf("model %s is not loaded", name); return; }
    loopv(mapmodels)
    {
        mapmodelinfo &mmi = mapmodels[i];
        if(mmi.m == m) mmi.m = nullptr;
        if(mmi.collide == m) mmi.collide = nullptr;
    }
    models.remove(name);
    m->cleanup();
    delete m;
    conoutf("cleared model %s", name);
}

COMMAND(clearmodel, "s");

auto modeloccluded(const vec &center, float radius) -> bool
{
    ivec bbmin(vec(center).sub(radius)), bbmax(vec(center).add(radius+1));
    return pvsoccluded(bbmin, bbmax) || bboccluded(bbmin, bbmax);
}

struct batchedmodel
{
    vec pos, center;
    float radius, yaw, pitch, roll, sizescale;
    vec4 colorscale;
    int anim, basetime, basetime2, flags, attached;
    union
    {
        int visible;
        int culled;
    };
    dynent *d;
    int next;
};
struct modelbatch
{
    model *m;
    int flags, batched;
};
static vector<batchedmodel> batchedmodels;
static vector<modelbatch> batches;
static vector<modelattach> modelattached;

void resetmodelbatches()
{
    batchedmodels.setsize(0);
    batches.setsize(0);
    modelattached.setsize(0);
}

void addbatchedmodel(model *m, batchedmodel &bm, int idx)
{
    modelbatch *b = nullptr;
    if(batches.inrange(m->batch))
    {
        b = &batches[m->batch];
        if(b->m == m && (b->flags & MDL_MAPMODEL) == (bm.flags & MDL_MAPMODEL))
            goto foundbatch;
    }
    
    m->batch = batches.length();
    b = &batches.add();
    b->m = m;
    b->flags = 0;
    b->batched = -1;

foundbatch:
    b->flags |= bm.flags;
    bm.next = b->batched;
    b->batched = idx;
}

static inline void renderbatchedmodel(model *m, const batchedmodel &b)
{
    modelattach *a = nullptr;
    if(b.attached>=0) a = &modelattached[b.attached];

    int anim = b.anim;
    if(shadowmapping > SM_REFLECT)
    {
        anim |= ANIM_NOSKIN;
    }
    else
    {
        if(b.flags&MDL_FULLBRIGHT) anim |= ANIM_FULLBRIGHT;
    }

    m->render(anim, b.basetime, b.basetime2, b.pos, b.yaw, b.pitch, b.roll, b.d, a, b.sizescale, b.colorscale);
}

VAR(maxmodelradiusdistance, 10, 200, 1000);

static inline void enablecullmodelquery()
{
    startbb();
}

static inline void rendercullmodelquery(model *m, dynent *d, const vec &center, float radius)
{
    if(fabs(camera1->o.x-center.x) < radius+1 &&
       fabs(camera1->o.y-center.y) < radius+1 &&
       fabs(camera1->o.z-center.z) < radius+1)
    {
        d->query = nullptr;
        return;
    }
    d->query = newquery(d);
    if(!d->query) return;
    startquery(d->query);
    int br = int(radius*2)+1;
    drawbb(ivec(int(center.x-radius), int(center.y-radius), int(center.z-radius)), ivec(br, br, br));
    endquery(d->query);
}

static inline void disablecullmodelquery()
{
    endbb();
}

static inline auto cullmodel(model *m, const vec &center, float radius, int flags, dynent *d = nullptr) -> int
{
    if(flags&MDL_CULL_DIST && center.dist(camera1->o)/radius>maxmodelradiusdistance) return MDL_CULL_DIST;
    if(flags&MDL_CULL_VFC && isfoggedsphere(radius, center)) return MDL_CULL_VFC;
    if(flags&MDL_CULL_OCCLUDED && modeloccluded(center, radius)) return MDL_CULL_OCCLUDED;
    else if(flags&MDL_CULL_QUERY && d->query && d->query->owner==d && checkquery(d->query)) return MDL_CULL_QUERY;
    return 0;
}

static inline auto shadowmaskmodel(const vec &center, float radius) -> int
{
    switch(shadowmapping)
    {
        case SM_REFLECT:
            return calcspherersmsplits(center, radius);
        case SM_CUBEMAP:
        {
            vec scenter = vec(center).sub(shadoworigin);
            float sradius = radius + shadowradius;
            if(scenter.squaredlen() >= sradius*sradius) return 0;
            return calcspheresidemask(scenter, radius, shadowbias);
        }
        case SM_CASCADE:
            return calcspherecsmsplits(center, radius);
        case SM_SPOT:
        {
            vec scenter = vec(center).sub(shadoworigin);
            float sradius = radius + shadowradius;
            return scenter.squaredlen() < sradius*sradius && sphereinsidespot(shadowdir, shadowspot, scenter, radius) ? 1 : 0;
        }
    }
    return 0;
}

void shadowmaskbatchedmodels(bool dynshadow)
{
    loopv(batchedmodels)
    {
        batchedmodel &b = batchedmodels[i];
        if(b.flags&MDL_MAPMODEL) break;
        b.visible = dynshadow && b.colorscale.a >= 1 ? shadowmaskmodel(b.center, b.radius) : 0;
    }
}

auto batcheddynamicmodels() -> int
{
    int visible = 0;
    loopv(batchedmodels)
    {
        batchedmodel &b = batchedmodels[i];
        if(b.flags&MDL_MAPMODEL) break;
        visible |= b.visible;
    }
    loopv(batches)
    {
        modelbatch &b = batches[i];
        if(!(b.flags&MDL_MAPMODEL) || !b.m->animated()) continue;
        for(int j = b.batched; j >= 0;)
        {
            batchedmodel &bm = batchedmodels[j];
            j = bm.next;
            visible |= bm.visible;
        }
    }
    return visible;
}

auto batcheddynamicmodelbounds(int mask, vec &bbmin, vec &bbmax) -> int
{
    int vis = 0;
    loopv(batchedmodels)
    {
        batchedmodel &b = batchedmodels[i];
        if(b.flags&MDL_MAPMODEL) break;
        if(b.visible&mask)
        {
            bbmin.min(vec(b.center).sub(b.radius));
            bbmax.max(vec(b.center).add(b.radius));
            ++vis;
        }
    }
    loopv(batches)
    {
        modelbatch &b = batches[i];
        if(!(b.flags&MDL_MAPMODEL) || !b.m->animated()) continue;
        for(int j = b.batched; j >= 0;)
        {
            batchedmodel &bm = batchedmodels[j];
            j = bm.next;
            if(bm.visible&mask)
            {
                bbmin.min(vec(bm.center).sub(bm.radius));
                bbmax.max(vec(bm.center).add(bm.radius));
                ++vis;
            }
        }
    }
    return vis;
}

void rendershadowmodelbatches(bool dynmodel)
{
    loopv(batches)
    {
        modelbatch &b = batches[i];
        if(!b.m->shadow || (!dynmodel && (!(b.flags&MDL_MAPMODEL) || b.m->animated()))) continue;
        bool rendered = false;
        for(int j = b.batched; j >= 0;)
        {
            batchedmodel &bm = batchedmodels[j];
            j = bm.next;
            if(!(bm.visible&(1<<shadowside))) continue;
            if(!rendered) { b.m->startrender(); rendered = true; }
            renderbatchedmodel(b.m, bm);
        }
        if(rendered) b.m->endrender();
    }
}

void rendermapmodelbatches()
{
    enableaamask();
    loopv(batches)
    {
        modelbatch &b = batches[i];
        if(!(b.flags&MDL_MAPMODEL)) continue;
        b.m->startrender();
        setaamask(b.m->animated());
        for(int j = b.batched; j >= 0;)
        {
            batchedmodel &bm = batchedmodels[j];
            renderbatchedmodel(b.m, bm);
            j = bm.next;
        }
        b.m->endrender();
    }
    disableaamask();
}

float transmdlsx1 = -1, transmdlsy1 = -1, transmdlsx2 = 1, transmdlsy2 = 1;
uint transmdltiles[LIGHTTILE_MAXH];

void rendermodelbatches()
{
    transmdlsx1 = transmdlsy1 = 1;
    transmdlsx2 = transmdlsy2 = -1;
    memset(transmdltiles, 0, sizeof(transmdltiles));

    enableaamask();
    loopv(batches)
    {
        modelbatch &b = batches[i];
        if(b.flags&MDL_MAPMODEL) continue;
        bool rendered = false;
        for(int j = b.batched; j >= 0;)
        {
            batchedmodel &bm = batchedmodels[j];
            j = bm.next;
            bm.culled = cullmodel(b.m, bm.center, bm.radius, bm.flags, bm.d);
            if(bm.culled || bm.flags&MDL_ONLYSHADOW) continue;
            if(bm.colorscale.a < 1)
            {
                float sx1, sy1, sx2, sy2;
                ivec bbmin(vec(bm.center).sub(bm.radius)), bbmax(vec(bm.center).add(bm.radius+1));
                if(calcbbscissor(bbmin, bbmax, sx1, sy1, sx2, sy2))
                {
                    transmdlsx1 = min(transmdlsx1, sx1);
                    transmdlsy1 = min(transmdlsy1, sy1);
                    transmdlsx2 = max(transmdlsx2, sx2);
                    transmdlsy2 = max(transmdlsy2, sy2);
                    masktiles(transmdltiles, sx1, sy1, sx2, sy2);
                }
                continue;
            }
            if(!rendered)
            {
                b.m->startrender();
                rendered = true;
                setaamask(true);
            }
            if(bm.flags&MDL_CULL_QUERY)
            {
                bm.d->query = newquery(bm.d);
                if(bm.d->query)
                {
                    startquery(bm.d->query);
                    renderbatchedmodel(b.m, bm);
                    endquery(bm.d->query);
                    continue;
                }
            }
            renderbatchedmodel(b.m, bm);
        }
        if(rendered) b.m->endrender();
        if(b.flags&MDL_CULL_QUERY)
        {
            bool queried = false;
            for(int j = b.batched; j >= 0;)
            {
                batchedmodel &bm = batchedmodels[j];
                j = bm.next;
                if(bm.culled&(MDL_CULL_OCCLUDED|MDL_CULL_QUERY) && bm.flags&MDL_CULL_QUERY)
                {
                    if(!queried)
                    {
                        if(rendered) setaamask(false);
                        enablecullmodelquery();
                        queried = true;
                    }
                    rendercullmodelquery(b.m, bm.d, bm.center, bm.radius);
                }
            }
            if(queried) disablecullmodelquery();
        }
    }
    disableaamask();
}

void rendertransparentmodelbatches(int stencil)
{
    enableaamask(stencil);
    loopv(batches)
    {
        modelbatch &b = batches[i];
        if(b.flags&MDL_MAPMODEL) continue;
        bool rendered = false;
        for(int j = b.batched; j >= 0;)
        {
            batchedmodel &bm = batchedmodels[j];
            j = bm.next;
            bm.culled = cullmodel(b.m, bm.center, bm.radius, bm.flags, bm.d);
            if(bm.culled || bm.colorscale.a >= 1 || bm.flags&MDL_ONLYSHADOW) continue;
            if(!rendered)
            {
                b.m->startrender();
                rendered = true;
                setaamask(true);
            }
            if(bm.flags&MDL_CULL_QUERY)
            {
                bm.d->query = newquery(bm.d);
                if(bm.d->query)
                {
                    startquery(bm.d->query);
                    renderbatchedmodel(b.m, bm);
                    endquery(bm.d->query);
                    continue;
                }
            }
            renderbatchedmodel(b.m, bm);
        }
        if(rendered) b.m->endrender();
    }
    disableaamask();
}

static occludequery *modelquery = nullptr;
static int modelquerybatches = -1, modelquerymodels = -1, modelqueryattached = -1;
 
void startmodelquery(occludequery *query)
{
    modelquery = query;
    modelquerybatches = batches.length();
    modelquerymodels = batchedmodels.length();
    modelqueryattached = modelattached.length();
}

void endmodelquery()
{
    if(batchedmodels.length() == modelquerymodels)
    {
        modelquery->fragments = 0;
        modelquery = nullptr;
        return;
    }
    enableaamask();
    startquery(modelquery);
    loopv(batches)
    {
        modelbatch &b = batches[i];
        int j = b.batched;
        if(j < modelquerymodels) continue;
        b.m->startrender();
        setaamask(!(b.flags&MDL_MAPMODEL) || b.m->animated());
        do
        {
            batchedmodel &bm = batchedmodels[j];
            renderbatchedmodel(b.m, bm);
            j = bm.next;
        }
        while(j >= modelquerymodels);
        b.batched = j;
        b.m->endrender();
    }
    endquery(modelquery);
    modelquery = nullptr;
    batches.setsize(modelquerybatches);
    batchedmodels.setsize(modelquerymodels);
    modelattached.setsize(modelqueryattached);
    disableaamask();
}

void clearbatchedmapmodels()
{
    loopv(batches)
    {
        modelbatch &b = batches[i];
        if(b.flags&MDL_MAPMODEL)
        {
            batchedmodels.setsize(b.batched);
            batches.setsize(i);
            break;
        }
    }
}

void rendermapmodel(int idx, int anim, const vec &o, float yaw, float pitch, float roll, int flags, int basetime, float size)
{
    if(!mapmodels.inrange(idx)) return;
    mapmodelinfo &mmi = mapmodels[idx];
    model *m = mmi.m ? mmi.m : loadmodel(mmi.name);
    if(!m) return;

    vec center, bbradius;
    m->boundbox(center, bbradius);
    float radius = bbradius.magnitude();
    center.mul(size);
    if(roll) center.rotate_around_y(-roll*RAD);
    if(pitch && m->pitched()) center.rotate_around_x(pitch*RAD);
    center.rotate_around_z(yaw*RAD);
    center.add(o);
    radius *= size;

    int visible = 0;
    if(shadowmapping)
    {
        if(!m->shadow) return;
        visible = shadowmaskmodel(center, radius);
        if(!visible) return;
    }
    else if(flags&(MDL_CULL_VFC|MDL_CULL_DIST|MDL_CULL_OCCLUDED) && cullmodel(m, center, radius, flags))
        return;

    batchedmodel &b = batchedmodels.add();
    b.pos = o;
    b.center = center;
    b.radius = radius;
    b.anim = anim;
    b.yaw = yaw;
    b.pitch = pitch;
    b.roll = roll;
    b.basetime = basetime;
    b.basetime2 = 0;
    b.sizescale = size;
    b.colorscale = vec4(1, 1, 1, 1);
    b.flags = flags | MDL_MAPMODEL;
    b.visible = visible;
    b.d = nullptr;
    b.attached = -1;
    addbatchedmodel(m, b, batchedmodels.length()-1);
}

void rendermodel(const char *mdl, int anim, const vec &o, float yaw, float pitch, float roll, int flags, dynent *d, modelattach *a, int basetime, int basetime2, float size, const vec4 &color)
{
    model *m = loadmodel(mdl);
    if(!m) return;

    vec center, bbradius;
    m->boundbox(center, bbradius);
    float radius = bbradius.magnitude();
    if(d)
    {
        if(d->ragdoll)
        {
            if(anim&ANIM_RAGDOLL && d->ragdoll->millis >= basetime)
            {
                radius = max(radius, d->ragdoll->radius);
                center = d->ragdoll->center;
                goto hasboundbox;
            }
            DELETEP(d->ragdoll);
        }
        if(anim&ANIM_RAGDOLL) flags &= ~(MDL_CULL_VFC | MDL_CULL_OCCLUDED | MDL_CULL_QUERY);
    }
    center.mul(size);
    if(roll) center.rotate_around_y(-roll*RAD);
    if(pitch && m->pitched()) center.rotate_around_x(pitch*RAD);
    center.rotate_around_z(yaw*RAD);
    center.add(o);
hasboundbox:
    radius *= size;

    if(flags&MDL_NORENDER) anim |= ANIM_NORENDER;

    if(a) for(int i = 0; a[i].tag; i++)
    {
        if(a[i].name) a[i].m = loadmodel(a[i].name);
    }

    if(flags&MDL_CULL_QUERY)
    {
        if(!oqfrags || !oqdynent || !d) flags &= ~MDL_CULL_QUERY;
    }

    if(flags&MDL_NOBATCH)
    {
        int culled = cullmodel(m, center, radius, flags, d);
        if(culled)
        {
            if(culled&(MDL_CULL_OCCLUDED|MDL_CULL_QUERY) && flags&MDL_CULL_QUERY)
            {
                enablecullmodelquery();
                rendercullmodelquery(m, d, center, radius);
                disablecullmodelquery();
            }
            return;
        }
        enableaamask();
        if(flags&MDL_CULL_QUERY)
        {
            d->query = newquery(d);
            if(d->query) startquery(d->query);
        }
        m->startrender();
        setaamask(true);
        if(flags&MDL_FULLBRIGHT) anim |= ANIM_FULLBRIGHT;
        m->render(anim, basetime, basetime2, o, yaw, pitch, roll, d, a, size, color);
        m->endrender();
        if(flags&MDL_CULL_QUERY && d->query) endquery(d->query);
        disableaamask();
        return;
    }

    batchedmodel &b = batchedmodels.add();
    b.pos = o;
    b.center = center;
    b.radius = radius;
    b.anim = anim;
    b.yaw = yaw;
    b.pitch = pitch;
    b.roll = roll;
    b.basetime = basetime;
    b.basetime2 = basetime2;
    b.sizescale = size;
    b.colorscale = color;
    b.flags = flags;
    b.visible = 0;
    b.d = d;
    b.attached = a ? modelattached.length() : -1;
    if(a) for(int i = 0;; i++) { modelattached.add(a[i]); if(!a[i].tag) break; }
    addbatchedmodel(m, b, batchedmodels.length()-1);
}

auto intersectmodel(const char *mdl, int anim, const vec &pos, float yaw, float pitch, float roll, const vec &o, const vec &ray, float &dist, int mode, dynent *d, modelattach *a, int basetime, int basetime2, float size) -> int
{
    model *m = loadmodel(mdl);
    if(!m) return -1;
    if(d && d->ragdoll && (!(anim&ANIM_RAGDOLL) || d->ragdoll->millis < basetime)) DELETEP(d->ragdoll);
    if(a) for(int i = 0; a[i].tag; i++)
    {
        if(a[i].name) a[i].m = loadmodel(a[i].name);
    }
    return m->intersect(anim, basetime, basetime2, pos, yaw, pitch, roll, d, a, size, o, ray, dist, mode);
}

void abovemodel(vec &o, const char *mdl)
{
    model *m = loadmodel(mdl);
    if(!m) return;
    o.z += m->above();
}

auto matchanim(const char *name, const char *pattern) -> bool
{
    for(;; pattern++)
    {
        const char *s = name;
        char c;
        for(;; pattern++)
        {
            c = *pattern;
            if(!c || c=='|') break;
            else if(c=='*')
            {
                if(!*s || iscubespace(*s)) break;
                do s++; while(*s && !iscubespace(*s));
            }
            else if(c!=*s) break;
            else s++;
        }
        if(!*s && (!c || c=='|')) return true;
        pattern = strchr(pattern, '|');
        if(!pattern) break;
    }
    return false;
}

ICOMMAND(findanims, "s", (char *name),
{
    vector<int> anims;
    game::findanims(name, anims);
    vector<char> buf;
    string num;
    loopv(anims)
    {
        formatstring(num, "%d", anims[i]);
        if(i > 0) buf.add(' ');
        buf.put(num, strlen(num));
    }
    buf.add('\0');
    result(buf.getbuf());
});

void loadskin(const char *dir, const char *altdir, Texture *&skin, Texture *&masks) // model skin sharing
{
#define ifnoload(tex, path) if((tex = textureload(path, 0, true, false))==notexture)
#define tryload(tex, prefix, cmd, name) \
    ifnoload(tex, makerelpath(mdir, name ".jpg", prefix, cmd)) \
    { \
        ifnoload(tex, makerelpath(mdir, name ".png", prefix, cmd)) \
        { \
            ifnoload(tex, makerelpath(maltdir, name ".jpg", prefix, cmd)) \
            { \
                ifnoload(tex, makerelpath(maltdir, name ".png", prefix, cmd)) return; \
            } \
        } \
    }

    defformatstring(mdir, "media/model/%s", dir);
    defformatstring(maltdir, "media/model/%s", altdir);
    masks = notexture;
    tryload(skin, nullptr, nullptr, "skin");
    tryload(masks, nullptr, nullptr, "masks");
}

void setbbfrommodel(dynent *d, const char *mdl)
{
    model *m = loadmodel(mdl);
    if(!m) return;
    vec center, radius;
    m->collisionbox(center, radius);
    if(m->collide != COLLIDE_ELLIPSE) d->collidetype = COLLIDE_OBB;
    d->xradius   = radius.x + fabs(center.x);
    d->yradius   = radius.y + fabs(center.y);
    d->radius    = d->collidetype==COLLIDE_OBB ? sqrtf(d->xradius*d->xradius + d->yradius*d->yradius) : max(d->xradius, d->yradius);
    d->eyeheight = (center.z-radius.z) + radius.z*2*m->eyeheight;
    d->aboveeye  = radius.z*2*(1.0f-m->eyeheight);
}

