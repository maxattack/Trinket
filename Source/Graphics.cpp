// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Graphics.h"
#include "DiligentCore/Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"
#include "DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp"
#include "DiligentCore/Graphics/GraphicsTools/interface/GraphicsUtilities.h"

#include <glm/gtx/color_space.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Math.h"

#define TEX_FORMAT_SHADOW_MAP TEX_FORMAT_D16_UNORM;
#define SHADOW_MAP_DEBUG 0


void GraphicsDebugMessageCallback(
	enum DEBUG_MESSAGE_SEVERITY Severity,
	const Char* Message,
	const Char* Function,
	const Char* File,
	int Line) 
{
	using namespace std;
	cout << "[GRAPHICS] " << Message << endl;
}


Graphics::Graphics(SkelRegistry* aSkel, SDL_Window* aWindow) 
	: pSkel(aSkel)
	, pAssets(aSkel->GetAssets())
	, pWorld(aSkel->GetWorld())
	, pWindow(aWindow)
	, pov{ RPose(ForceInit::Default), 60.f, 0.01f, 100000.f }
	, lightDirection(0, -1, 0)
{
	pAssets->AddListener(this);
	pWorld->AddListener(this);

	HWND hwnd = GetActiveWindow(); // TODO: way to get this from pWindow?

	// Rendering is routed through the Diligent Graphics API, which performs
	// - Backend Abstraction
	// - Debug Validation

	// create device
	EngineD3D12CreateInfo EngineCI;
	EngineCI.DebugMessageCallback = &GraphicsDebugMessageCallback;
#if _DEBUG
	EngineCI.EnableDebugLayer = true;
#endif
	auto GetEngineFactoryD3D12 = LoadGraphicsEngineD3D12();
	auto* pFactoryD3D12 = GetEngineFactoryD3D12();
	pFactoryD3D12->CreateDeviceAndContextsD3D12(EngineCI, &pDevice, &pContext);
	pEngineFactory = pFactoryD3D12;


	// create swap chain
	SwapChainDesc SCDesc;
	pFactoryD3D12->CreateSwapChainD3D12(pDevice, pContext, SCDesc, FullScreenModeDesc{}, Win32NativeWindow{ hwnd }, &pSwapChain);
	pSwapChain->SetMaximumFrameLatency(1);

}

Graphics::~Graphics() {
	pAssets->RemoveListener(this);
	pWorld->RemoveListener(this);
	meshAssets.Clear();
	if (pContext)
		pContext->Flush();
}

void Graphics::Database_WillReleaseAsset(AssetDatabase* caller, ObjectID id) {
	// TODO
}

void Graphics::World_WillReleaseObject(World* caller, ObjectID id) {
	// TODO
}

void Graphics::Skeleton_WillReleaseSkeleton(class SkelRegistry* Caller, ObjectID id) {
	// TODO
}

void Graphics::Skeleton_WillReleaseSkelAsset(class SkelRegistry* Caller, ObjectID id) {
	// TODO
}

void Graphics::InitSceneRenderer() {
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
		RefCntAutoPtr<IShader> pShadowMapDebugVS; {
			ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
			ShaderCI.EntryPoint = "main";
			ShaderCI.Desc.Name = "Shadow Map Vis VS";
			ShaderCI.FilePath = "shadow_debug.vsh";
			pDevice->CreateShader(ShaderCI, &pShadowMapDebugVS);
		}
		RefCntAutoPtr<IShader> pShadowMapDebugPS; {
			ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
			ShaderCI.EntryPoint = "main";
			ShaderCI.Desc.Name = "Shadow Map Vis PS";
			ShaderCI.FilePath = "shadow_debug.psh";
			pDevice->CreateShader(ShaderCI, &pShadowMapDebugPS);
		}
		PSODesc.GraphicsPipeline.pVS = pShadowMapDebugVS;
		PSODesc.GraphicsPipeline.pPS = pShadowMapDebugPS;
		PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
		SamplerDesc SamLinearClampDesc {
			FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
			TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
		};
		StaticSamplerDesc StaticSamplers[] {
			{SHADER_TYPE_PIXEL, "g_ShadowMap", SamLinearClampDesc}
		};
		PSODesc.ResourceLayout.StaticSamplers = StaticSamplers;
		PSODesc.ResourceLayout.NumStaticSamplers = _countof(StaticSamplers);
		pDevice->CreatePipelineState(PSOCreateInfo, &pShadowMapDebugPSO);
		pShadowMapDebugPSO->CreateShaderResourceBinding(&pShadowMapDebugSRB, true);
		pShadowMapDebugSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_ShadowMap")->Set(pShadowMapSRV);
	}
