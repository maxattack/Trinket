// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Name.h"
#include "Object.h"
#include "GraphicsPlatform.h"
#include <eastl/string.h>

struct TextureArg {
	const char* variableName;
	Texture* pTexture;
};

struct MaterialArgs {
	const char* vertexShaderFile;
	const char* pixelShaderFile;
	TextureArg* pTextureArgs;
	int numTextures;
};

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
	bool TryLoad(Graphics* pGraphics, class Material* pCaller, const MaterialArgs& args);
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
	bool TryLoad(Graphics* pGraphics, const MaterialArgs& args) { return defaultMaterialPass.TryLoad(pGraphics, this, args); }
	bool TryUnload(Graphics* pGraphics) { return defaultMaterialPass.TryUnload(pGraphics); }

};