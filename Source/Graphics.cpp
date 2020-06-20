// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Graphics.h"
#include "DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp"
#include "DiligentCore/Graphics/GraphicsTools/interface/GraphicsUtilities.h"

#include <glm/gtx/color_space.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Math.h"


Graphics::Graphics(Display* aDisplay, SkelRegistry* aSkel) 
	: pDisplay(aDisplay)
	, pSkel(aSkel)
	, pAssets(aSkel->GetAssets())
	, pScene(aSkel->GetScene())
	, pov{ RPose(ForceInit::Default), 60.f, 0.01f, 100000.f }
	, lightDirection(0, -1, 0)
{
	pAssets->AddListener(this);
	pScene->AddListener(this);

	let pDevice = GetDevice();
	let pSwapChain = GetSwapChain();
	let pContext = GetContext();
	let pEngineFactory = pDisplay->GetEngineFactory();

	// create the "shader source" (just use surface filesystem hook for now)
	pEngineFactory->CreateDefaultShaderSourceStreamFactory("Assets", &pShaderSourceFactory);

	// create shadow map texture
	{
		TextureDesc SMDesc;
		SMDesc.Name = "Shadow Map";
		SMDesc.Type = RESOURCE_DIM_TEX_2D;
		SMDesc.Width = 4096;
		SMDesc.Height = 4096;
		SMDesc.Format = TEX_FORMAT_SHADOW_MAP;
		SMDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;
		pDevice->CreateTexture(SMDesc, nullptr, &pShadowMap);
		pShadowMapSRV = pShadowMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
		pShadowMapDSV = pShadowMap->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
	}


	// create render constants
	{
		BufferDesc BD;
		BD.Name = "CB_Surface";
		BD.uiSizeInBytes = sizeof(RenderConstants);
		BD.Usage = USAGE_DYNAMIC;
		BD.BindFlags = BIND_UNIFORM_BUFFER;
		BD.CPUAccessFlags = CPU_ACCESS_WRITE;
		pDevice->CreateBuffer(BD, nullptr, &pRenderConstants);
	}

	// create shadow map pipeline state
	{
		PipelineStateCreateInfo PCI;
		PipelineStateDesc& PSODesc = PCI.PSODesc;
		PSODesc.Name = "PS_Shadow";
		PSODesc.IsComputePipeline = false;
		PSODesc.GraphicsPipeline.NumRenderTargets = 0;
		PSODesc.GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_UNKNOWN;
		PSODesc.GraphicsPipeline.DSVFormat = TEX_FORMAT_SHADOW_MAP;
		PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		PSODesc.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
		PSODesc.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
		RefCntAutoPtr<IShader> pVS;
		{
			ShaderCreateInfo SCI;
			SCI.pShaderSourceStreamFactory = pShaderSourceFactory;
			SCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
			SCI.UseCombinedTextureSamplers = true;
			SCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
			SCI.EntryPoint = "main";
			SCI.Desc.Name = "VS_shadow";
			SCI.FilePath = "shadow.vsh";
			pDevice->CreateShader(SCI, &pVS);
		}
		PSODesc.GraphicsPipeline.pVS = pVS;
		PSODesc.GraphicsPipeline.pPS = nullptr; // depth/vertex-shader only
		PSODesc.GraphicsPipeline.InputLayout.LayoutElements = MeshVertexLayoutElems;
		PSODesc.GraphicsPipeline.InputLayout.NumElements = _countof(MeshVertexLayoutElems);
		PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
		PSODesc.GraphicsPipeline.RasterizerDesc.DepthClipEnable = false;
		PSODesc.GraphicsPipeline.RasterizerDesc.SlopeScaledDepthBias = 5.f;
		pDevice->CreatePipelineState(PCI, &pShadowPipelineState);
		pShadowPipelineState->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(pRenderConstants);
		pShadowPipelineState->CreateShaderResourceBinding(&pShadowResourceBinding, true);
	}

	#if SHADOW_MAP_DEBUG
	// shadowmap viz pso
	{
		PipelineStateCreateInfo PSOCreateInfo;
		PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
		PSODesc.Name = "PSO_ShadowMapDebug";
		PSODesc.IsComputePipeline = false;
		PSODesc.GraphicsPipeline.NumRenderTargets = 1;
		PSODesc.GraphicsPipeline.RTVFormats[0] = pSwapChain->GetDesc().ColorBufferFormat;
		PSODesc.GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;
		PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		PSODesc.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
		PSODesc.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
		ShaderCreateInfo ShaderCI;
		ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
		ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
		ShaderCI.UseCombinedTextureSamplers = true;
		RefCntAutoPtr<IShader> pShadowMapDebugVS;
		{
			ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
			ShaderCI.EntryPoint = "main";
			ShaderCI.Desc.Name = "Shadow Map Vis VS";
			ShaderCI.FilePath = "shadow_debug.vsh";
			pDevice->CreateShader(ShaderCI, &pShadowMapDebugVS);
		}
		RefCntAutoPtr<IShader> pShadowMapDebugPS;
		{
			ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
			ShaderCI.EntryPoint = "main";
			ShaderCI.Desc.Name = "Shadow Map Vis PS";
			ShaderCI.FilePath = "shadow_debug.psh";
			pDevice->CreateShader(ShaderCI, &pShadowMapDebugPS);
		}
		PSODesc.GraphicsPipeline.pVS = pShadowMapDebugVS;
		PSODesc.GraphicsPipeline.pPS = pShadowMapDebugPS;
		PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
		SamplerDesc SamLinearClampDesc{
			FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
			TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
		};
		StaticSamplerDesc StaticSamplers[]{
			{SHADER_TYPE_PIXEL, "g_ShadowMap", SamLinearClampDesc}
		};
		PSODesc.ResourceLayout.StaticSamplers = StaticSamplers;
		PSODesc.ResourceLayout.NumStaticSamplers = _countof(StaticSamplers);
		pDevice->CreatePipelineState(PSOCreateInfo, &pShadowMapDebugPSO);
		pShadowMapDebugPSO->CreateShaderResourceBinding(&pShadowMapDebugSRB, true);
		pShadowMapDebugSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_ShadowMap")->Set(pShadowMapSRV);
	}
	#endif

	#if TRINKET_TEST
	// debug wireframe
	{
		// Wireframe PSO
		PipelineStateCreateInfo Args;
		PipelineStateDesc& PSODesc = Args.PSODesc;
		PSODesc.Name = "PSO_DebugWireframe";
		PSODesc.IsComputePipeline = false;
		PSODesc.GraphicsPipeline.NumRenderTargets = 1;
		PSODesc.GraphicsPipeline.RTVFormats[0] = pSwapChain->GetDesc().ColorBufferFormat;
		PSODesc.GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;
		PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_LINE_LIST;
		PSODesc.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
		PSODesc.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;

		ShaderCreateInfo SCI;
		SCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
		SCI.UseCombinedTextureSamplers = true; // For GL Compat
		SCI.pShaderSourceStreamFactory = GetShaderSourceStream();
		RefCntAutoPtr<IShader> pVS;
		{
			SCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
			SCI.EntryPoint = "main";
			SCI.Desc.Name = "VS_DebugWireframe";
			SCI.FilePath = "wireframe.vsh";
			pDevice->CreateShader(SCI, &pVS);
			CHECK_ASSERT(pVS);
		}
		RefCntAutoPtr<IShader> pPS;
		{
			SCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
			SCI.EntryPoint = "main";
			SCI.Desc.Name = "PS_DebugWireframe";
			SCI.FilePath = "wireframe.psh";
			pDevice->CreateShader(SCI, &pPS);
			CHECK_ASSERT(pPS);
		}

		const LayoutElement WireframeLayoutElems[2]{
			LayoutElement{ 0, 0, 3, VT_FLOAT32, false },
			LayoutElement{ 1, 0, 4, VT_FLOAT32, false },
		};

		PSODesc.GraphicsPipeline.InputLayout.LayoutElements = WireframeLayoutElems;
		PSODesc.GraphicsPipeline.InputLayout.NumElements = _countof(WireframeLayoutElems);
		PSODesc.GraphicsPipeline.pVS = pVS;
		PSODesc.GraphicsPipeline.pPS = pPS;

		PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
		PSODesc.ResourceLayout.NumVariables = 0;
		PSODesc.ResourceLayout.NumStaticSamplers = 0;

		pDevice->CreatePipelineState(Args, &pDebugWireframePSO);
		CHECK_ASSERT(pDebugWireframePSO);

		pDebugWireframePSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(GetRenderConstants());
		pDebugWireframePSO->CreateShaderResourceBinding(&pDebugWireframeSRB, true);

		// Wireframe Vertex Buffer
		BufferDesc VBD;
		VBD.Name = "VB_DebugWireframe";
		VBD.Usage = USAGE_DEFAULT; //USAGE_DYNAMIC;
		VBD.BindFlags = BIND_VERTEX_BUFFER;
		VBD.uiSizeInBytes = sizeof(lineBuf);
		pDevice->CreateBuffer(VBD, nullptr, &pDebugWireframeBuf);
		CHECK_ASSERT(pDebugWireframeBuf);
	}
	#endif
}

