// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Display.h"
#include "Object.h"
#include "Math.h"
#include "AssetData.h"

struct MeshVertex {
	vec3 position;
	vec3 normal;
	vec2 uv;
	vec4 color; // encode as 8bpc instead?
};

extern const LayoutElement MeshVertexLayoutElems[4];

struct SubmeshHeader {
	uint32 VertexCount;
	uint32 IndexCount;
	uint32 VertexOffset;
	uint32 IndexOffset;
};

struct MeshAssetData : AssetDataHeader {
	static const schema_t SCHEMA = SCHEMA_MESH;

	uint32 SubmeshCount;

	// Const Getters
	const SubmeshHeader* SubmeshData(uint32 Idx) const { return Peek<SubmeshHeader>(this, sizeof(MeshAssetData) + Idx * sizeof(SubmeshHeader)); }
	const MeshVertex* VertexData(uint32 Idx) const { return Peek<MeshVertex>(this, SubmeshData(Idx)->VertexOffset); }
	const uint32* IndexData(uint32 Idx) const { return Peek<uint32>(this, SubmeshData(Idx)->IndexOffset); }

	// Helper Modifiers
	SubmeshHeader* SubmeshData(uint32 Idx) { return Peek<SubmeshHeader>(this, sizeof(MeshAssetData) + Idx * sizeof(SubmeshHeader)); }
	MeshVertex* VertexData(uint32 Idx) { return Peek<MeshVertex>(this, SubmeshData(Idx)->VertexOffset); }
	uint32* IndexData(uint32 Idx) { return Peek<uint32>(this, SubmeshData(Idx)->IndexOffset); }

	void ReverseWindingOrder();
	void FlipNormals();
	void SetColor(vec4 c);

};

MeshAssetData* ImportMeshAssetDataFromConfig(const char* configPath);
MeshAssetData* CreateCubeMeshAssetData(float extent);
MeshAssetData* CreatePlaneMeshAssetData(float extent);


class SubMesh {
private:
	RefCntAutoPtr<IBuffer> pVertexBuffer;
	RefCntAutoPtr<IBuffer> pIndexBuffer;
	uint32 gpuVertexCount = 0;
	uint32 gpuIndexCount = 0;
	uint32 dynamic : 1;

public:

	bool IsDynamic() const { return dynamic; }
	bool IsLoaded() const { return pVertexBuffer != nullptr; }

	IBuffer* GetVertexBuffer() { return pVertexBuffer; }
	IBuffer* GetIndexBuffer() { return pIndexBuffer; }

	bool TryLoad(Graphics* gfx, bool dynamic, const MeshAssetData* pAsset, uint32 idx);
	bool TryRelease(Graphics* gfx);
	void DoDraw(Graphics* gfx);


};

class Mesh : public ObjectComponent {
private:
	SubMesh defaultSubmesh;

public:

	Mesh(ObjectID aID) noexcept : ObjectComponent(aID) {}
	
	// more of an aspirational interface, here, lol
	int GetSubmeshCount() const { return 1; }
	SubMesh& GetSubmesh(int idx) { return defaultSubmesh; }
	bool TryLoad(Graphics* gfx, bool dynamic, const MeshAssetData* pAsset) { return defaultSubmesh.TryLoad(gfx, dynamic, pAsset, 0); }

};