#endif
}

Material* Graphics::CreateMaterial(Name name, const MaterialArgs& Args) {

	// add asset object
	let id = pAssets->CreateObject(name);
	let result = NewObjectComponent<Material>(id);
	materialAssets.TryAppendObject(id, result);

	// defer this?
	result->TryLoad(this, Args);
	for(int it=0; it<Args.numTextures; ++it)
		pAssets->AddRef(Args.pTextureArgs[it].pTexture->ID());

	// add render passes
	for(int it=0; it<result->NumPasses(); ++it)
		passes.push_back(RenderPass { result, it, 0 });

	return result;
}

Texture* Graphics::CreateTexture(Name name) {
	let id = pAssets->CreateObject(name);
	let result = NewObjectComponent<Texture>(id);
	textureAssets.TryAppendObject(id, result);
	return result;
}

Mesh* Graphics::CreateMesh(Name name) {
	let id = pAssets->CreateObject(name);
	let result = NewObjectComponent<Mesh>(id);
	meshAssets.TryAppendObject(id, result);
	return result;
}

bool Graphics::TryAttachRenderMeshTo(ObjectID id, const RenderMeshData& data) {

	// check refs
	if (!pWorld->IsValid(id))
		return false;

	let pMesh = GetMesh(data.mesh);
	if (pMesh == nullptr)
		return false;
	
	let pMaterial = GetMaterial(data.material);
	if (pMaterial == nullptr)
		return false;

	if (!meshRenderers.TryAppendObject(id, data))
		return false;
	
	pAssets->AddRef(data.mesh);
	pAssets->AddRef(data.material);

	// add render items, update render passes
	int itemIdx = 0;
	for (auto pit = passes.begin(); pit != passes.end(); ++pit)
	{
		if (pit->pMaterial == pMaterial)
		{
			for (uint16 submeshIdx = 0; submeshIdx < pMesh->GetSubmeshCount(); ++submeshIdx) 
				items.insert(items.begin() + itemIdx, RenderItem { pMesh, id, submeshIdx, data.castsShadow });
			pit->itemCount += pMesh->GetSubmeshCount();
		}
		itemIdx += pit->itemCount;
	}

	return true;
}

const RenderMeshData* Graphics::GetRenderMeshFor(ObjectID id) const {
	return meshRenderers.TryGetComponent<C_RENDER_MESH>(id);
}

bool Graphics::TryReleaseRenderMeshFor(ObjectID id) {
	// TODO
	//return meshRenderers.TryReleaseObject_Swap(id);
	return false;
}

void Graphics::HandleEvent(const SDL_Event& ev) {
	let resize = ev.type == SDL_WINDOWEVENT && (
		ev.window.event == SDL_WINDOWEVENT_RESIZED ||
		ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED
	);
	if (resize) {
		int Width, Height;
		SDL_GetWindowSize(pWindow, &Width, &Height);
		pSwapChain->Resize(Width, Height);
	}
}

void Graphics::Draw() {
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
		let pHierarchy = pWorld->GetSublevelHierarchyFor(item.id);
		let pPose = pHierarchy->GetWorldPose(item.id);
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
	let viewProjection = glm::perspective(glm::radians(pov.fovy), GetAspect(), pov.zNear, pov.zFar) * view;
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
					CBConstants->WorldToShadowMapUVDepth = worldToShadowMapUVDepth;
					CBConstants->LightDirection = vec4(lightz, 0);
			}
			item.pMesh->GetSubmesh(item.submeshIdx).DoDraw(this);
		}
		itemIdx += pass.itemCount;
	}

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
