// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Mesh.h"
#include "Graphics.h"

#include <ini.h>
#include <EASTL/string.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>

const LayoutElement MeshVertexLayoutElems[4]{
	LayoutElement{ 0, 0, 3, VT_FLOAT32, false },
	LayoutElement{ 1, 0, 3, VT_FLOAT32, false },
	LayoutElement{ 2, 0, 2, VT_FLOAT32, false },
	LayoutElement{ 3, 0, 4, VT_UINT8,   true }
};

MeshAssetData* ImportMeshAssetDataFromSource(const char* configPath) {
	using namespace eastl::literals::string_literals;
	using namespace Assimp;

	struct MeshConfig {
		eastl::string path;
		float x = 0.f;
		float y = 0.f;
		float z = 0.f;
		float pitch = 0.f;
		float yaw = 0.f;
		float roll = 0.f;
		float scale = 1.f;
		bool includeSkinnedMeshes = true;
	};


	let handler = [](void* user, const char* section, const char* name, const char* value) {
		auto pConfig = (MeshConfig*)user;
		#define SECTION(s) (strcmp(section, s) == 0)
		#define MATCH(n) (strcmp(name, n) == 0)
		if (!SECTION("Mesh"))
			;
		else if (MATCH("path"))
			pConfig->path = value;
		else if (MATCH("x"))
			pConfig->x = strtof(value, nullptr);
		else if (MATCH("y"))
			pConfig->y = strtof(value, nullptr);
		else if (MATCH("z"))
			pConfig->z = strtof(value, nullptr);
		else if (MATCH("pitch"))
			pConfig->pitch = strtof(value, nullptr);
		else if (MATCH("yaw"))
			pConfig->yaw = strtof(value, nullptr);
		else if (MATCH("roll"))
			pConfig->roll = strtof(value, nullptr);
		else if (MATCH("scale"))
			pConfig->scale = strtof(value, nullptr);
		#undef SECTION
		#undef MATCH
		return 1;
	};

	MeshConfig config;
	let iniPath = "Assets/"s + configPath;
	if (ini_parse(iniPath.c_str(), handler, &config))
		return nullptr;

	config.path = "Assets/"s + config.path;

	let importTransform = HPose(
		quat(glm::radians(vec3(config.pitch, config.yaw, config.roll))),
		vec3(config.x, config.y, config.z),
		vec3(config.scale, config.scale, config.scale)
	).ToMatrix();

	Importer importer;

	if (!config.includeSkinnedMeshes) {

		// let ASSIMP pretransform vertices (won't include skinned meshes)
		let scene = importer.ReadFile(
			config.path.c_str(),
			aiProcess_PreTransformVertices     |
			aiProcess_CalcTangentSpace         |
			aiProcess_JoinIdenticalVertices    |
			aiProcess_MakeLeftHanded           |
			aiProcess_Triangulate              |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_SortByPType              |
			aiProcess_FindDegenerates          |
			aiProcess_FindInvalidData          |
			aiProcess_FlipUVs                  |
			aiProcess_FlipWindingOrder         |
			aiProcess_OptimizeMeshes
		);

		if (!scene || scene->mNumMeshes == 0)
			return nullptr;

		if (scene->mNumMeshes > 1)
			puts("[TODO] MULTI-SUBMESH IMPORT SUPPORT");
		

		let mesh = scene->mMeshes[0];

		let numVerts = mesh->mNumVertices;
		let numIndices = (3 * mesh->mNumFaces);

		let sz = uint32(
			sizeof(MeshAssetData) +
			sizeof(SubmeshHeader) +
			sizeof(MeshVertex) * numVerts +
			sizeof(uint32) * numIndices
		);

		let result = AllocAssetData<MeshAssetData>(sz);
		result->SubmeshCount = 1;

		AssetDataWriter writer(result, sizeof(MeshAssetData));
		auto pSubmesh = writer.PeekAndSeek<SubmeshHeader>();
		pSubmesh->IndexCount = numIndices;
		pSubmesh->VertexCount = numVerts;
		pSubmesh->VertexOffset = writer.GetOffset();

		for (uint32 it = 0; it < numVerts; ++it) {
			MeshVertex p;
			p.position = importTransform * vec4(FromAI(mesh->mVertices[it]), 1);
			p.uv = FromAI(mesh->mTextureCoords[0][it]);
			p.normal = FromAI(mesh->mNormals[it]);
			p.color = 0xffffffff; // TODO: Read Vertexc Color + Convert To Hex
			writer.WriteValue(p);
		}

		pSubmesh->IndexOffset = writer.GetOffset();
		for(uint32 it=0; it<mesh->mNumFaces; ++it) {
			let& face = mesh->mFaces[it];
			CHECK_ASSERT(face.mNumIndices == 3);
			writer.WriteValue(face.mIndices[0]);
			writer.WriteValue(face.mIndices[1]);
			writer.WriteValue(face.mIndices[2]);
		}

		return result;

	}

	//V2 - pretransform vertices ourselves, so we can 
	//     also include skinned meshes.
	let scene = importer.ReadFile(
		config.path.c_str(),
		aiProcess_CalcTangentSpace         |
		aiProcess_PopulateArmatureData     |
		aiProcess_JoinIdenticalVertices    |
		aiProcess_MakeLeftHanded           |
		aiProcess_Triangulate              |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_SortByPType              |
		aiProcess_FindDegenerates          |
		aiProcess_FindInvalidData          |
		aiProcess_FlipUVs                  |
		aiProcess_FlipWindingOrder         |
		aiProcess_OptimizeMeshes
	);
	if (!scene || scene->mNumMeshes == 0)
		return nullptr;


	if (scene->mNumMaterials > 1)
		puts("[TODO] MULTI-SUBMESH IMPORT SUPPORT");
	

	struct SceneItem {
		aiNode* pNode;
		mat4 toWorld;
	};

	eastl::vector<MeshVertex> vertices;
	eastl::vector<uint32> indices;
	eastl::vector<SceneItem> items;
	items.push_back(SceneItem { scene->mRootNode, importTransform * FromAI(scene->mRootNode->mTransformation) });
	for (uint32 currItem = 0; currItem < items.size(); ++currItem) {
		let item = items[currItem];

		for(uint32 it=0; it<item.pNode->mNumChildren; ++it) {
			let child = item.pNode->mChildren[it];
			let toParent = FromAI(child->mTransformation);
			items.push_back(SceneItem { child, item.toWorld * toParent });
		}

		let n = item.pNode->mNumMeshes;
		if (n == 0)
			continue;

		const mat3 normalMatrix = glm::inverseTranspose(item.toWorld);
		for(uint32 it=0; it<n; ++it) {
			let pMesh = scene->mMeshes[item.pNode->mMeshes[it]];
			
			let startIdx = uint32(vertices.size());
			vertices.reserve(startIdx + pMesh->mNumVertices);
			for(uint32 vit=0; vit<pMesh->mNumVertices; ++vit) {
				MeshVertex vtx;
				vtx.position = item.toWorld * vec4(FromAI(pMesh->mVertices[vit]), 1.f);
				vtx.normal = normalMatrix * FromAI(pMesh->mNormals[vit]);
				vtx.uv = FromAI(pMesh->mTextureCoords[0][vit]);
				vtx.color = 0xffffffff;
				vertices.push_back(vtx);
			}
			
			indices.reserve(indices.size() + 3 * pMesh->mNumFaces);
			for(uint32 fit=0; fit<pMesh->mNumFaces; ++fit) {
				let& face = pMesh->mFaces[fit];
				CHECK_ASSERT(face.mNumIndices == 3);
				indices.push_back(startIdx + face.mIndices[0]);
				indices.push_back(startIdx + face.mIndices[1]);
				indices.push_back(startIdx + face.mIndices[2]);
			}
		}
	}

	// add skinned meshes
	for (uint32 it = 0; it < scene->mNumMeshes; ++it) {
		let pMesh = scene->mMeshes[it];
		if (pMesh->mNumBones == 0)
			continue;

		let pRootNode = pMesh->mBones[0]->mArmature;
		mat4 modelMatrix = importTransform;

		for(let& item : items) {
			if (item.pNode == pRootNode) {
				modelMatrix = item.toWorld;
				break;
			}
		}

		const mat3 normalMatrix = glm::inverseTranspose(modelMatrix);

		let startIdx = uint32(vertices.size());
		vertices.reserve(startIdx + pMesh->mNumVertices);
		for (uint32 vit = 0; vit < pMesh->mNumVertices; ++vit) {
			MeshVertex vtx;
			vtx.position = modelMatrix * vec4(FromAI(pMesh->mVertices[vit]), 1.f);
			vtx.normal = normalMatrix * FromAI(pMesh->mNormals[vit]);
			vtx.uv = FromAI(pMesh->mTextureCoords[0][vit]);
			vtx.color = 0xffffffff;
			vertices.push_back(vtx);
		}

		indices.reserve(indices.size() + 3 * pMesh->mNumFaces);
		for (uint32 fit = 0; fit < pMesh->mNumFaces; ++fit) {
			let& face = pMesh->mFaces[fit];
			CHECK_ASSERT(face.mNumIndices == 3);
			indices.push_back(startIdx + face.mIndices[0]);
			indices.push_back(startIdx + face.mIndices[1]);
			indices.push_back(startIdx + face.mIndices[2]);
		}

	}

	let numVerts = uint32(vertices.size());
	let numIndices = uint32(indices.size());

	let sz = uint32(
		sizeof(MeshAssetData) +
		sizeof(SubmeshHeader) +
		sizeof(MeshVertex) * numVerts +
		sizeof(uint32) * numIndices
	);

	let result = AllocAssetData<MeshAssetData>(sz);
	result->SubmeshCount = 1;

	AssetDataWriter writer(result, sizeof(MeshAssetData));
	auto pSubmesh = writer.PeekAndSeek<SubmeshHeader>();
	pSubmesh->IndexCount = numIndices;
	pSubmesh->VertexCount = numVerts;

	pSubmesh->VertexOffset = writer.GetOffset();
	writer.WriteData(vertices.data(), sizeof(MeshVertex) * numVerts);
	
	pSubmesh->IndexOffset = writer.GetOffset();
	writer.WriteData(indices.data(), sizeof(uint32) * numIndices);

	return result;
}

