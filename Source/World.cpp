#include "World.h"

World::World(Display* aDisplay)
	: skel(&db, &scene)
	, phys(&db, &scene)
	, anim(&skel)
	, gfx(aDisplay, &skel)
{}