Graphics::~Graphics() {
	pAssets->RemoveListener(this);
	pScene->RemoveListener(this);
}

void Graphics::Database_WillReleaseAsset(AssetDatabase* caller, ObjectID id) {
	// TODO
}

void Graphics::Scene_WillReleaseObject(Scene* caller, ObjectID id) {
	// TODO
}

void Graphics::Skeleton_WillReleaseSkeleton(class SkelRegistry* Caller, ObjectID id) {
	// TODO
}

void Graphics::Skeleton_WillReleaseSkelAsset(class SkelRegistry* Caller, ObjectID id) {
	// TODO
}

Material* Graphics::LoadMaterial(ObjectID id, const MaterialAssetData* pData) {
	let idOkay = 
		pAssets->IsValid(id) && 
		!materials.Contains(id);
	if (!idOkay)
		return nullptr;

	let result = NewObjectComponent<Material>(id);
	if (!result->TryLoad(this, pData)) {
		FreeObjectComponent(result);
		return nullptr;
	}

	let bAdded = materials.TryAppendObject(id, result);
	CHECK_ASSERT(bAdded);

	for (int it = 0; it < result->NumPasses(); ++it)
		passes.push_back(RenderPass{ result, it, 0 });

	return result;
}

ITexture* Graphics::LoadTexture(ObjectID id, const TextureAssetData* pData) {
	let idOkay = 
		pAssets->IsValid(id) && 
		!textures.Contains(id);
	if (!idOkay)
		return nullptr;

	auto result = LoadTextureHandleFromAsset(pDisplay, pData);
	if (!result)
		return nullptr;

	let bAdded = textures.TryAppendObject(id, result);
	CHECK_ASSERT(bAdded);

	return result;
}

