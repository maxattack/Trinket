// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Display.h"
#include "World.h"
#include "Scripting.h"
#include "Editor.h"

#include "Triangulate.h"

#include <ofbx.h>
#include <cstdio>
#include <cstring>

// OFBX IMGUI sample, copied here as a test

ofbx::IScene* g_scene = nullptr;
const ofbx::IElement* g_selected_element = nullptr;
const ofbx::Object* g_selected_object = nullptr;

void LoadTestFBX() {
	let path = "Assets/mecha.fbx";
	FILE* pFile;
	if (fopen_s(&pFile, path, "rb"))
		return;
	fseek(pFile, 0, SEEK_END);
	let sz = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);
	let pContent = (uint8*) malloc(sz);
	fread(pContent, 1, sz, pFile);
	fclose(pFile);
	pFile = nullptr;
	g_scene = ofbx::load(pContent, sz, 0); //(uint64) ofbx::LoadFlags::TRIANGULATE);
	free(pContent);
}



template <int N>
void toString(ofbx::DataView view, char(&out)[N]) {
	int len = int(view.end - view.begin);
	if (len > sizeof(out) - 1) len = sizeof(out) - 1;
	strncpy_s(out, (const char*)view.begin, len);
	out[len] = 0;
}


int getPropertyCount(ofbx::IElementProperty* prop) {
	return prop ? getPropertyCount(prop->getNext()) + 1 : 0;
}


template <int N>
void catProperty(char(&out)[N], const ofbx::IElementProperty& prop) {
	char tmp[128];
	switch (prop.getType()) {
	case ofbx::IElementProperty::DOUBLE: sprintf_s(tmp, "%f", prop.getValue().toDouble()); break;
	case ofbx::IElementProperty::LONG: sprintf_s(tmp, "%" PRId64, prop.getValue().toU64()); break;
	case ofbx::IElementProperty::INTEGER: sprintf_s(tmp, "%d", prop.getValue().toInt()); break;
	case ofbx::IElementProperty::STRING: prop.getValue().toString(tmp); break;
	default: sprintf_s(tmp, "Type: %c", (char)prop.getType()); break;
	}
	strcat_s(out, tmp);
}


void showGUI(const ofbx::IElement& parent) {
	for (const ofbx::IElement* element = parent.getFirstChild(); element; element = element->getSibling()) {
		auto id = element->getID();
		char label[128];
		id.toString(label);
		strcat_s(label, " (");
		ofbx::IElementProperty* prop = element->getFirstProperty();
		bool first = true;
		while (prop) {
			if (!first)
				strcat_s(label, ", ");
			first = false;
			catProperty(label, *prop);
			prop = prop->getNext();
		}
		strcat_s(label, ")");

		ImGui::PushID((const void*)id.begin);
		ImGuiTreeNodeFlags flags = g_selected_element == element ? ImGuiTreeNodeFlags_Selected : 0;
		if (!element->getFirstChild()) flags |= ImGuiTreeNodeFlags_Leaf;
		if (ImGui::TreeNodeEx(label, flags)) {
			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) g_selected_element = element;
			if (element->getFirstChild()) showGUI(*element);
			ImGui::TreePop();
		} else {
			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) g_selected_element = element;
		}
		ImGui::PopID();
	}
}


template <typename T>
void showArray(const char* label, const char* format, ofbx::IElementProperty& prop) {
	if (!ImGui::CollapsingHeader(label)) return;

	int count = prop.getCount();
	ImGui::Text("Count: %d", count);
	std::vector<T> tmp;
	tmp.resize(count);
	prop.getValues(&tmp[0], int(sizeof(tmp[0]) * tmp.size()));
	for (T v : tmp) {
		ImGui::Text(format, v);
	}
}


