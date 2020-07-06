// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "AssetData.h"
#include "Display.h"
#include "Geom.h"
#include "Math.h"
#include "Name.h"
#include "ObjectPool.h"

struct MeshVertex {
	vec3 position;
	vec3 normal;
	vec2 uv;
	union {
		uint32 color;
		struct {
			uint8 r;
			uint8 g;
			uint8 b;
			uint8 a;
		};
	};
};

AABB ComputeMeshAABB(MeshVertex* pVertices, uint count);

extern const LayoutElement MeshVertexLayoutElems[4];

struct SubmeshHeader {
	uint32 VertexCount;
	uint32 IndexCount;
	uint32 VertexOffset;
	uint32 IndexOffset;
};

struct MeshAssetData : AssetDataHeader {
	static const schema_t SCHEMA = SCHEMA_MESH;
	AABB BoundingBox;
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

MeshAssetData* ImportMeshAssetDataFromSource(const char* configPath);

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

	bool TryLoad(IRenderDevice* pDevice, bool dynamic, const MeshAssetData* pAsset, uint32 idx);
	bool TryLoad(IRenderDevice* pDevice, bool dynamic, uint nverts, uint nidx, const MeshVertex* pVertices, const uint32* pIndices);
	bool TryRelease(IRenderDevice* pDevice);
	void DoDraw(IDeviceContext* pContext);

};

class Mesh : public ObjectComponent {
private:
	AABB boundingBox;
	SubMesh defaultSubmesh;

public:

	Mesh(ObjectID aID) noexcept : ObjectComponent(aID) {}
	
	// more of an aspirational interface, here, lol
	AABB GetBoundingBox() const { return boundingBox; }
	int GetSubmeshCount() const { return 1; }
	SubMesh* GetSubmesh(int idx) { return idx == 0 ? &defaultSubmesh : nullptr; }
	
	bool TryLoad(IRenderDevice* pDevice, bool dynamic, const MeshAssetData* pAsset);
	bool TryLoad(IRenderDevice* pDevice, bool dynamic, uint nverts, uint nidx, const MeshVertex* pVertices, const uint32* pIndices, const AABB& bbox);

	void SetBoundingBox(const AABB& bbox) { boundingBox = bbox; }
};

class World;

class MeshRegistry {
public:

	MeshRegistry(World* aWorld) : pWorld(aWorld) {}

	Mesh* AddMesh(ObjectID id);
	Mesh* GetMesh(ObjectID id) { return DerefPP(meshes.TryGetComponent<1>(id)); }
	Mesh* FindMesh(Name path);

private:

	World* pWorld;
	ObjectPool<StrongRef<Mesh>> meshes;

};



struct MeshPlotter {

	// The scripting engine will keep a scratchpad plotter around
	// so that we don't have to keep reallocating memory when loading
	// procedural meshes from lua.

	// Usage:
	// (1) "Plot" Data
	// (2) Save to Asset Data or else Load/Update Directly

	eastl::vector<uint32> indices;
	eastl::vector<MeshVertex> vertices;

	void PlotCapsule(float halfHeight, float radius, uint radiusSampleCount, uint capRingCount, bool plotIndex = true);
	void PlotCube(float extent);
	void PlotPlane(float extent);
	void SetVertexColor(uint32 color);

	MeshAssetData* CreateAssetData();
	bool TryLoad(IRenderDevice *pDevice, Mesh* pMesh);
	bool TryUpdate(IRenderDevice *pDevice, Mesh* pMesh);
};

