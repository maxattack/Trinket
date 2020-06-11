#include "Material.h"
#include "Graphics.h"

Material::Material(ObjectID aID) : ObjectComponent(aID) {}

bool MaterialPass::TryLoad(Graphics* pGraphics, Material* pCaller) {
	if (IsLoaded())
		return true;

	let nameStr = pGraphics->GetAssets()->GetName(pCaller->ID()).GetString();
	let pDevice = pGraphics->GetDevice();
	let pSwapChain = pGraphics->GetSwapChain();

	PipelineStateCreateInfo Args;
	auto& PSODesc = Args.PSODesc;

	let descName = "PSO_" + nameStr;
	PSODesc.Name = descName.c_str();

	PSODesc.IsComputePipeline = false;
	PSODesc.GraphicsPipeline.NumRenderTargets = 1;
	PSODesc.GraphicsPipeline.RTVFormats[0] = pSwapChain->GetDesc().ColorBufferFormat;
	PSODesc.GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;
	PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	PSODesc.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
	PSODesc.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;

	ShaderCreateInfo SCI;
	SCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
	SCI.UseCombinedTextureSamplers = true; // For GL Compat
	SCI.pShaderSourceStreamFactory = pGraphics->GetShaderSourceStream();
	RefCntAutoPtr<IShader> pVS;
	{
		let vsDescName = "VS_" + nameStr;
		let vsFilename = nameStr + ".vsh";
		SCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
		SCI.EntryPoint = "main";
		SCI.Desc.Name = vsDescName.c_str();
		SCI.FilePath = vsFilename.c_str();
		pDevice->CreateShader(SCI, &pVS);
		if (!pVS)
			return false;
	}
	RefCntAutoPtr<IShader> pPS;
	{
		let psDescName = "PS_" + nameStr;
		let psFilename = nameStr + ".psh";
		SCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
		SCI.EntryPoint = "main";
		SCI.Desc.Name = psDescName.c_str();
		SCI.FilePath = psFilename.c_str();
		pDevice->CreateShader(SCI, &pPS);
		if (!pPS)
			return false;
	}

	PSODesc.GraphicsPipeline.InputLayout.LayoutElements = MeshVertexLayoutElems;
	PSODesc.GraphicsPipeline.InputLayout.NumElements = _countof(MeshVertexLayoutElems);
	PSODesc.GraphicsPipeline.pVS = pVS;
	PSODesc.GraphicsPipeline.pPS = pPS;

	PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
	ShaderResourceVariableDesc Vars[]{
		{ SHADER_TYPE_PIXEL, "g_ShadowMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE }
	};
	PSODesc.ResourceLayout.Variables = Vars;
	PSODesc.ResourceLayout.NumVariables = _countof(Vars);

	// define static comparison sampler for shadow map
	SamplerDesc ComparisonSampler;
	ComparisonSampler.ComparisonFunc = COMPARISON_FUNC_LESS;
	ComparisonSampler.MinFilter = FILTER_TYPE_COMPARISON_LINEAR;
	ComparisonSampler.MagFilter = FILTER_TYPE_COMPARISON_LINEAR;
	ComparisonSampler.MipFilter = FILTER_TYPE_COMPARISON_LINEAR;
	StaticSamplerDesc StaticSamplers[]{
		{ SHADER_TYPE_PIXEL, "g_ShadowMap", ComparisonSampler }
	};
	PSODesc.ResourceLayout.StaticSamplers = StaticSamplers;
	PSODesc.ResourceLayout.NumStaticSamplers = _countof(StaticSamplers);


	pDevice->CreatePipelineState(Args, &pMaterialPipelineState);
	if (!pMaterialPipelineState)
		return false;

	pMaterialPipelineState->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(pGraphics->GetRenderConstants());
	pMaterialPipelineState->CreateShaderResourceBinding(&pMaterialResourceBinding, true);

	pMaterialResourceBinding->GetVariableByName(SHADER_TYPE_PIXEL, "g_ShadowMap")->Set(pGraphics->GetShadowMapSRV());
	return true;
}

bool MaterialPass::TryUnload(Graphics* pGraphics) {
	// TODO
	return false;
}

bool MaterialPass::Bind(Graphics* pGraphics) {
	if (!IsLoaded())
		return false;
	let pContext = pGraphics->GetContext();
	pContext->SetPipelineState(pMaterialPipelineState);
	pContext->CommitShaderResources(pMaterialResourceBinding, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	return true;
}
