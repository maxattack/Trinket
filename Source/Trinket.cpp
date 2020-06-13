#include "Graphics.h"
#include "Assets.h"
#include "World.h"
#include "Scripting.h"
#include "Physics.h"

#include "imgui.h"
#include <vector>
#include "DiligentTools/ImGui/interface/ImGuiImplDiligent.hpp"
#include "DiligentTools/ImGui/interface/ImGuiUtils.hpp"
#include "imgui_impl_sdl.h"

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
	static Graphics gfx(&db, &world, window);

	// init IMGUI before the scene renderer :P
	ImGuiImplDiligent gui(gfx.GetDevice(), gfx.GetSwapChain()->GetDesc().ColorBufferFormat, gfx.GetSwapChain()->GetDesc().DepthBufferFormat);
	ImGuiIO& io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.IniFilename = nullptr;
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForD3D(window);

	gfx.InitSceneRenderer();

	static Physics phys(&db, &world);

	static ScriptVM vm(&db, &world, &input, &gfx, &phys);
	vm.RunScript("Assets/main.lua");

	auto selection = OBJECT_NIL;
	static char renameBuf[1024];
	renameBuf[0] = 0;

	for (bool done = false; !done;) {

		// handle events
		for (SDL_Event event; SDL_PollEvent(&event); ) {
			let isEndEvent = event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE);
			if (isEndEvent)
				done = true;
			input.HandleEvent(event);
			gfx.HandleEvent(event);
			ImGui_ImplSDL2_ProcessEvent(&event);
		}

		// tick
		input.Update();
		if (input.GetDeltaTicks() > 0) {
			vm.Tick();
			phys.Tick(input.GetDeltaTime());
		}

		ImGui_ImplSDL2_NewFrame(window);
		auto& SCDesc = gfx.GetSwapChain()->GetDesc();
		gui.NewFrame(SCDesc.Width, SCDesc.Height, SCDesc.PreTransform);

		static bool showDemoWindow = false;
		if (showDemoWindow)
			ImGui::ShowDemoWindow(&showDemoWindow);

		// outliner window
		{
			ImGui::SetNextWindowPos(ImVec2(25, 25), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(400, 680), ImGuiCond_FirstUseEver);
			ImGuiWindowFlags outlinerFlags(ImGuiWindowFlags_MenuBar);
			ImGui::Begin("Outliner", nullptr, outlinerFlags);

			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("Add")) {
					if (ImGui::MenuItem("Add New Sublevel")) {
						cout << "New Sublevel" << endl;
					}

					if (ImGui::MenuItem("Add Empty Object")) {
						cout << "New Object" << endl;
					}
					if (ImGui::MenuItem("Add Cube Primitive")) {
						cout << "Add Cube" << endl;
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Show")) {
					if (ImGui::MenuItem("Show IMGUI Demo")) {
						showDemoWindow = true;
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			for (int sublevelIdx = 0; sublevelIdx < world.GetSublevelCount(); ++sublevelIdx) {
				let pHierarchy = world.GetHierarchyByIndex(sublevelIdx);
				let hierarchyName = world.GetName(world.GetSublevelByIndex(sublevelIdx)).GetString();
				if (ImGui::CollapsingHeader(hierarchyName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

					let n = pHierarchy->Count();
					int32 depth = 0;
					auto itemClicked = OBJECT_NIL;
					auto reparentItem = OBJECT_NIL;
					auto reparentTarget = OBJECT_NIL;

					for (int it = 0; it < n;) {
						let id = pHierarchy->GetObjectByIndex(it);
						let hasChildren = pHierarchy->HasChildrenByIndex(it);
						let name = world.GetName(id).GetString();
						let selected = id == selection; //outlineSelection.Contains(id);

						let newDepth = pHierarchy->GetDepthByIndex(it);
						while (depth > newDepth) {
							ImGui::TreePop();
							--depth;
						}

						ImGuiTreeNodeFlags nodeFlags(
							ImGuiTreeNodeFlags_OpenOnArrow |
							//ImGuiTreeNodeFlags_OpenOnDoubleClick | 
							ImGuiTreeNodeFlags_SpanAvailWidth |
							ImGuiTreeNodeFlags_DefaultOpen
						);
						if (selected)
							nodeFlags |= ImGuiTreeNodeFlags_Selected;
						if (!hasChildren)
							nodeFlags |= ImGuiTreeNodeFlags_Leaf;

						let open = ImGui::TreeNodeEx(name.c_str(), nodeFlags);

						if (ImGui::IsItemClicked())
							itemClicked = id;

						ImGuiDragDropFlags dragFlags(0);
						if (ImGui::BeginDragDropSource(dragFlags)) {
							ImGui::SetDragDropPayload("OUTLINE_ITEM", &id, sizeof(ObjectID));
							ImGui::EndDragDropSource();
						}

						if (ImGui::BeginDragDropTarget()) {
							ImGuiDragDropFlags dropFlags(0);
							if (let payload = ImGui::AcceptDragDropPayload("OUTLINE_ITEM", dropFlags)) {
								reparentItem = *static_cast<ObjectID*>(payload->Data);
								reparentTarget = id;
							}
							ImGui::EndDragDropTarget();
						}

						if (open) {
							++it;
							++depth;
						} else {
							it = pHierarchy->GetDescendentRangeByIndex(it);
						}
					}
					while (depth > 0) {
						--depth;
						ImGui::TreePop();
					}
					if (!itemClicked.IsNil()) {

						if (world.IsValid(selection))
							if (let hierarchy = world.GetSublevelHierarchyFor(selection))
								hierarchy->SetRelativeScale(selection, vec3(1.f, 1.f, 1.f));

						selection = itemClicked;
						let name = world.GetName(selection).GetString();
						strcpy_s(renameBuf, 1024, name.c_str());

						if (world.IsValid(selection))
							if (let hierarchy = world.GetSublevelHierarchyFor(selection))
								hierarchy->SetRelativeScale(selection, vec3(1.2f, 1.2f, 1.2f));


					}
					if (!reparentItem.IsNil()) {
						pHierarchy->TryReparent(reparentItem, reparentTarget);
					}
				}
			}

			ImGui::End();
		}

		// inspector window
		{
			ImGui::SetNextWindowPos(ImVec2(1920 - 25 - 400, 25), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(400, 680), ImGuiCond_FirstUseEver);

			auto toDelete = OBJECT_NIL;

			ImGui::Begin("Inspector");

			if (world.IsValid(selection)) {
				ImGui::Text("Object #%d", selection);
				ImGui::InputText("", renameBuf, 1024);
				ImGui::SameLine();
				if (ImGui::Button("Rename") && renameBuf[0]) {
					world.TryRename(selection, Name(renameBuf));
				}
				if (ImGui::Button("Delete?"))
					toDelete = selection;

				if (let pHierarchy = world.GetSublevelHierarchyFor(selection)) {
					let rel = pHierarchy->GetWorldPose(selection);
					ImGui::LabelText("Position", "<%.2f, %.2f, %.2f>", rel->position.x, rel->position.y, rel->position.z);
				}

			} else {
				ImGui::Text("No Object Selected.");
			}


			ImGui::End();

			if (!toDelete.IsNil())
				world.TryReleaseObject(toDelete);
		}

		gui.EndFrame();

		gfx.Draw();
		gui.Render(gfx.GetContext());

		bool vsync = true;
		gfx.GetSwapChain()->Present(vsync ? 1 : 0);
	}

    return 0;
}
