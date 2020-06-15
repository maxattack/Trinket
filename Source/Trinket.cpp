// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Assets.h"
#include "World.h"
#include "Physics.h"
#include "Animation.h"
#include "Graphics.h"
#include "Scripting.h"
#include "Editor.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int main(int argc, char** argv) {
    using namespace std;

	srand(clock());

	// init sdl
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_GAMECONTROLLER) < 0) {
		cout << "[SDL] Init Error: " << SDL_GetError() << endl;
		return -1;
	}

	let windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
	let window = SDL_CreateWindow("Trinket", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, windowFlags);
	if (window == nullptr) {
		cout << "[SDL] Create Window Error: " << SDL_GetError() << endl;
		getchar();
		return -1;
	}

	struct SDL_ScopeGuard {
		SDL_Window* pWindow;
		SDL_ScopeGuard(SDL_Window* aWindow) noexcept : pWindow(aWindow) {}
		~SDL_ScopeGuard() {
			SDL_DestroyWindow(pWindow);
			SDL_Quit();
		}
	};

	static SDL_ScopeGuard scope(window);
	static AssetDatabase db;
	static World world;
	static Input input;
	static SkelRegistry skel(&db, &world);
	static Physics phys(&db, &world);
	static AnimationRuntime anim(&skel);
	static Graphics gfx(&skel, window);
	static ScriptVM vm(&input, &gfx, &phys);
	#if TRINKET_EDITOR
	static Editor editor(&phys, &gfx);
	#endif

	// init
	gfx.InitSceneRenderer();
	vm.RunScript("Assets/main.lua");

	// main loop
	// TODO: separate gameplay and rendering (and, eventually, animation) threads
	for (bool done = false; !done;) {

		// handle events
		for (SDL_Event event; SDL_PollEvent(&event); ) {
			let isEndEvent = event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE);
			if (isEndEvent)
				done = true;
			input.HandleEvent(event);
			gfx.HandleEvent(event);
			#if TRINKET_EDITOR
			editor.HandleEvent(event);
			#endif
	
		}

		// update
		input.Update();
		#if TRINKET_EDITOR
		editor.Update();
		#endif
		if (input.GetDeltaTicks() > 0) {
			vm.Tick();
			phys.Tick(input.GetDeltaTime());
		}

		// draw
		gfx.Draw();
		#if TRINKET_EDITOR
		editor.Draw();
		#endif

		bool vsync = true;
		gfx.GetSwapChain()->Present(vsync ? 1 : 0);
	}

    return 0;
}

