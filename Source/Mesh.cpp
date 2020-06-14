// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Mesh.h"
#include "Graphics.h"

const LayoutElement MeshVertexLayoutElems[4] {
	LayoutElement{ 0, 0, 3, VT_FLOAT32, false },
	LayoutElement{ 1, 0, 3, VT_FLOAT32, false },
	LayoutElement{ 2, 0, 2, VT_FLOAT32, false },
	LayoutElement{ 3, 0, 4, VT_FLOAT32, false },
};


SubMesh::SubMesh(SubMesh&& mesh) noexcept 
	: vertices(mesh.vertices)
	, index(mesh.index)
	, pVertexBuffer(std::move(mesh.pVertexBuffer))
	, pIndexBuffer(std::move(mesh.pIndexBuffer))
	, allocVertexCount(mesh.allocVertexCount)
	, allocIndexCount(mesh.allocIndexCount)
	, gpuVertexCount(mesh.gpuVertexCount)
	, gpuIndexCount(mesh.gpuIndexCount)
	, dynamic(mesh.dynamic)
{
	mesh.vertices = nullptr;
	mesh.index = nullptr;
}

SubMesh::~SubMesh() {
	free(vertices);
	free(index);
}

void SubMesh::AllocVertices(int vertexCount) {
	allocVertexCount = vertexCount;
	let newVertices = (MeshVertex*) realloc(vertices, vertexCount * sizeof(MeshVertex));
	DEBUG_ASSERT(newVertices != nullptr);
	vertices = newVertices;
}

void SubMesh::AllocIndices(int indexCount) {
	allocIndexCount = indexCount;
	let newIndex = (uint32*) realloc(index, indexCount * sizeof(uint32));
	DEBUG_ASSERT(newIndex != nullptr);
	index = newIndex;
}

void SubMesh::Dealloc() {
	free(vertices);
	free(index);
	vertices = nullptr;
	index = nullptr;
	allocVertexCount = 0;
	allocIndexCount = 0;
}

bool SubMesh::TryLoad(Graphics* gfx, bool aDynamic) {
	let statusOK = !IsLoaded() && IsAllocated();
	if (!statusOK)
		return false;

	gpuVertexCount = allocVertexCount;
	gpuIndexCount = allocIndexCount;
	dynamic = aDynamic;

	{
		let vertexByteCount = static_cast<Uint32>(sizeof(MeshVertex) * allocVertexCount);
		BufferDesc VBD;
		VBD.Name = "VB_Mesh"; // get name for mesh?
		VBD.Usage = aDynamic ? USAGE_DEFAULT : USAGE_STATIC;
		VBD.BindFlags = BIND_VERTEX_BUFFER;
		VBD.uiSizeInBytes = vertexByteCount;
		BufferData buf;
		buf.pData = vertices;
		buf.DataSize = vertexByteCount;
		gfx->GetDevice()->CreateBuffer(VBD, &buf, &pVertexBuffer);
	}

	if (gpuIndexCount > 0) {
		let indexByteCount = static_cast<Uint32>(sizeof(uint32) * allocIndexCount);
		BufferDesc IBD;
		IBD.Name = "IB_Mesh";
		IBD.Usage = USAGE_STATIC;
		IBD.BindFlags = BIND_INDEX_BUFFER;
		IBD.uiSizeInBytes = indexByteCount;
		BufferData buf;
		buf.pData = index;
		buf.DataSize = indexByteCount;
		gfx->GetDevice()->CreateBuffer(IBD, &buf, &pIndexBuffer);
	}
	
	return true;
}

bool SubMesh::TryUpdate(Graphics* gfx) {
	// TODO
	return false;
}

bool SubMesh::TryRelease(Graphics* gfx) {
	// TODO
	return false;
}

void SubMesh::DoDraw(Graphics* gfx) {
	DEBUG_ASSERT(IsLoaded());

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

void SubMesh::ReverseWindingOrder() {
	if (index) {
		for (int it=0; it<allocIndexCount; it+=3)
			std::swap(index[it + 1], index[it + 2]);
	} else if (vertices) {
		for(int it=0; it<allocVertexCount; it+=3)
			std::swap(vertices[it+1], vertices[it+2]);
	}
}

void SubMesh::FlipNormals() {
}

void SubMesh::SetColor(vec4 c) {
	for(auto& it : *this) {
		it.color = c;
	}
}

void SubMesh::AllocPlaneXY(float extent) {
	AllocVertices(4);
	AllocIndices(6);

	vertices[0] = { vec3(-extent, -extent, 0), vec3(0, 0, 1), vec2(0, 0), vec4(1, 1, 1, 1) };
	vertices[1] = { vec3( extent, -extent, 0), vec3(0, 0, 1), vec2(1, 0), vec4(1, 1, 1, 1) };
	vertices[2] = { vec3( extent,  extent, 0), vec3(0, 0, 1), vec2(1, 1), vec4(1, 1, 1, 1) };
	vertices[3] = { vec3(-extent,  extent, 0), vec3(0, 0, 1), vec2(0, 1), vec4(1, 1, 1, 1) };

	index[0] = 0;
	index[1] = 1;
	index[2] = 2;
	index[3] = 3;
	index[4] = 0;
	index[5] = 2;
}

void SubMesh::AllocCube(float extent) {

	struct CubeVertex
	{
		vec3 pos;
		vec2 uv;
		vec3 normal;
	};

	// Cube vertices

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

	// clang-format off
	CubeVertex CubeVerts[] {
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

	Uint32 Indices[] {
		2,0,1,    2,3,0,
		4,6,5,    4,7,6,
		8,10,9,   8,11,10,
		12,14,13, 12,15,14,
		16,18,17, 16,19,18,
		20,21,22, 20,22,23
	};

	AllocVertices(24);
	AllocIndices(_countof(Indices));

	for(auto it=0; it<24; ++it) {
		auto& p = vertices[it];
		p.position = extent * CubeVerts[it].pos;
		p.uv = CubeVerts[it].uv;
		p.normal = CubeVerts[it].normal;
		p.color = vec4(1, 1, 1, 1);
	}

	memcpy(index, Indices, sizeof(Indices));
}
