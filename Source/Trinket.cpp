// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Display.h"
#include "Input.h"
#include "World.h"
#include "Scripting.h"
#include "Editor.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int main(int argc, char** argv) {
    using namespace std;

	// init sdl
	srand(clock());
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_GAMECONTROLLER) < 0) {
		cout << "[SDL] Init Error: " << SDL_GetError() << endl;
		return -1;
	}
	struct SDL_ScopeGuard { ~SDL_ScopeGuard() { SDL_Quit(); } };
	static SDL_ScopeGuard scope;

	// init globals
	static Display display("Trinket");
	static Input input;
	static World world(&display);
	static ScriptVM vm(&input, &world);
	#if TRINKET_EDITOR
	static Editor editor(&world);
	#endif

	// init content
	vm.RunScript("Assets/main.lua");

	// main loop
	// TODO: multithreading :P
	for (bool done = false; !done;) {

		// handle events
		for (SDL_Event event; SDL_PollEvent(&event); ) {
			let isEndEvent = event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE);
			if (isEndEvent)
				done = true;
			input.HandleEvent(event);
			display.HandleEvent(event);
			#if TRINKET_EDITOR
			editor.HandleEvent(event);
			#endif
		}

		// update
		input.Update();
		#if TRINKET_EDITOR
		editor.Update();
		#endif
		vm.Tick();
		if (input.GetDeltaTicks() > 0)
			world.phys.Tick(input.GetDeltaTime());

		// draw
		world.gfx.Draw();
		#if TRINKET_EDITOR
		editor.Draw();
		#endif

		display.Present();
	}

    return 0;
}

