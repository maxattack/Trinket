// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Math.h"
#include "Object.h"
#include "Assets.h"
#include "World.h"
#include "Listener.h"

// skeleton registry is a subsystem that's shared between the 
// graphics system (for rendering skinned meshes), and the
// animation system (for posing character rigs).

typedef int16 joint_idx_t;

class SkelAsset : public ObjectComponent {
public:
	SkelAsset(ObjectID aID) : ObjectComponent(aID) {}

private:
	enum Components { C_NAME, C_PARENT, C_REST_POSE };
	eastl::tuple_vector<Name, joint_idx_t, HPose> bones;

};

class Skeleton : public ObjectComponent {
public:

	Skeleton(ObjectID aID, SkelAsset* asset) 
		: ObjectComponent(aID) 
		, pAsset(asset)
	{}

	SkelAsset* GetSkelAsset() const { return pAsset; }

private:

	SkelAsset* pAsset;

};

class ISkeletonRegistryListener {
public:

	virtual void Skeleton_WillReleaseSkeleton(class SkeletonRegistry* Caller, ObjectID id) = 0;
	virtual void Skeleton_WillReleaseSkelAsset(class SkeletonRegistry* Caller, ObjectID id) = 0;
};

class SkeletonRegistry : IAssetListener, IWorldListener {
public:

	SkeletonRegistry(AssetDatabase* aAssets, World* aWorld);
	~SkeletonRegistry();

	AssetDatabase* GetAssets() const { return pAssets; }
	World* GetWorld() const { return pWorld; }

	void AddListener(ISkeletonRegistryListener* listener) { listeners.TryAdd(listener); }
	void RemoveListener(ISkeletonRegistryListener* listener) { listeners.TryRemove_Swap(listener); }

	SkelAsset* CreateSkeletonAsset(Name name);
	SkelAsset* GetSkeletonAsset(ObjectID id);

	Skeleton* AttachSkeletonTo(ObjectID id, SkelAsset* skel);
	Skeleton* GetSkeletonFor(ObjectID id);

	bool TryAttachSocketTo(ObjectID id, Skeleton* skel, int16 jointIdx, const HPose& pose);
	bool TryReleaseScoket(ObjectID id);

	void Update();

private:

	ListenerList<ISkeletonRegistryListener> listeners;
	AssetDatabase* pAssets;
	World* pWorld;

	struct Socket {
		ObjectID skelID;
		joint_idx_t idx;
		HPose relativePose;
	};

	ObjectPool<SkelAsset*> assets;
	ObjectPool<Skeleton*> instances;
	ObjectPool<Socket> sockets;

	void Database_WillReleaseAsset(AssetDatabase* caller, ObjectID id) override;
	void World_WillReleaseObject(World* caller, ObjectID id) override;



};