void showGUI(ofbx::IElementProperty& prop) {
	ImGui::PushID((void*)&prop);
	char tmp[256];
	switch (prop.getType()) {
	case ofbx::IElementProperty::LONG: ImGui::Text("Long: %" PRId64, prop.getValue().toU64()); break;
	case ofbx::IElementProperty::FLOAT: ImGui::Text("Float: %f", prop.getValue().toFloat()); break;
	case ofbx::IElementProperty::DOUBLE: ImGui::Text("Double: %f", prop.getValue().toDouble()); break;
	case ofbx::IElementProperty::INTEGER: ImGui::Text("Integer: %d", prop.getValue().toInt()); break;
	case ofbx::IElementProperty::ARRAY_FLOAT: showArray<float>("float array", "%f", prop); break;
	case ofbx::IElementProperty::ARRAY_DOUBLE: showArray<double>("double array", "%f", prop); break;
	case ofbx::IElementProperty::ARRAY_INT: showArray<int>("int array", "%d", prop); break;
	case ofbx::IElementProperty::ARRAY_LONG: showArray<ofbx::u64>("long array", "%" PRId64, prop); break;
	case ofbx::IElementProperty::STRING:
		toString(prop.getValue(), tmp);
		ImGui::Text("String: %s", tmp);
		break;
	default:
		ImGui::Text("Other: %c", (char)prop.getType());
		break;
	}

	ImGui::PopID();
	if (prop.getNext()) showGUI(*prop.getNext());
}


static void showCurveGUI(const ofbx::Object& object) {
	const ofbx::AnimationCurve& curve = static_cast<const ofbx::AnimationCurve&>(object);

	const int c = curve.getKeyCount();
	for (int i = 0; i < c; ++i) {
		const float t = (float)ofbx::fbxTimeToSeconds(curve.getKeyTime()[i]);
		const float v = curve.getKeyValue()[i];
		ImGui::Text("%fs: %f ", t, v);

	}
}


void showObjectGUI(const ofbx::Object& object) {
	const char* label = "";
	switch (object.getType()) {
	case ofbx::Object::Type::GEOMETRY: label = "geometry"; break;
	case ofbx::Object::Type::MESH: label = "mesh"; break;
	case ofbx::Object::Type::MATERIAL: label = "material"; break;
	case ofbx::Object::Type::ROOT: label = "root"; break;
	case ofbx::Object::Type::TEXTURE: label = "texture"; break;
	case ofbx::Object::Type::NULL_NODE: label = "null"; break;
	case ofbx::Object::Type::LIMB_NODE: label = "limb node"; break;
	case ofbx::Object::Type::NODE_ATTRIBUTE: label = "node attribute"; break;
	case ofbx::Object::Type::CLUSTER: label = "cluster"; break;
	case ofbx::Object::Type::SKIN: label = "skin"; break;
	case ofbx::Object::Type::ANIMATION_STACK: label = "animation stack"; break;
	case ofbx::Object::Type::ANIMATION_LAYER: label = "animation layer"; break;
	case ofbx::Object::Type::ANIMATION_CURVE: label = "animation curve"; break;
	case ofbx::Object::Type::ANIMATION_CURVE_NODE: label = "animation curve node"; break;
	default: assert(false); break;
	}

	ImGuiTreeNodeFlags flags = g_selected_object == &object ? ImGuiTreeNodeFlags_Selected : 0;
	char tmp[128];
	sprintf_s(tmp, "%" PRId64 " %s (%s)", object.id, object.name, label);
	if (ImGui::TreeNodeEx(tmp, flags)) {
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) g_selected_object = &object;
		int i = 0;
		while (ofbx::Object* child = object.resolveObjectLink(i)) {
			showObjectGUI(*child);
			++i;
		}
		if (object.getType() == ofbx::Object::Type::ANIMATION_CURVE) {
			showCurveGUI(object);
		}

		ImGui::TreePop();
	} else {
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) g_selected_object = &object;
	}
}


void showObjectsGUI(const ofbx::IScene& scene) {
	if (!ImGui::Begin("Objects")) {
		ImGui::End();
		return;
	}
	const ofbx::Object* root = scene.getRoot();
	if (root) showObjectGUI(*root);

	int count = scene.getAnimationStackCount();
	for (int i = 0; i < count; ++i) {
		const ofbx::Object* stack = scene.getAnimationStack(i);
		showObjectGUI(*stack);
	}

	ImGui::End();
}

