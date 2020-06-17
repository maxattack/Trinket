// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Input.h"
#include "World.h"

class ScriptVM {
private:
	Input* pInput;
	World* pWorld;
	struct lua_State* lua;

public:

	ScriptVM(Input* aInput, World* pWorld);
	~ScriptVM();

	World* GetWorld() { return pWorld; }
	Input* GetInput() { return pInput; }

	void RunScript(const char* path);
	void Tick();

#if TRINKET_TEST
	vec3 wireframePosition = vec3(0, 0, 0);
	vec4 wireframeColor = vec4(1, 1, 1, 1);
	quat wireframeRotation = quat(1,0,0,0);
#endif

};
