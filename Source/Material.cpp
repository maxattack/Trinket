// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Material.h"
#include "Graphics.h"
#include "Texture.h"
#include <ini.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

MaterialDataHeader* LoadMaterialAssetDataFromConfig(const char* configPath) {

	struct TextureVarConfig {
		eastl::string variableName;
		eastl::string texturePath;
	};

	struct MaterialConfig {
		eastl::string vertexShaderPath;
		eastl::string pixelShaderPath;
		eastl::vector<TextureVarConfig> textureVariables;
	};

	let handler = [](void* user, const char* section, const char* name, const char* value) {
		auto pConfig = (MaterialConfig*)user;
		#define SECTION(s) strcmp(section, s) == 0
		#define MATCH(n) strcmp(name, n) == 0

		if (SECTION("Material")) {
			if (MATCH("vsh"))
				pConfig->vertexShaderPath = value;
			else if (MATCH("psh"))
				pConfig->pixelShaderPath = value;
		} else if (SECTION("Textures")) {
			pConfig->textureVariables.emplace_back(TextureVarConfig { name, value });
		}

		#undef SECTION
		#undef MATCH
		return 1;
	};

	MaterialConfig config;
	if (ini_parse(configPath, handler, &config))
		return nullptr;

	uint32 sz = 
		sizeof(MaterialDataHeader) + 
		config.textureVariables.size() * sizeof(uint32) + 
		uint32(config.vertexShaderPath.size()) + 1 + 
		uint32(config.pixelShaderPath.size()) + 1;
	for(auto it : config.textureVariables)
		sz += it.variableName.size() + it.texturePath.size() + 2;

	let result = (MaterialDataHeader*) AllocAssetData(sz);
	auto pStart = (uint8*) result;
	auto pCurr = pStart + sizeof(MaterialDataHeader);
	
	let WriteString = [&](const eastl::string& str) {
		memcpy(pCurr, str.c_str(), str.size());
		pCurr += str.size();
		*pCurr = 0;
		++pCurr;
	};

	result->VertexShaderNameOffset = uint32(pCurr - pStart);
	WriteString(config.vertexShaderPath);
	result->PixelShaderNameOffset = uint32(pCurr - pStart);
	WriteString(config.pixelShaderPath);
	result->TextureVariablesOffset = uint32(pCurr - pStart);
	result->TextureCount = (uint32) config.textureVariables.size();
	for(auto it : config.textureVariables) {
		WriteString(it.variableName);
		WriteString(it.texturePath);
	}

	return result;
}

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
