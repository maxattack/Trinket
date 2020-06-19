// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Display.h"
#include "World.h"
#include "Scripting.h"
#include "Editor.h"

int main(int argc, char** argv) {
    using namespace std;

	if (let mat = LoadMaterialAssetDataFromConfig("Assets/checkerboard_m.ini")) {
		cout << "VSH: " << mat->VertexShaderPath() << endl;
		cout << "PSH: " << mat->PixelShaderPath() << endl;
		let N = mat->TextureCount;
		auto pTex = mat->GetTextureVariables();
		for(uint32 it=0; it<N; ++it) {
			let name = pTex;
			pTex += strlen(name) + 1;
			let path = pTex;
			pTex += strlen(path) + 1;
			cout << "Variable: " << name << " = " << path << endl;
		}
		FreeAssetData(mat);
	} else {
		cout << "No Dice" << endl;
	}

	if (let checkerboardData = LoadTextureAssetDataFromConfig("Assets/checkerboard_t.ini")) {
		cout << "Width = " << checkerboardData->TextureWidth << endl;
		cout << "Height = " << checkerboardData->TextureHeight << endl;
		cout << "Size = " << checkerboardData->ByteCount << endl;
		FreeAssetData(checkerboardData);
	} else {
		cout << "No Dice" << endl;
	}
	getchar();

	// init sdl
	srand(clock());
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_GAMECONTROLLER) < 0) {
		cout << "[SDL] Init Error: " << SDL_GetError() << endl;
		return -1;
	}
	struct SDL_ScopeGuard { ~SDL_ScopeGuard() { SDL_Quit(); } } scope;

	// init globals
	static Display display("Trinket");
	static World world(&display);
	#if TRINKET_EDITOR
	static Editor editor(&world);
	#endif

	// init content
	world.vm.RunScript("Assets/main.lua");

	// main loop
	// TODO: multithreading :P
	for (bool done = false; !done;) {

		// handle events
		for (SDL_Event event; SDL_PollEvent(&event); ) {
			let isEndEvent = event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE);
			if (isEndEvent)
				done = true;
			display.HandleEvent(event);
			#if TRINKET_EDITOR
			editor.HandleEvent(event);
			#endif
			world.HandleEvent(event);
		}

		// update
		#if TRINKET_EDITOR
		editor.Update();
		#endif
		world.Update();

		// draw
		world.gfx.Draw();
		#if TRINKET_EDITOR
		editor.Draw();
		#endif

		display.Present();
	}

    return 0;
}

