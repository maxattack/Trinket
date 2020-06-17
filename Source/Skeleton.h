// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Math.h"
#include "Object.h"
#include "Assets.h"
#include "Scene.h"
#include "Listener.h"

// Only need 16-bits to index a skeleton
typedef int16 skel_idx_t;

// skeleton pure helper functions
namespace Skel {

	void CalcSceneSpacePose(HPose* outScenePoses, const HPose& skelToScene, const HPose* inLocalPoses, const skel_idx_t* inParents, int n);
}

class SkelAsset : public ObjectComponent {
public:
	SkelAsset(ObjectID aID) noexcept : ObjectComponent(aID) {}
	~SkelAsset();

	bool IsAllocated() const { return pNames != nullptr; }
	bool TryAlloc(int count);
	bool TryDealloc();

	int NumBones() const { return nbones; }
	bool InRange(skel_idx_t idx) const { return idx >= 0 && idx < nbones; }
	Name GetName(skel_idx_t idx) const { CHECK_ASSERT(InRange(idx)); return pNames[idx]; }
	skel_idx_t FindBone(Name name) const;
	skel_idx_t GetParent(skel_idx_t idx) const { CHECK_ASSERT(InRange(idx)); return pParents[idx]; }
	const HPose& GetLocalRestPose(skel_idx_t idx) const { CHECK_ASSERT(InRange(idx)); return pLocalPoses[idx]; }

	void SetName(skel_idx_t idx, Name name);
	void SetParent(skel_idx_t idx, skel_idx_t parent);
	void SetLocalRestPose(skel_idx_t idx, const HPose& pose);

private:
	int32 nbones = 0;
	HPose* pLocalPoses = nullptr;
	Name* pNames = nullptr;
	skel_idx_t* pParents = nullptr;

	friend class Skeleton;
};

class Skeleton : public ObjectComponent {
public:

	Skeleton(ObjectID aID, SkelAsset* asset) noexcept;
	~Skeleton();

	SkelAsset* GetSkelAsset() const { return pAsset; }

	void ResetRestPoses();
	const HPose& GetObjectPose(skel_idx_t idx) const { CHECK_ASSERT(pAsset->InRange(idx)); return pObjectPoses[idx]; }

private:

	SkelAsset* pAsset;
	HPose* pObjectPoses = nullptr;

};

// skeleton registry is a subsystem that's shared between the 
// graphics system (for rendering skinned meshes), and the
// animation system (for posing character rigs).


class ISkelRegistryListener {
public:

	virtual void Skeleton_WillReleaseSkeleton(class SkelRegistry* Caller, ObjectID id) = 0;
	virtual void Skeleton_WillReleaseSkelAsset(class SkelRegistry* Caller, ObjectID id) = 0;
};

class SkelRegistry : IAssetListener, ISceneListener {
public:

	SkelRegistry(AssetDatabase* aAssets, Scene* aScene);
	~SkelRegistry();

	AssetDatabase* GetAssets() const { return pAssets; }
	Scene* GetScene() const { return pScene; }

	void AddListener(ISkelRegistryListener* listener) { listeners.TryAdd(listener); }
	void RemoveListener(ISkelRegistryListener* listener) { listeners.TryRemove_Swap(listener); }

	SkelAsset* CreateSkeletonAsset(Name name);
	SkelAsset* GetSkeletonAsset(ObjectID id);

	Skeleton* AttachSkeletonTo(ObjectID id, SkelAsset* skel);
	Skeleton* GetSkeletonFor(ObjectID id);

	// Sockets 'socket' a scene object to a skeletal joint, e.g. a prop-bone
	bool TryAttachSocketTo(ObjectID id, Skeleton* skel, int16 jointIdx, const HPose& relativePose);
	bool TryReleaseScoket(ObjectID id);

	void Update();

private:

	ListenerList<ISkelRegistryListener> listeners;
	AssetDatabase* pAssets;
	Scene* pScene;

	struct Socket {
		Skeleton* pSkeleton;
		HPose relativePose;
		skel_idx_t idx;
	};

	ObjectPool<StrongRef<SkelAsset>> assets;
	ObjectPool<StrongRef<Skeleton>> instances;
	ObjectPool<Socket> sockets;

	void Database_WillReleaseAsset(AssetDatabase* caller, ObjectID id) override;
	void Scene_WillReleaseObject(Scene* caller, ObjectID id) override;



};