MeshAssetData* CreateCubeMeshAssetData(float extent) {
	
	// Cube vertices
	//
	//      (-1,+1,+1)________________(+1,+1,+1)
	//               /|              /|
	//              / |             / |
	//             /  |            /  |
	//            /   |           /   |
	//(-1,-1,+1) /____|__________/(+1,-1,+1)
	//           |    |__________|____|
	//           |   /(-1,+1,-1) |    /(+1,+1,-1)
	//           |  /            |   /
	//           | /             |  /
	//           |/              | /
	//           /_______________|/
	//        (-1,-1,-1)       (+1,-1,-1)
	//
	
	struct CubeVertex {
		vec3 pos;
		vec2 uv;
		vec3 normal;
	};

	static const CubeVertex verts[] {
		{vec3(-1,-1,-1), vec2(0,1), vec3(0, 0, -1)},
		{vec3(-1,+1,-1), vec2(0,0), vec3(0, 0, -1)},
		{vec3(+1,+1,-1), vec2(1,0), vec3(0, 0, -1)},
		{vec3(+1,-1,-1), vec2(1,1), vec3(0, 0, -1)},
	
		{vec3(-1,-1,-1), vec2(0,1), vec3(0, -1, 0)},
		{vec3(-1,-1,+1), vec2(0,0), vec3(0, -1, 0)},
		{vec3(+1,-1,+1), vec2(1,0), vec3(0, -1, 0)},
		{vec3(+1,-1,-1), vec2(1,1), vec3(0, -1, 0)},
	
		{vec3(+1,-1,-1), vec2(0,1), vec3(+1, 0, 0)},
		{vec3(+1,-1,+1), vec2(1,1), vec3(+1, 0, 0)},
		{vec3(+1,+1,+1), vec2(1,0), vec3(+1, 0, 0)},
		{vec3(+1,+1,-1), vec2(0,0), vec3(+1, 0, 0)},
	
		{vec3(+1,+1,-1), vec2(0,1), vec3(0, +1, 0)},
		{vec3(+1,+1,+1), vec2(0,0), vec3(0, +1, 0)},
		{vec3(-1,+1,+1), vec2(1,0), vec3(0, +1, 0)},
		{vec3(-1,+1,-1), vec2(1,1), vec3(0, +1, 0)},
	
		{vec3(-1,+1,-1), vec2(1,0), vec3(-1, 0, 0)},
		{vec3(-1,+1,+1), vec2(0,0), vec3(-1, 0, 0)},
		{vec3(-1,-1,+1), vec2(0,1), vec3(-1, 0, 0)},
		{vec3(-1,-1,-1), vec2(1,1), vec3(-1, 0, 0)},
	
		{vec3(-1,-1,+1), vec2(1,1), vec3(0, 0, +1)},
		{vec3(+1,-1,+1), vec2(0,1), vec3(0, 0, +1)},
		{vec3(+1,+1,+1), vec2(0,0), vec3(0, 0, +1)},
		{vec3(-1,+1,+1), vec2(1,0), vec3(0, 0, +1)}
	};
	
	static const uint32 indices[] {
		2,0,1,    2,3,0,
		4,6,5,    4,7,6,
		8,10,9,   8,11,10,
		12,14,13, 12,15,14,
		16,18,17, 16,19,18,
		20,21,22, 20,22,23
	};
	
	let sz = 
		sizeof(MeshAssetData) + 
		sizeof(SubmeshHeader) + 
		sizeof(MeshVertex) * _countof(verts) + 
		sizeof(uint32) * _countof(indices);
	
	let result = AllocAssetData<MeshAssetData>(sz);
	result->SubmeshCount = 1;

	AssetDataWriter writer (result, sizeof(MeshAssetData));
	auto pSubmesh = writer.PeekAndSeek<SubmeshHeader>();
	pSubmesh->IndexCount = _countof(indices);
	pSubmesh->VertexCount = _countof(verts);
	pSubmesh->VertexOffset = writer.GetOffset();

	for(int it=0; it<24; ++it) {
		MeshVertex p;
		p.position = extent * verts[it].pos;
		p.uv = verts[it].uv;
		p.normal = verts[it].normal;
		p.color = 0xffffffff;
		writer.WriteValue(p);
	}

	pSubmesh->IndexOffset = writer.GetOffset();
	writer.WriteData(indices, sizeof(indices));

	return result;
}

