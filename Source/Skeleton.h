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

class ISkelRegistryListener {
public:

	virtual void Skeleton_WillReleaseSkeleton(class SkelRegistry* Caller, ObjectID id) = 0;
	virtual void Skeleton_WillReleaseSkelAsset(class SkelRegistry* Caller, ObjectID id) = 0;
};

class SkelRegistry : IAssetListener, IWorldListener {
public:

	SkelRegistry(AssetDatabase* aAssets, World* aWorld);
	~SkelRegistry();

	AssetDatabase* GetAssets() const { return pAssets; }
	World* GetWorld() const { return pWorld; }

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