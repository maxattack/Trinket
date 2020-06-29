#pragma once
#include "Input.h"
#include "Assets.h"
#include "Scene.h"
#include "Mesh.h"
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
	TextureRegistry tex;
	MaterialRegistry mat;
	MeshRegistry mesh;
	SkelRegistry skel;
	PhysicsRuntime phys;
	AnimationRuntime anim;
	Graphics gfx;
	ScriptVM vm;

	bool IsValid(ObjectID id) const { return id.IsFingerprinted() ? db.IsValid(id) : scene.IsValid(id); }

	void HandleEvent(const SDL_Event& ev);
	void Update();

	World* Clone();

	Input* GetInput() { return &input; }
	AssetDatabase* GetAssetDatabase() { return &db; }
	Scene* GetScene() { return &scene; }
	TextureRegistry* GetTextureRegistry() { return &tex; }
	MaterialRegistry* GetMaterialRegistry() { return &mat; }
	MeshRegistry* GetMeshRegistry() { return &mesh; }
	SkelRegistry* GetSkelRegistory() { return &skel; }
	PhysicsRuntime* GetPhysicsRuntime() { return &phys; }
	AnimationRuntime* GetAnimationRuntime() { return &anim; }
	Graphics* GetGraphics() { return &gfx; }
	ScriptVM* GetScriptVM() { return &vm; }
};