void showFbxGUI() {
	if (g_scene) {
		if (ImGui::Begin("Elements")) {
			const ofbx::IElement* root = g_scene->getRootElement();
			if (root && root->getFirstChild()) showGUI(*root);
		}
		ImGui::End();

		if (ImGui::Begin("Properties") && g_selected_element) {
			ofbx::IElementProperty* prop = g_selected_element->getFirstProperty();
			if (prop) showGUI(*prop);
		}
		ImGui::End();

		showObjectsGUI(*g_scene);
	}
}



//------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    using namespace std;

	LoadTestFBX();

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


	// transcode fbx dom to debugdraw list
	eastl::vector<mat4> matrices;
	eastl::vector<int16> parents;
	
	struct edge_data_t {
		union {
			struct {
				uint32 idx0;
				uint32 idx1;
			};
			uint64 word;
		};

		inline bool operator==(const edge_data_t& rhs) const { return word == rhs.word; }
	};
	struct mesh_data_t {
		uint16_t parentIdx;
		eastl::vector<vec3> points;
		eastl::vector<edge_data_t> edges;
	};
	eastl::vector<mesh_data_t> meshes;

	if (g_scene) {
		let isJoint = [](const ofbx::Object* obj) -> bool {
			switch(obj->getType()) {
			case ofbx::Object::Type::ROOT:
			case ofbx::Object::Type::NULL_NODE:
			case ofbx::Object::Type::LIMB_NODE:
				return true;
			default:
				return false;
			}
		};

		// count the number of joints
		eastl::vector<const ofbx::Object*> openSet;

		openSet.reserve(g_scene->getAllObjectCount());
		parents.reserve(g_scene->getAllObjectCount());
		openSet.push_back(g_scene->getRoot());
		parents.push_back(INVALID_INDEX);
		for (auto it = 0u; it < openSet.size(); ++it) {
			let itObject = openSet[it];
			int idx = 0;
			while(let child = itObject->resolveObjectLink(idx)) {
				if (isJoint(child)) {
					openSet.push_back(child);
					parents.push_back((int16) it);
				}
				++idx;
			}
		}

		let ConvertMatrix = [](mat4& result, const ofbx::Matrix& mat) -> mat4 {
			for (auto m = 0; m < 4; ++m) {
				let m4 = m << 2;
				result[m] = vec4(
					mat.m[m4],
					mat.m[m4 + 1],
					mat.m[m4 + 2],
					mat.m[m4 + 3]
				);
			}
			result[3] = 0.001f * result[3]; // unit conversion
			return result;
		};

		let ConvertVertexPosition = [](const ofbx::Vec3& v) -> vec3 {
			return 0.001f * vec3(v.x, v.y, v.z);
		};

		cout << "Node Count = " << openSet.size() << endl;
		matrices.resize(openSet.size());
		for(auto it = 0u; it<openSet.size(); ++it)
			ConvertMatrix(matrices[it], openSet[it]->getGlobalTransform());
		
		eastl::vector<const ofbx::Mesh*> meshSet;
		let nobj = g_scene->getAllObjectCount();
		let pobjbegin = g_scene->getAllObjects();
		let pobjend = pobjbegin + nobj;
		for(auto p = pobjbegin; p != pobjend; ++p) {
			let obj = *p;
			if (obj->getType() == ofbx::Object::Type::MESH) {
				meshSet.push_back((const ofbx::Mesh*) obj);
			}
		}

		meshes.resize(meshSet.size());
		for(auto it = 0u; it<meshSet.size(); ++it) {
			auto& mesh = meshes[it];

			let geo = meshSet[it]->getGeometry();
			bool bSkin = false;
			int gi=0;
			while(let obj = geo->resolveObjectLink(gi)) {
				++gi;
				if (obj->getType() == ofbx::Object::Type::SKIN) {
					bSkin = true;
					break;
				}
			}

			mat4 geoMatrix, nodeMatrix;
			ConvertMatrix(geoMatrix, meshSet[it]->getGeometricMatrix());
			ConvertMatrix(nodeMatrix, meshSet[it]->getGlobalTransform());

			let worldMatrix = nodeMatrix * geoMatrix;
			let nverts = geo->getVertexCount();
			let pverts = geo->getVertices();
			mesh.points.resize(nverts);
			for(int vit=0; vit<nverts; ++vit) {
				let localPos = vec4(ConvertVertexPosition(pverts[vit]), 1.f);
				mesh.points[vit] = bSkin ?  matrices[1] * localPos : worldMatrix * localPos;
			}
		
			let nidx = geo->getIndexCount();
			let pidx = geo->getFaceIndices();
			let TryAddEdge = [&](int x, int y) {
				if (x < 0) x = ~x;
				if (y < 0) y = ~y;
				edge_data_t edge;
				edge.idx0 = uint32(x < y ? x : y);
				edge.idx1 = uint32(x < y ? y : x);
				if (eastl::find(mesh.edges.begin(), mesh.edges.end(), edge) == mesh.edges.end())
					mesh.edges.push_back(edge);
			};


			// for triangulation
			vec3 posBuffer[MAX_EDGE_LOOP_LENGTH];
			uint32 idxBuffer[TRIANGULATE_RESULT_CAPACITY];
			let ToRealIndex = [](int idx) -> int { return idx >= 0 ? idx : ~idx; };

			int idx = 0;
			while(idx < nidx) {

				// search to the end of the next loop
				int loopLen = 1;
				while(pidx[idx + loopLen - 1] > 0)
					++loopLen;

				if (loopLen >= 3) {

					// triangulate the face (ugh, ugh)
					for(int loopIndex=0; loopIndex<loopLen; ++loopIndex)
						posBuffer[loopIndex] = mesh.points[ToRealIndex(pidx[idx + loopIndex])];

					Triangulate(idxBuffer, posBuffer, loopLen);

					let tricount = loopLen - 2;
					for(int triIndex=0; triIndex < tricount; ++triIndex) {
						let loopIdx1 = idx + idxBuffer[3 * triIndex + 0];
						let loopIdx2 = idx + idxBuffer[3 * triIndex + 1];
						let loopIdx3 = idx + idxBuffer[3 * triIndex + 2];
						TryAddEdge(pidx[loopIdx1], pidx[loopIdx2]);
						TryAddEdge(pidx[loopIdx2], pidx[loopIdx3]);
						TryAddEdge(pidx[loopIdx3], pidx[loopIdx1]);
					}

					//for(int loopIndex=1; loopIndex<loopLen; ++loopIndex)
					//	TryAddEdge(pidx[idx + loopIndex-1], pidx[idx + loopIndex]);
					//TryAddEdge(pidx[idx], pidx[idx + loopLen - 1]);

				}

				idx += loopLen;
			}

			cout << "Mesh: " << meshSet[it]->name << " EdgeCout: " << mesh.edges.size() << endl;
		}



	}

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
		showFbxGUI();
		editor.EndUpdate();
		#endif
		world.Update();

		
		//for(auto it=1u; it<matrices.size(); ++it) {
		//	if (parents[it] <= 1)
		//		continue;
		//	let prev = matrices[parents[it]][3];
		//	let next = matrices[it][3];
		//	world.gfx.DrawDebugLine(vec4(0.5f, 1.5f, 0.5f, 1.0f), prev, next);

		//}
		for(let& mesh : meshes) {
			for(let& edge : mesh.edges) {
				let& p0 = mesh.points[edge.idx0];
				let& p1 = mesh.points[edge.idx1];
				world.gfx.DrawDebugLine(vec4(0.5f, 1.5f, 0.5f, 1.0f), p0, p1);
			}
		}

		// draw
		world.gfx.Draw();
		#if TRINKET_EDITOR
		editor.Draw();
		#endif

		display.Present();
	}

	if (g_scene)
		g_scene->destroy();

    return 0;
}