Mesh* Graphics::AddMesh(ObjectID id) {
	let idOkay =
		pAssets->IsValid(id) &&
		!meshes.Contains(id);
	if (!idOkay)
		return nullptr;

	let result = NewObjectComponent<Mesh>(id);
	meshes.TryAppendObject(id, result);
	return result;
}

bool Graphics::AddRenderMesh(ObjectID id, const RenderMeshData& data) {

	// check refs
	if (!pScene->IsValid(id))
		return false;

	if (data.pMesh == nullptr)
		return false;
	
	if (data.pMaterial == nullptr)
		return false;

	if (!meshRenderers.TryAppendObject(id, data))
		return false;

	// add render items, update render passes
	int itemIdx = 0;
	for (auto pit = passes.begin(); pit != passes.end(); ++pit)
	{
		if (pit->pMaterial == data.pMaterial)
		{
			for (uint16 submeshIdx = 0; submeshIdx < data.pMesh->GetSubmeshCount(); ++submeshIdx) 
				items.insert(items.begin() + itemIdx, RenderItem { data.pMesh, id, submeshIdx, data.castsShadow });
			pit->itemCount += data.pMesh->GetSubmeshCount();
		}
		itemIdx += pit->itemCount;
	}

	return true;
}

void Graphics::DrawDebugLine(const vec4& color, const vec3& start, const vec3& end) {
	#if TRINKET_TEST

	if (lineCount >= DEBUG_LINE_CAPACITY)
		return;

	let idx = lineCount << 1;
	lineBuf[idx].position = start;
	lineBuf[idx].color = color;
	lineBuf[idx+1].position = end;
	lineBuf[idx+1].color = color;
	++lineCount;

	#endif
}

