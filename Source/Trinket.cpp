// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Display.h"
#include "World.h"
#include "Scripting.h"
#include "Editor.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>

#include <glm/gtx/norm.hpp>

//------------------------------------------------------------------------------------------

void PrintMatrix(const aiMatrix4x4& m) {
	//printf("{ %.2f, %.2f, %.2f, %.2f }\n{ %.2f, %.2f, %.2f, %.2f }\n{ %.2f, %.2f, %.2f, %.2f }\n{ %.2f, %.2f, %.2f, %.2f }\n",
	//	m.a1, m.a2, m.a3, m.a4,
	//	m.b1, m.b2, m.b3, m.b4,
	//	m.c1, m.c2, m.c3, m.c4,
	//	m.d1, m.d2, m.d3, m.d4
	//);
	printf("{ %.2f %.2f %.2f }\n", m.a4, m.b4, m.c4);
}

int main(int argc, char** argv) {
    using namespace std;


	// test assimp
	struct line_t { vec3 start; vec3 end; };
	eastl::vector<line_t> debugLines;
	//{
	//	using namespace Assimp;
	//	//DefaultLogger::create("", Logger::VERBOSE, aiDefaultLogStream_STDOUT);
	//	
	//	Importer importer;
	//	let scene = importer.ReadFile(
	//		"Assets/mecha.fbx", 
	//		aiProcess_CalcTangentSpace         | 
	//		aiProcess_JoinIdenticalVertices    |
	//		aiProcess_MakeLeftHanded           |
	//		aiProcess_Triangulate              |
	//		aiProcess_RemoveRedundantMaterials |
	//		aiProcess_PopulateArmatureData     |
	//		aiProcess_SortByPType              |
	//		aiProcess_FindDegenerates          |
	//		aiProcess_FindInvalidData          |
	//		aiProcess_OptimizeMeshes           
	//		//aiProcess_OptimizeGraph            
	//	);
	//	if (scene) {
	//		mat4 importPose = HPose(
	//			quat(vec3(0,0,0)),//quat(vec3(glm::radians(90.f), 0.f, 0.f)), 
	//			vec3(0.f, 0.f, 0.f), 
	//			vec3(1.f, 1.f, 1.f)
	//		).ToMatrix();
	//		for(auto it=0u; it<scene->mNumMeshes; ++it) {
	//			let mesh = scene->mMeshes[it];
	//			for(auto fit=0u; fit<mesh->mNumFaces; ++fit) {
	//				let face = mesh->mFaces[fit];
	//				for(auto idx=1u; idx<face.mNumIndices; ++idx) {
	//					let p0 = reinterpret_cast<vec3&>(mesh->mVertices[face.mIndices[idx-1]]);
	//					let p1 = reinterpret_cast<vec3&>(mesh->mVertices[face.mIndices[idx]]);
	//					//debugLines.push_back(line_t { importPose * vec4(p0, 1), importPose * vec4(p1, 1) });
	//				}
	//			}
	//		}
	//		struct NodeIt {
	//			aiNode* node;
	//			uint depth;
	//			int parentIdx;
	//			aiMatrix4x4 toWorld;
	//		};
	//		eastl::vector<NodeIt> nodes;
	//		let s = .001f;
	//		aiMatrix4x4 importMatrix;
	//		aiMatrix4x4::Scaling(aiVector3D(s, s, s), importMatrix);
	//		nodes.push_back(NodeIt { scene->mRootNode, 0, -1, importMatrix * scene->mRootNode->mTransformation });
	//		for(auto it=0u; it < nodes.size();) {
	//			let node = nodes[it];
	//			
	//			for(auto cit = 0u; cit<node.node->mNumChildren; ++cit) {
	//				let child = node.node->mChildren[cit];
	//				let toParent = child->mTransformation;
	//				nodes.push_back(NodeIt { child, node.depth+1, (int) it,  node.toWorld * toParent });
	//			}
	//			
	//			for(auto d=0u; d<=node.depth; ++d)
	//				cout << "< ";
	//			cout << node.node->mName.C_Str() << endl;
	//			PrintMatrix(node.toWorld);
	//			++it;
	//		}
	//		for (auto it = 1u; it < nodes.size(); ++it) {
	//			let parentIdx = nodes[it].parentIdx;
	//			let parentToWorld = nodes[parentIdx].toWorld;
	//			let itToWorld = nodes[it].toWorld;
	//			let p0 = vec3(parentToWorld.a4, parentToWorld.b4, parentToWorld.c4);
	//			let p1 = vec3(itToWorld.a4, itToWorld.b4, itToWorld.c4);
	//			let skip = 
	//				glm::epsilonEqual(glm::length(p0), 0.f, .05f) || 
	//				glm::epsilonEqual(glm::length(p1), 0.f, .05f);
	//			if (skip)
	//				continue;
	//			debugLines.push_back(line_t { importPose * vec4(p0, 1), importPose * vec4(p1, 1) });
	//		}
	//	}
	//	//DefaultLogger::kill();
	//}
		


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
		editor.BeginUpdate();
		editor.EndUpdate();
		#endif
		world.Update();

		for(auto line : debugLines)
			world.gfx.DrawDebugLine(vec4(1.f, 1.f, 0.f, 1.f), line.start, line.end);

		// draw
		world.gfx.Draw();
		#if TRINKET_EDITOR
		editor.Draw();
		#endif

		display.Present();
	}

    return 0;
}
