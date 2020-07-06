// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Display.h"
#include "World.h"
#include "Scripting.h"
#include "Editor.h"

#include "Geom.h"

int main(int argc, char** argv) {
    using namespace std;

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
	static Editor editor(&display, &world);
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
		editor.BeginUpdate();
		editor.EndUpdate();
		#endif
		world.Update();

		// draw
		world.gfx.Draw();
		display.ResolveMultisampling();
		#if TRINKET_EDITOR
		editor.Draw();
		#endif
		display.Present();
	}

    return 0;
}
