// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Skeleton.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"

// compile-time graphics config
#ifndef TEX_FORMAT_SHADOW_MAP
#	define TEX_FORMAT_SHADOW_MAP TEX_FORMAT_D16_UNORM
#endif
#ifndef SHADOW_MAP_DEBUG
#	define SHADOW_MAP_DEBUG 0
#endif
#ifndef DEBUG_LINE_CAPACITY
#	define DEBUG_LINE_CAPACITY (1024 * 1024)
#endif

// TODO: Implement IAssetListener to detect releases

struct CameraPOV {
	RPose pose;
	float fovy;
	float zNear;
	float zFar;
};

struct MeshComponentHandle {
	Graphics* gfx;
	ObjectID id;
};

struct RenderMeshData {
	// TODO: multi-material meshes?
	Mesh* pMesh;
	Material* pMaterial;
	bool castsShadow;
};

struct RenderConstants {
	mat4 ModelViewProjectionTransform;
	mat4 ModelViewTransform;
	mat4 ModelTransform;
	mat4 NormalTransform;
	mat4 SceneToShadowMapUVDepth;
	vec4 LightDirection;
};

class World;

class Graphics : IAssetListener, ISceneListener, ISkelRegistryListener {
public:

	Graphics(Display *aDisplay, World* aWorld);
	~Graphics();

	Display* GetDisplay() { return pDisplay; }
	World* GetWorld() const { return pWorld; }
	IShaderSourceInputStreamFactory* GetShaderSourceStream() { return pShaderSourceFactory; }
	IBuffer* GetRenderConstants() { return pRenderConstants; }
	ITextureView* GetShadowMapSRV() { return pShadowMapSRV; }
	const CameraPOV& GetPOV() const { return pov; }

	void SetEyePosition(vec3 position) { pov.pose.position = position; }
	void SetEyeRotation(quat rotation) { pov.pose.rotation = rotation; }
	void SetFOV(float fovy) { pov.fovy = fovy; }

	void SetLightDirection(vec3 direction) { lightDirection = glm::normalize(direction); }

	void AddRenderPasses(Material* pMaterial);

	bool AddMeshRenderer(ObjectID id, const RenderMeshData& Data);
	const RenderMeshData* GetMeshRenderer(ObjectID id) const { return meshRenderers.TryGetComponent<1>(id); }

	void DrawDebugLine(const vec4& color, const vec3& start, const vec3& end);

	void Draw();

private:

	void Database_WillReleaseAsset(AssetDatabase* caller, ObjectID id) override;
	void Scene_WillReleaseObject(Scene* caller, ObjectID id) override;
	void Skeleton_WillReleaseSkeleton(class SkelRegistry* Caller, ObjectID id) override;
	void Skeleton_WillReleaseSkelAsset(class SkelRegistry* Caller, ObjectID id) override;

	Display* pDisplay;
	World* pWorld;

	ObjectPool<RenderMeshData> meshRenderers;

	CameraPOV pov;
	vec3 lightDirection;

	RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;

	RefCntAutoPtr<ITextureView> m_pMSColorRTV;
	RefCntAutoPtr<ITextureView> m_pMSDepthDSV;


	RefCntAutoPtr<IBuffer>                pRenderConstants;

	RefCntAutoPtr<ITexture>               pShadowMap;
	RefCntAutoPtr<ITextureView>           pShadowMapSRV;
	RefCntAutoPtr<ITextureView>           pShadowMapDSV;
	RefCntAutoPtr<IPipelineState>         pShadowPipelineState;
	RefCntAutoPtr<IShaderResourceBinding> pShadowResourceBinding;

	RefCntAutoPtr<IPipelineState>         pShadowMapDebugPSO;
	RefCntAutoPtr<IShaderResourceBinding> pShadowMapDebugSRB;

	struct RenderItem {
		Mesh* pMesh;
		ObjectID id;
		uint16 submeshIdx;
		uint16 shadows;
	};

	struct RenderPass {
		Material* pMaterial;
		int materialPassIdx;
		int itemCount;
	};


	eastl::vector<RenderPass> passes;
	eastl::vector<RenderItem> items;
	eastl::vector<mat4> matrices;

#if TRINKET_TEST

	struct WireframeVertex {
		vec3 position;
		vec4 color;
	};

	RefCntAutoPtr<IPipelineState> pDebugWireframePSO;
	RefCntAutoPtr<IShaderResourceBinding> pDebugWireframeSRB;
	RefCntAutoPtr<IBuffer>        pDebugWireframeBuf;

	// TODO: double buffer (for multithreading?)
	uint32 lineCount = 0;
	WireframeVertex lineBuf[2 * DEBUG_LINE_CAPACITY];

#endif
};

