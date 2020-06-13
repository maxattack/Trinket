#include "Skeleton.h"

SkeletonRegistry::SkeletonRegistry(AssetDatabase* aAssets, World* aWorld) 
	: pAssets(aAssets)
	, pWorld(aWorld)
{
	pAssets->AddListener(this);
	pWorld->AddListener(this);
}

SkeletonRegistry::~SkeletonRegistry() {
	pAssets->RemoveListener(this);
	pWorld->RemoveListener(this);
}

SkelAsset* SkeletonRegistry::CreateSkeletonAsset(Name name) {
	let id = pAssets->CreateObject(name);
	let result = NewObjectComponent<SkelAsset>(id);
	assets.TryAppendObject(id, result);
	return result;
}

SkelAsset* SkeletonRegistry::GetSkeletonAsset(ObjectID id) {
	return DerefPP(assets.TryGetComponent<1>(id));
}

Skeleton* SkeletonRegistry::AttachSkeletonTo(ObjectID id, SkelAsset* skel) {
	let earlyOut = instances.Contains(id) || !pWorld->IsValid(id);
	if (earlyOut)
		return nullptr;

	let result = NewObjectComponent<Skeleton>(id, skel);
	instances.TryAppendObject(id, result);
	return result;
}

Skeleton* SkeletonRegistry::GetSkeletonFor(ObjectID id) {
	return DerefPP(instances.TryGetComponent<1>(id));
}

bool SkeletonRegistry::TryAttachSocketTo(ObjectID id, Skeleton* skel, int16 jointIdx, const HPose& pose) {
	let statusOK = !sockets.Contains(id) && pWorld->IsValid(id);
	return statusOK && sockets.TryAppendObject(id, Socket { skel->ID(), jointIdx, pose });
}

bool SkeletonRegistry::TryReleaseScoket(ObjectID id) {
	return sockets.TryReleaseObject_Swap(id);
}

void SkeletonRegistry::Update() {
	// TODO: write sockets to hierarchy
}

void SkeletonRegistry::Database_WillReleaseAsset(AssetDatabase* caller, ObjectID id) {
	if (!assets.Contains(id))
		return;

	for(auto it : listeners)
		it->Skeleton_WillReleaseSkelAsset(this, id);
	assets.TryReleaseObject_Swap(id);
}

void SkeletonRegistry::World_WillReleaseObject(World* caller, ObjectID id) {
	sockets.TryReleaseObject_Swap(id);
	if (!instances.Contains(id))
		return;

	for(auto it : listeners)
		it->Skeleton_WillReleaseSkeleton(this, id);
	instances.TryReleaseObject_Swap(id);
	
}