MeshAssetData* CreatePlaneMeshAssetData(float extent) {

	let sz = 
		sizeof(MeshAssetData) + 
		sizeof(SubmeshHeader) + 
		sizeof(MeshVertex) * 4 + 
		sizeof(uint32) * 6;
	
	let result = AllocAssetData<MeshAssetData>(sz);
	result->SubmeshCount = 1;
	AssetDataWriter writer (result, sizeof(MeshAssetData));
	
	auto pSubmesh = writer.PeekAndSeek<SubmeshHeader>();
	pSubmesh->IndexCount = 6;
	pSubmesh->VertexCount = 4;
	pSubmesh->VertexOffset = writer.GetOffset();

	MeshVertex vertices[4] {
		{ vec3(-extent, -extent, 0), vec3(0, 0, 1), vec2(0, 0), 0xffffffff },
		{ vec3( extent, -extent, 0), vec3(0, 0, 1), vec2(1, 0), 0xffffffff },
		{ vec3( extent,  extent, 0), vec3(0, 0, 1), vec2(1, 1), 0xffffffff },
		{ vec3(-extent,  extent, 0), vec3(0, 0, 1), vec2(0, 1), 0xffffffff },
	};
	writer.WriteData(vertices, 4 * sizeof(MeshVertex));

	pSubmesh->IndexOffset = writer.GetOffset();
	uint32 indices[6] { 
		0,1,2, 
		3,0,2 
	};

	writer.WriteData(indices, 6 * sizeof(uint32));

	return result;
}

