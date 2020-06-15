// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Material.h"
#include "Graphics.h"
#include "Texture.h"

Material::Material(ObjectID aID) : ObjectComponent(aID) {}

bool MaterialPass::TryLoad(Graphics* pGraphics, Material* pCaller, const MaterialArgs& args) {
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
		SCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
		SCI.EntryPoint = "main";
		SCI.Desc.Name = vsDescName.c_str();
		SCI.FilePath = args.vertexShaderFile;
		pDevice->CreateShader(SCI, &pVS);
		if (!pVS)
			return false;
	}
	RefCntAutoPtr<IShader> pPS;
	{
		let psDescName = "PS_" + nameStr;
		SCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
		SCI.EntryPoint = "main";
		SCI.Desc.Name = psDescName.c_str();
		SCI.FilePath = args.pixelShaderFile;
		pDevice->CreateShader(SCI, &pPS);
		if (!pPS)
			return false;
	}

	PSODesc.GraphicsPipeline.InputLayout.LayoutElements = MeshVertexLayoutElems;
	PSODesc.GraphicsPipeline.InputLayout.NumElements = _countof(MeshVertexLayoutElems);
	PSODesc.GraphicsPipeline.pVS = pVS;
	PSODesc.GraphicsPipeline.pPS = pPS;

	PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	CHECK_ASSERT(args.numTextures < 16);

	ShaderResourceVariableDesc Vars[16];
	Vars[0] = { SHADER_TYPE_PIXEL, "g_ShadowMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE };
	for(int it=0; it<args.numTextures; ++it)
		Vars[it+1] = { SHADER_TYPE_PIXEL, args.pTextureArgs[it].variableName, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE };
	PSODesc.ResourceLayout.Variables = Vars;
	PSODesc.ResourceLayout.NumVariables = args.numTextures + 1;

	// define static comparison sampler for shadow map
	SamplerDesc ComparisonSampler;
	ComparisonSampler.ComparisonFunc = COMPARISON_FUNC_LESS;
	ComparisonSampler.MinFilter = FILTER_TYPE_COMPARISON_LINEAR;
	ComparisonSampler.MagFilter = FILTER_TYPE_COMPARISON_LINEAR;
	ComparisonSampler.MipFilter = FILTER_TYPE_COMPARISON_LINEAR;


	StaticSamplerDesc StaticSamplers[16];
	StaticSamplers[0] = { SHADER_TYPE_PIXEL, "g_ShadowMap", ComparisonSampler };
	for(int it=0; it<args.numTextures; ++it)
		StaticSamplers[it+1] = { SHADER_TYPE_PIXEL, args.pTextureArgs[it].variableName, ComparisonSampler };
	PSODesc.ResourceLayout.StaticSamplers = StaticSamplers;
	PSODesc.ResourceLayout.NumStaticSamplers = args.numTextures + 1;

	pDevice->CreatePipelineState(Args, &pMaterialPipelineState);
	if (!pMaterialPipelineState)
		return false;

	pMaterialPipelineState->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(pGraphics->GetRenderConstants());
	pMaterialPipelineState->CreateShaderResourceBinding(&pMaterialResourceBinding, true);

	if (let pShadowMapVar = pMaterialResourceBinding->GetVariableByName(SHADER_TYPE_PIXEL, "g_ShadowMap"))
		pShadowMapVar->Set(pGraphics->GetShadowMapSRV());

	for(int it=0; it<args.numTextures; ++it)
		if (let pVar = pMaterialResourceBinding->GetVariableByName(SHADER_TYPE_PIXEL, args.pTextureArgs[it].variableName))
			pVar->Set(args.pTextureArgs[it].pTexture->GetSRV());

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
