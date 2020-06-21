// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Mesh.h"
#include "Graphics.h"

const LayoutElement MeshVertexLayoutElems[4]{
	LayoutElement{ 0, 0, 3, VT_FLOAT32, false },
	LayoutElement{ 1, 0, 3, VT_FLOAT32, false },
	LayoutElement{ 2, 0, 2, VT_FLOAT32, false },
	LayoutElement{ 3, 0, 4, VT_UINT8,   true }
};


MeshAssetData* ImportMeshAssetDataFromSource(const char* configPath) {
	// LOL TODO
	return nullptr;
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
		for(uint32 it=0; it<pSubmesh->VertexCount; ++it)
			pVertices[it].normal = -pVertices[it].normal;
	}
}

void MeshAssetData::SetColor(vec4 c) {
	for (uint32 sub = 0; sub < SubmeshCount; ++sub) {
		let pSubmesh = SubmeshData(sub);
		let pVertices = VertexData(sub);
		for (uint32 it = 0; it < pSubmesh->VertexCount; ++it)
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
