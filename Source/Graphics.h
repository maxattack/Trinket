// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Skeleton.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"

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
	ObjectID mesh;
	ObjectID material;
	bool castsShadow;
};

struct RenderConstants {
	mat4 ModelViewProjectionTransform;
	mat4 ModelViewTransform;
	mat4 ModelTransform;
	mat4 NormalTransform;
	mat4 WorldToShadowMapUVDepth;
	vec4 LightDirection;
};

class Graphics : IAssetListener, IWorldListener, ISkeletonRegistryListener {
public:

	Graphics(SkeletonRegistry* pSkel, SDL_Window* aWindow);
	~Graphics();

	void InitSceneRenderer();
	void HandleEvent(const SDL_Event& aEvent);
	void Draw();

	SkeletonRegistry* GetSkeletonRegistory() const { return pSkel; }
	AssetDatabase* GetAssets() const { return pAssets; }
	World* GetWorld() const { return pWorld; }

	SDL_Window* GetWindow() { return pWindow; }

	IRenderDevice* GetDevice() { return pDevice; }
	IDeviceContext* GetContext() { return pContext; }
	ISwapChain* GetSwapChain() { return pSwapChain; }

	IShaderSourceInputStreamFactory* GetShaderSourceStream() { return pShaderSourceFactory; }
	IBuffer* GetRenderConstants() { return pRenderConstants; }
	ITextureView* GetShadowMapSRV() { return pShadowMapSRV; }

	const CameraPOV& GetPOV() const { return pov; }

	void SetEyePosition(vec3 position) { pov.pose.position = position; }
	void SetEyeRotation(quat rotation) { pov.pose.rotation = rotation; }
	void SetFOV(float fovy) { pov.fovy = fovy; }

	void SetLightDirection(vec3 direction) { lightDirection = glm::normalize(direction); }

	float GetAspect() const { let& SCD = pSwapChain->GetDesc(); return float(SCD.Width) / float(SCD.Height); }

	Material* CreateMaterial(Name name, const MaterialArgs& Args);
	Material* GetMaterial(ObjectID assetID) { return DerefPP(materialAssets.TryGetComponent<C_MATERIAL_ASSET>(assetID)); }

	Texture* CreateTexture(Name name);
	Texture* GetTexture(ObjectID assetID) { return DerefPP(textureAssets.TryGetComponent<C_TEXTURE_ASSET>(assetID)); }

	Mesh* CreateMesh(Name name);
	Mesh* GetMesh(ObjectID meshID) { return DerefPP(meshAssets.TryGetComponent<C_MESH_ASSET>(meshID)); }

	bool TryAttachRenderMeshTo(ObjectID id, const RenderMeshData& Data);
	const RenderMeshData* GetRenderMeshFor(ObjectID id) const;
	bool TryReleaseRenderMeshFor(ObjectID id);

private:

	void Database_WillReleaseAsset(AssetDatabase* caller, ObjectID id) override;
	void World_WillReleaseObject(World* caller, ObjectID id) override;
	void Skeleton_WillReleaseSkeleton(class SkeletonRegistry* Caller, ObjectID id) override;
	void Skeleton_WillReleaseSkelAsset(class SkeletonRegistry* Caller, ObjectID id) override;


	SkeletonRegistry* pSkel;
	AssetDatabase* pAssets;
	World* pWorld;

	enum MeshAssetComponents { C_HANDLE, C_MESH_ASSET=1 };
	enum MaterialAssetComponents { C_MATERIAL_ASSET=1 };
	enum TextureAssetComponents { C_TEXTURE_ASSET=1 };
	enum RenderMeshComponents { C_RENDER_MESH=1 };

	ObjectPool<Mesh*> meshAssets;
	ObjectPool<Texture*> textureAssets;
	ObjectPool<Material*> materialAssets;
	ObjectPool<RenderMeshData> meshRenderers;

	SDL_Window* pWindow;

	CameraPOV pov;
	vec3 lightDirection;

	RefCntAutoPtr<IRenderDevice>  pDevice;
	RefCntAutoPtr<IDeviceContext> pContext;
	RefCntAutoPtr<IEngineFactory> pEngineFactory;
	RefCntAutoPtr<ISwapChain>     pSwapChain;

	RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;

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

};