void Graphics::Draw() {
	let pDevice = GetDevice();
	let pSwapChain = GetSwapChain();
	let pContext = GetContext();

	const auto& DevCaps = pDevice->GetDeviceCaps();
	const auto& NDCAttribs = DevCaps.GetNDCAttribs();
	const bool  IsGL = DevCaps.IsGLDevice();

	// TODO: VIEWPORT CULLING

	// get transforms
	if (matrices.size() != items.size())
		matrices.resize(items.size());
	for(uint32 it=0; it<items.size(); ++it)
	{
		let& item = items[it];
		let pHierarchy = pScene->GetSublevelHierarchyFor(item.id);
		let pPose = pHierarchy->GetScenePose(item.id);
		matrices[it] = pPose->ToMatrix();
	}

	// draw shadow map
	let lightz = lightDirection;
	const vec3 referenceVec = lightz.y * lightz.y < lightz.z * lightz.z ? vec3(0, 1, 0) : vec3(0, 0, 1);
	let lightx = glm::normalize(glm::cross(referenceVec, lightz));
	let lighty = glm::cross(lightz, lightx);

	// TODO: actually calculate view bounds
	float  r = 5.f;
	const vec3 viewCenter (0, 0.0f, 0);
	let viewMin = viewCenter - vec3(r, r, r);
	let viewMax = viewCenter + vec3(r, r, r);
	let viewExtent = viewMax - viewMin;

	// Apply bias to shift the extent to 
	//    [-1,1]x[-1,1]x[0,1] for DX or to 
	//    [-1,1]x[-1,1]x[-1,1] for GL
	// Find bias such that sceneMin -> 
	//    (-1,-1,0) for DX or 
	//    (-1,-1,-1) for GL
	const vec3 lightSpaceScale (
		2.f / viewExtent.x,
		2.f / viewExtent.y,
		(IsGL ? 2.f : 1.f) / viewExtent.z
	);
	const vec3 lightSpaceScaledBias (
		-viewMin.x * lightSpaceScale.x - 1.f,
		-viewMin.y * lightSpaceScale.y - 1.f,
		-viewMin.z * lightSpaceScale.z + (IsGL ? -1.f : 0.f)
	);

	let worldToLightProjSpace = 
		glm::translate(lightSpaceScaledBias) *              // offset to scene center
		glm::scale(lightSpaceScale) *                       // scale to scene extents
		mat4(glm::transpose(mat3(lightx, lighty, lightz))); // view matrix of directional light

	let worldToShadowMapUVDepth = 
		glm::translate(vec3(0.5f, 0.5f, NDCAttribs.GetZtoDepthBias())) *         // to UV bias
		glm::scale(vec3(0.5f, NDCAttribs.YtoVScale, NDCAttribs.ZtoDepthScale)) * // to UV scale
		worldToLightProjSpace;

	{
		pContext->SetRenderTargets(0, nullptr, pShadowMapDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		pContext->ClearDepthStencil(pShadowMapDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		pContext->SetPipelineState(pShadowPipelineState);
		pContext->CommitShaderResources(pShadowResourceBinding, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		for(uint32 it=0; it<items.size(); ++it) {
			let& item = items[it];
			if (item.shadows) {
				{
					MapHelper<RenderConstants> CBConstants(pContext, pRenderConstants, MAP_WRITE, MAP_FLAG_DISCARD);
					CBConstants->ModelViewProjectionTransform = worldToLightProjSpace * matrices[it];
				}
				item.pMesh->GetSubmesh(item.submeshIdx).DoDraw(this);
			}
		}
	}

	// draw material passes
	let view = pov.pose.Inverse().ToMatrix();
	let aspect = pDisplay->GetAspect();
	let viewProjection = glm::perspective(glm::radians(pov.fovy), aspect, pov.zNear, pov.zFar) * view;
	const float ClearColor[] = { 0.50f, 0.05f, 0.55f, 1.0f };
	auto* pRTV = pSwapChain->GetCurrentBackBufferRTV();
	auto* pDSV = pSwapChain->GetDepthBufferDSV();
	pContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	pContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	pContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	int itemIdx=0;
	for(auto& pass : passes) {
		let bSkip = pass.itemCount == 0 || !pass.pMaterial->GetPass(pass.materialPassIdx).Bind(this);
		if (bSkip)
			continue;
		for(int it=0; it<pass.itemCount; ++it) {
			let& item = items[itemIdx + it];
			let& pose = matrices[itemIdx + it];
			let normalXf = glm::inverseTranspose(mat3(pose));
			let mvp = viewProjection * pose;
			let mv = view * pose;
			{
					MapHelper<RenderConstants> CBConstants(pContext, pRenderConstants, MAP_WRITE, MAP_FLAG_DISCARD);
					CBConstants->ModelViewProjectionTransform = mvp;
					CBConstants->ModelViewTransform = mv;
					CBConstants->ModelTransform = pose;
					CBConstants->NormalTransform = normalXf;
					CBConstants->SceneToShadowMapUVDepth = worldToShadowMapUVDepth;
					CBConstants->LightDirection = vec4(lightz, 0);
			}
			item.pMesh->GetSubmesh(item.submeshIdx).DoDraw(this);
		}
		itemIdx += pass.itemCount;
	}

	#if TRINKET_TEST
	if (lineCount > 0) {
	
		pContext->UpdateBuffer(pDebugWireframeBuf, 0, sizeof(WireframeVertex)* (lineCount + lineCount), lineBuf, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		pContext->SetPipelineState(pDebugWireframePSO);
		pContext->CommitShaderResources(pDebugWireframeSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		uint32 offset = 0;
		IBuffer* pBuffers[]{ pDebugWireframeBuf };
		pContext->SetVertexBuffers(0, 1, pBuffers, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
		{
			MapHelper<RenderConstants> CBConstants(pContext, pRenderConstants, MAP_WRITE, MAP_FLAG_DISCARD);
			CBConstants->ModelViewProjectionTransform = viewProjection;
			CBConstants->ModelViewTransform = view;
			CBConstants->ModelTransform = glm::identity<mat4>();
			CBConstants->NormalTransform = glm::identity<mat3>();
			//CBConstants->SceneToShadowMapUVDepth = worldToShadowMapUVDepth;
			//CBConstants->LightDirection = vec4(lightz, 0);
		}

		DrawAttribs draw;
		draw.NumVertices = lineCount << 1;
		pContext->Draw(draw);
		lineCount = 0;
	}

	#endif

	#if SHADOW_MAP_DEBUG
	// draw shadow debug
	{
		pContext->SetPipelineState(pShadowMapDebugPSO);
		pContext->CommitShaderResources(pShadowMapDebugSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		DrawAttribs DrawAttrs(4, DRAW_FLAG_VERIFY_ALL);
		pContext->Draw(DrawAttrs);
	}
	#endif

}
