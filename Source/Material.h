// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Name.h"
#include "Object.h"
#include "Display.h"
#include "AssetData.h"


struct MaterialAssetData : AssetDataHeader {
	static const schema_t SCHEMA = SCHEMA_MATERIAL;

	// TODO: multiple passes (see for ref: submeshes in MeshAssetDatas)

	uint32 VertexShaderNameOffset;
	uint32 PixelShaderNameOffset;
	uint32 TextureVariablesOffset;
	uint32 TextureCount;

	const char* VertexShaderPath() const { return Peek<char>(this, VertexShaderNameOffset); }
	const char* PixelShaderPath() const { return Peek<char>(this, PixelShaderNameOffset); }

	// Alternative name/path string pairs
	AssetDataReader TextureVariables() const { return AssetDataReader(this, TextureVariablesOffset); }
};

MaterialAssetData* ImportMaterialAssetDataFromSource(const char* configPath);

class MaterialPass {
private:
	RefCntAutoPtr<IPipelineState>         pMaterialPipelineState;
	RefCntAutoPtr<IShaderResourceBinding> pMaterialResourceBinding;

public:

	MaterialPass() noexcept = default;
	MaterialPass(MaterialPass && mesh) noexcept = default;

	MaterialPass(const MaterialPass&) = delete;
	MaterialPass& operator=(const MaterialPass&) = delete;
	
	bool IsLoaded() const { return pMaterialPipelineState; }
	bool TryLoad(Graphics* pGraphics, class Material* pCaller, const MaterialAssetData *pData, int Idx);
	bool TryUnload(Graphics* pGraphics);

	bool Bind(Graphics* pGraphics);
};

class Material : public ObjectComponent {
private:
	MaterialPass defaultMaterialPass;

public:

	Material(ObjectID id);

	int NumPasses() const { return 1; }
	MaterialPass& GetPass(int idx) { return defaultMaterialPass; }

	bool IsLoaded() const { return defaultMaterialPass.IsLoaded(); }
	bool TryLoad(Graphics* pGraphics, const MaterialAssetData* pData) { return defaultMaterialPass.TryLoad(pGraphics, this, pData, 0); }
	bool TryUnload(Graphics* pGraphics) { return defaultMaterialPass.TryUnload(pGraphics); }

};