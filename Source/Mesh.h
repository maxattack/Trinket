#pragma once
#include "GraphicsPlatform.h"
#include "Object.h"
#include "Math.h"

struct MeshVertex {
	vec3 position;
	vec3 normal;
	vec4 color;
	vec2 uv;
};

extern const LayoutElement MeshVertexLayoutElems[4];

class SubMesh {
private:
	MeshVertex* vertices = nullptr;
	uint32* index = nullptr;
	RefCntAutoPtr<IBuffer> pVertexBuffer;
	RefCntAutoPtr<IBuffer> pIndexBuffer;
	int32 allocVertexCount = 0;
	int32 allocIndexCount = 0;
	int32 gpuVertexCount = 0;
	int32 gpuIndexCount = 0;
	uint32 dynamic : 1;

public:

	SubMesh() noexcept = default;
	SubMesh(SubMesh&& mesh) noexcept;
	SubMesh(const SubMesh&) = delete;
	SubMesh& operator=(const SubMesh&) = delete;
	~SubMesh();

	int32 GetAllocatedVertexCount() const { return allocVertexCount; }
	int32 GetAllocatedIndexCount() const { return allocIndexCount; }
	bool IsDynamic() const { return dynamic; }
	bool IsAllocated() const { return vertices != nullptr; }
	bool IsLoaded() const { return pVertexBuffer != nullptr; }

	void AllocVertices(int vertexCount);
	void AllocIndices(int indexCount);
	void Dealloc();

	MeshVertex* begin() { return vertices; }
	MeshVertex* end() { return vertices + allocVertexCount; }

	IBuffer* GetVertexBuffer() { return pVertexBuffer; }
	IBuffer* GetIndexBuffer() { return pIndexBuffer; }

	bool TryLoad(Graphics* gfx, bool dynamic = false);
	bool TryUpdate(Graphics* gfx);
	bool TryRelease(Graphics* gfx);
	void DoDraw(Graphics* gfx);


	void ReverseWindingOrder();
	void FlipNormals();
	void SetColor(vec4 c);
	void AllocPlaneXY(float extent);
	void AllocCube(float extent);

};

class Mesh : public ObjectComponent {
private:
	SubMesh defaultSubmesh;

public:

	Mesh(ObjectID aID) noexcept : ObjectComponent(aID) {}
	
	// more of an aspirational interface, here, lol
	int GetSubmeshCount() const { return 1; }
	SubMesh& GetSubmesh(int idx) { return defaultSubmesh; }
};