void MeshAssetData::ReverseWindingOrder() {
	for(uint32 sub=0; sub<SubmeshCount; ++sub) {
		let pSubmesh = SubmeshData(sub);
		if (pSubmesh->IndexCount > 0) {
			let pIndices = IndexData(sub);
			for(uint it=0; it<pSubmesh->IndexCount; it+=3)
				eastl::swap(pIndices[it+1], pIndices[it+2]);
		} else {
			let pVertices = VertexData(sub);
			for(uint it=0; it<pSubmesh->VertexCount; it+=3)
				eastl::swap(pVertices[it+1], pVertices[it+2]);
		}
	}
}

void MeshAssetData::FlipNormals() {
	for (uint32 sub = 0; sub < SubmeshCount; ++sub) {
		let pSubmesh = SubmeshData(sub);
		let pVertices = VertexData(sub);
		for(auto it=0u; it<pSubmesh->VertexCount; ++it)
			pVertices[it].normal = -pVertices[it].normal;
	}
}

void MeshAssetData::SetColor(vec4 c) {
	for (uint32 sub = 0; sub < SubmeshCount; ++sub) {
		let pSubmesh = SubmeshData(sub);
		let pVertices = VertexData(sub);
		for (auto it = 0u; it < pSubmesh->VertexCount; ++it)
			pVertices[it].color;
	}
}


