#pragma once
#include "Name.h"
#include "Object.h"
#include "Diligent.h"
#include <eastl/string.h>

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
	bool TryLoad(Graphics* pGraphics, class Material* pCaller);
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
	bool TryLoad(Graphics* pGraphics) { return defaultMaterialPass.TryLoad(pGraphics, this); }
	bool TryUnload(Graphics* pGraphics) { return defaultMaterialPass.TryUnload(pGraphics); }

};