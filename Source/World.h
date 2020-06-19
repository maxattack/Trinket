#pragma once
#include "Input.h"
#include "Assets.h"
#include "Scene.h"
#include "Skeleton.h"
#include "Physics.h"
#include "Animation.h"
#include "Graphics.h"
#include "Scripting.h"

class World {
public:

	World(Display* aDisplay);

	Input input;
	AssetDatabase db;
	Scene scene;
	SkelRegistry skel;
	Physics phys;
	AnimationRuntime anim;
	Graphics gfx;
	ScriptVM vm;

	bool IsValid(ObjectID id) const { return id.IsFingerprinted() ? db.IsValid(id) : scene.IsValid(id); }

	void HandleEvent(const SDL_Event& ev);
	void Update();

	World* Clone();
};