bool SubMesh::TryLoad(Graphics* gfx, bool aDynamic, const MeshAssetData* pAsset, uint32 idx) {
	if (IsLoaded())
		return false;

	let pSubmesh = pAsset->SubmeshData(idx);
	let pVertices = pAsset->VertexData(idx);
	let pIndices = pAsset->IndexData(idx);

	gpuVertexCount = pSubmesh->VertexCount;
	gpuIndexCount = pSubmesh->IndexCount;
	dynamic = aDynamic;

	{
		let vertexByteCount = uint32(sizeof(MeshVertex) * gpuVertexCount);
		BufferDesc VBD;
		VBD.Name = "VB_Mesh"; // get name for mesh?
		VBD.Usage = aDynamic ? USAGE_DEFAULT : USAGE_STATIC;
		VBD.BindFlags = BIND_VERTEX_BUFFER;
		VBD.uiSizeInBytes = vertexByteCount;
		BufferData buf;
		buf.pData = pVertices;
		buf.DataSize = vertexByteCount;
		gfx->GetDevice()->CreateBuffer(VBD, &buf, &pVertexBuffer);
	}

	if (gpuIndexCount > 0) {
		let indexByteCount = uint32(sizeof(uint32) * gpuIndexCount);
		BufferDesc IBD;
		IBD.Name = "IB_Mesh";
		IBD.Usage = USAGE_STATIC;
		IBD.BindFlags = BIND_INDEX_BUFFER;
		IBD.uiSizeInBytes = indexByteCount;
		BufferData buf;
		buf.pData = pIndices;
		buf.DataSize = indexByteCount;
		gfx->GetDevice()->CreateBuffer(IBD, &buf, &pIndexBuffer);
	}
	
	return true;
}

bool SubMesh::TryRelease(Graphics* gfx) {
	// TODO
	return false;
}

void SubMesh::DoDraw(Graphics* gfx) {
	CHECK_ASSERT(IsLoaded());

	uint32 offset = 0;
	IBuffer* pBuffers[]{ pVertexBuffer };
	gfx->GetContext()->SetVertexBuffers(0, 1, pBuffers, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
	if (pIndexBuffer != nullptr) {
		gfx->GetContext()->SetIndexBuffer(pIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		DrawIndexedAttribs draw;
		draw.IndexType = VT_UINT32;
		draw.NumIndices = gpuIndexCount;
		#if _DEBUG
		draw.Flags = DRAW_FLAG_VERIFY_ALL;
		#endif
		gfx->GetContext()->DrawIndexed(draw);
	} else {
		DrawAttribs draw;
		draw.NumVertices = gpuVertexCount;
		gfx->GetContext()->Draw(draw);
	}
}
