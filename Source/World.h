#pragma once
#include "Assets.h"
#include "Scene.h"
#include "Skeleton.h"
#include "Physics.h"
#include "Animation.h"
#include "Graphics.h"

class World {
public:

	World(Display* aDisplay);

	AssetDatabase db;
	Scene scene;
	SkelRegistry skel;
	Physics phys;
	AnimationRuntime anim;
	Graphics gfx;

	// TODO: support for taking "Snapshots"
};