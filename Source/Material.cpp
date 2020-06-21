// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Material.h"
#include "Graphics.h"
#include "Texture.h"
#include <ini.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

MaterialAssetData* ImportMaterialAssetDataFromSource(const char* configPath) {
	using namespace eastl::literals::string_literals;

	struct TextureVarConfig {
		eastl::string variableName;
		eastl::string texturePath;
	};

	struct MaterialConfig {
		bool hasMaterialSection = false;
		eastl::string vertexShaderPath;
		eastl::string pixelShaderPath;
		eastl::vector<TextureVarConfig> textureVariables;
	};

	let handler = [](void* user, const char* section, const char* name, const char* value) {
		auto pConfig = (MaterialConfig*)user;
		#define SECTION(s) strcmp(section, s) == 0
		#define MATCH(n) strcmp(name, n) == 0

		if (SECTION("Material")) {
			pConfig->hasMaterialSection = true;
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

	// Load Config
	MaterialConfig config;

	let iniPath = "Assets/"s + configPath;

	if (ini_parse(iniPath.c_str(), handler, &config))
		return nullptr;

	if (!config.hasMaterialSection)
		return nullptr;

	// TODO: validate paths

	// Compute Blob Size
	uint32 sz = 
		sizeof(MaterialAssetData) + 
		config.textureVariables.size() * sizeof(uint32) + 
		StrByteCount(config.vertexShaderPath) + 
		StrByteCount(config.pixelShaderPath);
	for(auto it : config.textureVariables)
		sz +=
			StrByteCount(it.variableName) + 
			StrByteCount(it.texturePath);

	let result = AllocAssetData<MaterialAssetData>(sz);
	AssetDataWriter writer(result, sizeof(MaterialAssetData));
	
	result->VertexShaderNameOffset = writer.GetOffset();
	writer.WriteString(config.vertexShaderPath);
	
	result->PixelShaderNameOffset = writer.GetOffset();
	writer.WriteString(config.pixelShaderPath);
	
	result->TextureVariablesOffset = writer.GetOffset();
	result->TextureCount = (uint32) config.textureVariables.size();
	for(auto it : config.textureVariables) {
		writer.WriteString(it.variableName);
		writer.WriteString(it.texturePath);
	}

	return result;
}

Material::Material(ObjectID aID) : ObjectComponent(aID) {}

bool MaterialPass::TryLoad(Graphics* pGraphics, class Material* pCaller, const MaterialAssetData *pData, int Idx) {
	if (IsLoaded())
		return true;

	CHECK_ASSERT(pData->TextureCount < 16);
	struct TextureVariable {
		const char* name;
		ITexture* pTexture;
	};
	TextureVariable tv[15];
	{
		auto reader = pData->TextureVariables();
		for (uint32 it = 0; it < pData->TextureCount; ++it) {
			let name = reader.ReadString();
			let path = reader.ReadString();
			let texture = pGraphics->FindTexture(path);
			if (texture == nullptr)
				return false;

			tv[it].name = name;
			tv[it].pTexture = texture;
		}
	}


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
		SCI.FilePath = pData->VertexShaderPath();
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
		SCI.FilePath = pData->PixelShaderPath();
		pDevice->CreateShader(SCI, &pPS);
		if (!pPS)
			return false;
	}

	PSODesc.GraphicsPipeline.InputLayout.LayoutElements = MeshVertexLayoutElems;
	PSODesc.GraphicsPipeline.InputLayout.NumElements = _countof(MeshVertexLayoutElems);
	PSODesc.GraphicsPipeline.pVS = pVS;
	PSODesc.GraphicsPipeline.pPS = pPS;

	PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	ShaderResourceVariableDesc Vars[16];
	Vars[0] = { SHADER_TYPE_PIXEL, "g_ShadowMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE };

	for(uint32 it=0; it<pData->TextureCount; ++it) {
		Vars[it+1] = { SHADER_TYPE_PIXEL, tv[it].name, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE };
	}
	PSODesc.ResourceLayout.Variables = Vars;
	PSODesc.ResourceLayout.NumVariables = pData->TextureCount + 1;

	// define static comparison sampler for shadow map
	SamplerDesc ComparisonSampler;
	ComparisonSampler.ComparisonFunc = COMPARISON_FUNC_LESS;
	ComparisonSampler.MinFilter = FILTER_TYPE_COMPARISON_LINEAR;
	ComparisonSampler.MagFilter = FILTER_TYPE_COMPARISON_LINEAR;
	ComparisonSampler.MipFilter = FILTER_TYPE_COMPARISON_LINEAR;


	StaticSamplerDesc StaticSamplers[16];
	StaticSamplers[0] = { SHADER_TYPE_PIXEL, "g_ShadowMap", ComparisonSampler };
	for(uint32 it=0; it<pData->TextureCount; ++it) {
		StaticSamplers[it+1] = { SHADER_TYPE_PIXEL, tv[it].name, ComparisonSampler };
	}
	PSODesc.ResourceLayout.StaticSamplers = StaticSamplers;
	PSODesc.ResourceLayout.NumStaticSamplers = pData->TextureCount + 1;

	pDevice->CreatePipelineState(Args, &pMaterialPipelineState);
	if (!pMaterialPipelineState)
		return false;

	pMaterialPipelineState->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(pGraphics->GetRenderConstants());
	pMaterialPipelineState->CreateShaderResourceBinding(&pMaterialResourceBinding, true);

	if (let pShadowMapVar = pMaterialResourceBinding->GetVariableByName(SHADER_TYPE_PIXEL, "g_ShadowMap"))
		pShadowMapVar->Set(pGraphics->GetShadowMapSRV());

	for(uint32 it=0; it<pData->TextureCount; ++it)
		if (let pVar = pMaterialResourceBinding->GetVariableByName(SHADER_TYPE_PIXEL, tv[it].name))
			pVar->Set(tv[it].pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
			

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
