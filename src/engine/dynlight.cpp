#include "engine/engine.h"

VARNP(dynlights, usedynlights, 0, 1, 1);
VARP(dynlightdist, 0, 1024, 10000);

struct dynlight {
	vec o, hud;
	float radius, initradius, curradius, dist;
	vec color, initcolor, curcolor;
	int fade, peak, expire, flags;
	physent* owner;
	vec dir;
	int spot;

	void calcradius() {
		if (fade + peak > 0) {
			int remaining = expire - lastmillis;
			if (flags & DL_EXPAND)
				curradius = initradius + (radius - initradius) * (1.0f - remaining / float(fade + peak));
			else if (!(flags & DL_FLASH) && remaining > fade)
				curradius = initradius + (radius - initradius) * (1.0f - float(remaining - fade) / peak);
			else if (flags & DL_SHRINK)
				curradius = (radius * remaining) / fade;
			else
				curradius = radius;
		} else
			curradius = radius;
	}

	void calccolor() {
		if (flags & DL_FLASH || peak <= 0)
			curcolor = color;
		else {
			int peaking = expire - lastmillis - fade;
			if (peaking <= 0)
				curcolor = color;
			else
				curcolor.lerp(initcolor, color, 1.0f - float(peaking) / peak);
		}

		float intensity = 1.0f;
		if (fade > 0) {
			int fading = expire - lastmillis;
			if (fading < fade)
				intensity = float(fading) / fade;
		}
		curcolor.mul(intensity);
	}
};

vector<dynlight> dynlights;
vector<dynlight*> closedynlights;

void adddynlight(const vec& o,
				 float radius,
				 const vec& color,
				 int fade,
				 int peak,
				 int flags,
				 float initradius,
				 const vec& initcolor,
				 physent* owner,
				 const vec& dir,
				 int spot) {
	if (!usedynlights)
		return;
	if (o.dist(camera1->o) > dynlightdist || radius <= 0)
		return;

	int insert = 0, expire = fade + peak + lastmillis;
	loopvrev(dynlights) if (expire >= dynlights[i].expire) {
		insert = i + 1;
		break;
	}
	dynlight d;
	d.o = d.hud = o;
	d.radius = radius;
	d.initradius = initradius;
	d.color = color;
	d.initcolor = initcolor;
	d.fade = fade;
	d.peak = peak;
	d.expire = expire;
	d.flags = flags;
	d.owner = owner;
	d.dir = dir;
	d.spot = spot;
	dynlights.insert(insert, d);
}

void cleardynlights() {
	int faded = -1;
	loopv(dynlights) if (lastmillis < dynlights[i].expire) {
		faded = i;
		break;
	}
	if (faded < 0)
		dynlights.setsize(0);
	else if (faded > 0)
		dynlights.remove(0, faded);
}

void removetrackeddynlights(physent* owner) {
	loopvrev(dynlights) if (owner ? dynlights[i].owner == owner : dynlights[i].owner != nullptr) dynlights.remove(i);
}

void updatedynlights() {
	cleardynlights();
	game::adddynlights();

	loopv(dynlights) {
		dynlight& d = dynlights[i];
		if (d.owner)
			game::dynlighttrack(d.owner, d.o, d.hud);
		d.calcradius();
		d.calccolor();
	}
}

auto finddynlights() -> int {
	closedynlights.setsize(0);
	if (!usedynlights)
		return 0;
	physent e;
	e.type = ENT_CAMERA;
	loopvj(dynlights) {
		dynlight& d = dynlights[j];
		if (d.curradius <= 0)
			continue;
		d.dist = camera1->o.dist(d.o) - d.curradius;
		if (d.dist > dynlightdist || isfoggedsphere(d.curradius, d.o) || pvsoccludedsphere(d.o, d.curradius))
			continue;
		e.o = d.o;
		e.radius = e.xradius = e.yradius = e.eyeheight = e.aboveeye = d.curradius;
		if (!collide(&e, vec(0, 0, 0), 0, false))
			continue;

		int insert = 0;
		loopvrev(closedynlights) if (d.dist >= closedynlights[i]->dist) {
			insert = i + 1;
			break;
		}
		closedynlights.insert(insert, &d);
	}
	return closedynlights.length();
}

auto getdynlight(int n, vec& o, float& radius, vec& color, vec& dir, int& spot, int& flags) -> bool {
	if (!closedynlights.inrange(n))
		return false;
	dynlight& d = *closedynlights[n];
	o = d.o;
	radius = d.curradius;
	color = d.curcolor;
	spot = d.spot;
	dir = d.dir;
	flags = d.flags & 0xFF;
	return true;
}

void dynlightreaching(const vec& target, vec& color, vec& dir, bool hud) {
	vec dyncolor(0, 0, 0);	//, dyndir(0, 0, 0);
	loopv(dynlights) {
		dynlight& d = dynlights[i];
		if (d.curradius <= 0)
			continue;

		vec ray(target);
		ray.sub(hud ? d.hud : d.o);
		float mag = ray.squaredlen();
		if (mag >= d.curradius * d.curradius)
			continue;
		mag = sqrtf(mag);

		float intensity = 1 - mag / d.curradius;
		if (d.spot > 0 && mag > 1e-4f) {
			float spotatten = 1 - (1 - ray.dot(d.dir) / mag) / (1 - cos360(d.spot));
			if (spotatten <= 0)
				continue;
			intensity *= spotatten;
		}

		vec color = d.curcolor;
		color.mul(intensity);
		dyncolor.add(color);
		// dyndir.add(ray.mul(intensity/mag));
	}
#if 0
    if(!dyndir.iszero())
    {
        dyndir.normalize();
        float x = dyncolor.magnitude(), y = color.magnitude();
        if(x+y>0)
        {
            dir.mul(x);
            dyndir.mul(y);
            dir.add(dyndir).div(x+y);
            if(dir.iszero()) dir = vec(0, 0, 1);
            else dir.normalize();
        }
    }
#endif
	color.add(dyncolor);
}
