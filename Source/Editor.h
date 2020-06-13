#pragma once

#include "Common.h"
#if TRINKET_EDITOR

#include <SDL.h>
#include "GraphicsPlatform.h"
#include "Object.h"
#include "imgui.h"
#include "DiligentTools/ImGui/interface/ImGuiImplDiligent.hpp"
#include "DiligentTools/ImGui/interface/ImGuiUtils.hpp"
#include "imgui_impl_sdl.h"


class AssetDatabase;
class World;
class Physics;
class Graphics;

class Editor {
private:
	AssetDatabase* pAssets;
	World* pWorld;
	Physics* pPhysics;
	Graphics* pGraphics;

	ImGuiImplDiligent impl;

	bool showDemoWindow = false;
	ObjectID selection = OBJECT_NIL;

	char renameBuf[1024];

public:

	Editor(Physics* aPhysics, Graphics *aGraphics);
	~Editor();

	void HandleEvent(const SDL_Event& Event);
	void Update();
	void Draw();

private:
	void ShowOutliner();
	void ShowInspector();

};

#endif