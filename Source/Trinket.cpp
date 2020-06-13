#include "Graphics.h"
#include "Assets.h"
#include "World.h"
#include "Scripting.h"
#include "Physics.h"
#include "Editor.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int main(int argc, char** argv) {
    using namespace std;

	srand(clock());

	// init sdl
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_GAMECONTROLLER) < 0) {
		cout << "SDL Init Error: " << SDL_GetError() << endl;
		return -1;
	}

	let windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
	let window = SDL_CreateWindow("Trinket", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, windowFlags);
	if (window == nullptr) {
		cout << "SDL Create Window Error: " << SDL_GetError() << endl;
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
	static Physics phys(&db, &world);
	static Graphics gfx(&db, &world, window);
	static Editor editor(&db, &world, &phys, &gfx);
	static ScriptVM vm(&db, &world, &input, &gfx, &phys);

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
			editor.HandleEvent(event);
		
		}

		// update
		input.Update();
		editor.Update();
		if (input.GetDeltaTicks() > 0) {
			vm.Tick();
			phys.Tick(input.GetDeltaTime());
		}

		// draw
		gfx.Draw();
		editor.Draw();
		bool vsync = true;
		gfx.GetSwapChain()->Present(vsync ? 1 : 0);
	}

    return 0;
}
