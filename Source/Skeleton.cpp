// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Skeleton.h"

SkelRegistry::SkelRegistry(AssetDatabase* aAssets, World* aWorld) 
	: pAssets(aAssets)
	, pWorld(aWorld)
{
	pAssets->AddListener(this);
	pWorld->AddListener(this);
}

SkelRegistry::~SkelRegistry() {
	pAssets->RemoveListener(this);
	pWorld->RemoveListener(this);
}

SkelAsset* SkelRegistry::CreateSkeletonAsset(Name name) {
	let id = pAssets->CreateObject(name);
	let result = NewObjectComponent<SkelAsset>(id);
	assets.TryAppendObject(id, result);
	return result;
}

SkelAsset* SkelRegistry::GetSkeletonAsset(ObjectID id) {
	return DerefPP(assets.TryGetComponent<1>(id));
}

Skeleton* SkelRegistry::AttachSkeletonTo(ObjectID id, SkelAsset* skel) {
	let earlyOut = instances.Contains(id) || !pWorld->IsValid(id);
	if (earlyOut)
		return nullptr;

	let result = NewObjectComponent<Skeleton>(id, skel);
	instances.TryAppendObject(id, result);
	return result;
}

Skeleton* SkelRegistry::GetSkeletonFor(ObjectID id) {
	return DerefPP(instances.TryGetComponent<1>(id));
}

bool SkelRegistry::TryAttachSocketTo(ObjectID id, Skeleton* skel, int16 jointIdx, const HPose& pose) {
	let statusOK = !sockets.Contains(id) && pWorld->IsValid(id);
	if (!statusOK && sockets.TryAppendObject(id, Socket { skel->ID(), jointIdx, pose }))
		return false;

	let pHierarchy = GetWorld()->GetSublevelHierarchyFor(id);
	pHierarchy->SetMask(id, PoseMask(true, true, true));
	return true;
}

bool SkelRegistry::TryReleaseScoket(ObjectID id) {
	return sockets.TryReleaseObject_Swap(id);
}

void SkelRegistry::Update() {
	// TODO: write sockets to hierarchy
}

void SkelRegistry::Database_WillReleaseAsset(AssetDatabase* caller, ObjectID id) {
	if (!assets.Contains(id))
		return;

	for(auto it : listeners)
		it->Skeleton_WillReleaseSkelAsset(this, id);
	assets.TryReleaseObject_Swap(id);
}

void SkelRegistry::World_WillReleaseObject(World* caller, ObjectID id) {
	sockets.TryReleaseObject_Swap(id);
	if (!instances.Contains(id))
		return;

	for(auto it : listeners)
		it->Skeleton_WillReleaseSkeleton(this, id);
	instances.TryReleaseObject_Swap(id);
	
}
