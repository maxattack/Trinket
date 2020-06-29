// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Skeleton.h"
#include "World.h"

//------------------------------------------------------------------------------------------
// Helper Functions

void Skel::CalcSceneSpacePose(HPose* outScenePoses, const HPose& skelToScene, const HPose* inLocalPoses, const skel_idx_t* inParents, int n) {
	if (n == 0)
		return;

	outScenePoses[0] = skelToScene * inLocalPoses[0];
	for(int it=1; it<n; ++it) {
		let parent = inParents[it];
		CHECK_ASSERT(parent >= 0 && parent< n);
		outScenePoses[it] = outScenePoses[parent] * inLocalPoses[it];
	}
}

//------------------------------------------------------------------------------------------
// Skeleton Asset

SkelAsset::~SkelAsset() {
	free(pLocalPoses);
}

bool SkelAsset::TryAlloc(int n) {
	if (IsAllocated())
		return false;

	size_t nbytes = n * (sizeof(Name) + sizeof(skel_idx_t) + sizeof(HPose));
	auto buf = malloc(nbytes);
	if (!buf)
		return false;

	pLocalPoses = (HPose*) buf;
	pNames = (Name*) &pLocalPoses[n];
	pParents = (skel_idx_t*) &pNames[n];
	nbones = n;

	pParents[0] = INVALID_INDEX;

	return false;
}

bool SkelAsset::TryDealloc() {
	if (!IsAllocated())
		return false;

	free(pNames);
	pNames = nullptr;
	pParents = nullptr;
	pLocalPoses = nullptr;
	nbones = 0;
	return true;
}

skel_idx_t SkelAsset::FindBone(Name name) const {
	for(skel_idx_t it=0; it<nbones; ++it)
		if (pNames[it] == name)
			return it;
	return INVALID_INDEX;
}

void SkelAsset::SetName(skel_idx_t idx, Name name) {
	CHECK_ASSERT(InRange(idx));
	pNames[idx] = name;
}

void SkelAsset::SetParent(skel_idx_t idx, skel_idx_t parent) {
	CHECK_ASSERT(idx > 0 && InRange(idx));
	CHECK_ASSERT(parent >= 0 && parent < idx);
	pParents[idx] = parent;
}

void SkelAsset::SetLocalRestPose(skel_idx_t idx, const HPose& pose) {
	CHECK_ASSERT(InRange(idx));
	pLocalPoses[idx] = pose;
}

//------------------------------------------------------------------------------------------
// Skeleton Scene Component

Skeleton::Skeleton(ObjectID aID, SkelAsset* asset) noexcept
	: ObjectComponent(aID)
	, pAsset(asset) 
{
	pObjectPoses = (HPose*) malloc(sizeof(HPose) * pAsset->NumBones());
	ResetRestPoses();
}

Skeleton::~Skeleton() {
	free(pObjectPoses);
}

void Skeleton::ResetRestPoses() {
	Skel::CalcSceneSpacePose(pObjectPoses, HPOSE_IDENTITY, pAsset->pLocalPoses, pAsset->pParents, pAsset->nbones);
}

//------------------------------------------------------------------------------------------
// Skeleton Registry

SkelRegistry::SkelRegistry(World* aWorld) : pWorld(aWorld)
{
	pWorld->GetAssetDatabase()->AddListener(this);
	pWorld->GetScene()->AddListener(this);
}

SkelRegistry::~SkelRegistry() {
	pWorld->GetAssetDatabase()->RemoveListener(this);
	pWorld->GetScene()->RemoveListener(this);
}

SkelAsset* SkelRegistry::CreateSkeletonAsset(Name name) {
	let id = pWorld->GetAssetDatabase()->CreateObject(name);
	let result = NewObjectComponent<SkelAsset>(id);
	assets.TryAppendObject(id, result);
	return result;
}

SkelAsset* SkelRegistry::GetSkeletonAsset(ObjectID id) {
	return DerefPP(assets.TryGetComponent<1>(id));
}

Skeleton* SkelRegistry::AttachSkeletonTo(ObjectID id, SkelAsset* skel) {
	let earlyOut = instances.Contains(id) || !pWorld->GetScene()->IsValid(id);
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
	let pScene = pWorld->GetScene();
	let statusOK = !sockets.Contains(id) && pScene->IsValid(id);
	if (!statusOK && sockets.TryAppendObject(id, Socket { skel, pose, jointIdx }))
		return false;

	let pHierarchy = pScene->GetSublevelHierarchyFor(id);
	pHierarchy->SetMask(id, PoseMask(true, true, true));
	return true;
}

bool SkelRegistry::TryReleaseScoket(ObjectID id) {
	return sockets.TryReleaseObject_Swap(id);
}

void SkelRegistry::Update() {
	
	// update sockets
	let pScene = pWorld->GetScene();
	let pSocketIDs = sockets.GetComponentData<0>();
	let pSockets = sockets.GetComponentData<1>();
	let nSockets = sockets.Count();
	for(int it=0; it<nSockets; ++it) {
		let id = pSocketIDs[it];
		let skelID = pSockets[it].pSkeleton->ID();
		let pSocketHierarchy = pScene->GetSublevelHierarchyFor(id);
		let pSkelHierarchy = pScene->GetSublevelHierarchyFor(skelID);
		let pose = (*pSkelHierarchy->GetScenePose(skelID)) * pSockets[it].pSkeleton->GetObjectPose(pSockets[it].idx);
		pSocketHierarchy->SetScenePose(id, pose);
	}
}

void SkelRegistry::Database_WillReleaseAsset(AssetDatabase* caller, ObjectID id) {
	if (!assets.Contains(id))
		return;

	for(auto it : listeners)
		it->Skeleton_WillReleaseSkelAsset(this, id);
	assets.TryReleaseObject_Swap(id);
}

void SkelRegistry::Scene_WillReleaseObject(Scene* caller, ObjectID id) {
	sockets.TryReleaseObject_Swap(id);
	if (!instances.Contains(id))
		return;

	for(auto it : listeners)
		it->Skeleton_WillReleaseSkeleton(this, id);
	instances.TryReleaseObject_Swap(id);
	
}
