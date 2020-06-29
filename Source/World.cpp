#include "World.h"

World::World(Display* aDisplay)
	: tex(this)
	, mat(this)
	, mesh(this)
	, skel(this)
	, phys(this)
	, anim(this)
	, gfx(aDisplay, this)
	, vm(this)
{}

void World::HandleEvent(const SDL_Event& ev) {
	input.HandleEvent(ev);
}

void World::Update() {
	input.Update();
	if (input.GetDeltaTicks() > 0)
		phys.Tick(input.GetDeltaTime());
	vm.Update();
}

World* World::Clone() {

	// This method stub is aspirational. Use cases cover:
	// (i)  Implementing Unreal/Unreal-style "Play In Editor"
	// (ii) Taking and saving Snapshots as fine-grained savestate or MOD packs

	// Requires first the implementation first of a proper asset-cache/cooker
	// for serializing graphics/physics resources.

	return nullptr;
}