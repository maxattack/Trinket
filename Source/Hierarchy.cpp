#include "Hierarchy.h"

Hierarchy::Hierarchy(ObjectID worldID) noexcept 
	: ObjectComponent(worldID) 
{
	pool.ReserveCompact(1024);
}

bool Hierarchy::IsRoot(ObjectID id) const {
	let idx = IndexOf(id);
	return idx >= 0 && GetParentIndexByIndex(idx) < 0;
}

ObjectID Hierarchy::GetParent(ObjectID id) const {
	let idx = IndexOf(id);
	if (idx == INVALID_INDEX)
		return OBJECT_NIL;
	
	let parentIdx = GetParentIndexByIndex(idx);
	return parentIdx != INVALID_INDEX ? GetObjectByIndex(parentIdx) : OBJECT_NIL;
}

bool Hierarchy::HasChildren(ObjectID id) const {
	let idx = IndexOf(id);
	return idx != INVALID_INDEX && HasChildrenByIndex(idx);
}

bool Hierarchy::HasChildrenByIndex(int32 idx) const {
	DEBUG_ASSERT(idx >= 0);
	DEBUG_ASSERT(idx < Count());
	let lastIdx = Count() - 1;
	return idx != lastIdx && GetParentIndexByIndex(idx + 1) == idx;
}

int32 Hierarchy::GetDescendentRangeByIndex(int32 startIndex) const {
	DEBUG_ASSERT(startIndex >= 0);
	DEBUG_ASSERT(startIndex < Count());
	int32 result = startIndex + 1;
	let pParents = pool.GetComponentData<C_PARENT>();
	let count = Count();
	while (result < count && pParents[result] >= startIndex)
		++result;
	return result;
}

int32 Hierarchy::GetDescendentCount(ObjectID id) const {
	let idx = IndexOf(id);
	return idx == INVALID_INDEX ? 0 : (GetDescendentRangeByIndex(idx) - idx - 1);
}

int32 Hierarchy::GetDepth(ObjectID id) const {
	int32 idx = IndexOf(id);
	return idx < 0 ? 0 : GetDepthByIndex(idx);
}

int32 Hierarchy::GetDepthByIndex(int32 idx) const {
	DEBUG_ASSERT(idx >= 0);
	DEBUG_ASSERT(idx < Count());
	int32 result = 0;
	let pParents = pool.GetComponentData<C_PARENT>();
	while(pParents[idx] != INVALID_INDEX) {
		++result;
		idx = pParents[idx];
	}
	return result;
}


bool Hierarchy::TryAdd(ObjectID id, ObjectID parent) {
	let statusOK = !pool.Contains(id);
	if (statusOK) {

		// appending to end?
		if (parent.IsNil()) {
			pool.TryAppendObject(id, INVALID_INDEX, HPOSE_IDENTITY, HPOSE_IDENTITY, PoseMask(ForceInit::Default));
			for(auto it : listeners)
				it->Hierarchy_DidAddObject(this, id);
			return true;
		}

		let parentIdx = IndexOf(parent);
		if (parentIdx == Count() - 1) {
			let parentPose = *pool.GetComponentByIndex<C_WORLD_POSE>(parentIdx);
			pool.TryAppendObject(id, parentIdx, HPOSE_IDENTITY, parentPose, PoseMask(ForceInit::Default));
			for (auto it : listeners)
				it->Hierarchy_DidAddObject(this, id);
			return true;
		}

		// inserting/downshifting
		if (parentIdx != INVALID_INDEX) {
			let parentPose = *pool.GetComponentByIndex<C_WORLD_POSE>(parentIdx);
			pool.TryInsertObjectAfter(id, parent, parentIdx, HPOSE_IDENTITY, parentPose, PoseMask(ForceInit::Default));
			DoShiftIndexes(parentIdx + 1, 1);
			for (auto it : listeners)
				it->Hierarchy_DidAddObject(this, id);
			return true;
		}
	}

	return false;

}
bool Hierarchy::TryReparent(ObjectID id, ObjectID underParent, ReparentMode mode) {
	// ?? In principle could be optimized better for batch-operation ??

	// early out?
	let idx = IndexOf(id);
	if (idx == INVALID_INDEX)
		return false;

	let worldStay = mode == ReparentMode::MaintainWorldPose;
	auto worldPose = *pool.GetComponentByIndex<C_WORLD_POSE>(idx);
	auto relPose = *pool.GetComponentByIndex<C_RELATIVE_POSE>(idx);
	let mask = *pool.GetComponentByIndex<C_MASK>(idx);

	{
		let pidx = IndexOf(underParent); // scope this because we're about to invalidate it

		// noop?
		if (GetParentIndexByIndex(idx) == pidx)
			return true;

		// trying to reparent under a child?
		if (pidx > idx && pidx < GetDescendentRangeByIndex(idx))
			return false;

	}

	// save parent-pointers of descendents, relative to idx
	struct SavedChild {
		ObjectID id;
		int32 relativeParent;
		HPose rel;
		HPose pose;
		PoseMask mask;

		SavedChild(ObjectID aID, int32 aRelativeParent, const HPose& aRel, const HPose& aPose, PoseMask aMask) 
			: id(aID)
			, relativeParent(aRelativeParent) 
			, rel(aRel)
			, pose(aPose)
			, mask(aMask)
		{}
	};
	static eastl::vector<SavedChild> childParents;
	childParents.clear();
	let idxEnd = GetDescendentRangeByIndex(idx);
	let rangeCount = idxEnd - idx;
	{
		let pHandles = pool.GetComponentData<C_HANDLE>();
		let pParents = pool.GetComponentData<C_PARENT>();
		let pRelativePoses = pool.GetComponentData<C_RELATIVE_POSE>();
		let pWorldPoses = pool.GetComponentData<C_WORLD_POSE>();
		let pMask = pool.GetComponentData<C_MASK>();
		for(int it=idx + 1; it<idxEnd; ++it)
			childParents.emplace_back(
				pHandles[it], 
				pParents[it] - idx,
				pRelativePoses[it],
				pWorldPoses[it],
				pMask[it]
			);
	}

	// remove range from tree
	pool.DoReleaseCompactRange_Shift(idx, idxEnd);
	DoShiftIndexes(idx, -rangeCount);

	// add to new parent
	let n = Count();
	let parentIdx = IndexOf(underParent);
	if (worldStay)
		relPose = mask.Rebase(parentIdx == INVALID_INDEX ? HPOSE_IDENTITY : *pool.GetComponentByIndex<C_WORLD_POSE>(parentIdx), worldPose);
	else
		worldPose = mask.Concat(*pool.GetComponentByIndex<C_WORLD_POSE>(parentIdx), relPose);

	if (parentIdx == INVALID_INDEX || parentIdx == n - 1) {
		
		// simply append to the end
		pool.TryAppendObject(id, parentIdx, relPose, worldPose, mask);
		if (worldStay) {
			for (auto it : childParents)
				pool.TryAppendObject(it.id, it.relativeParent + n, it.rel, it.pose, mask);
		} else {
			for (auto it : childParents) {
				let parentIdx = it.relativeParent + n;
				let parentPose = *pool.GetComponentByIndex<C_WORLD_POSE>(parentIdx);
				pool.TryAppendObject(it.id, parentIdx, it.rel, it.mask.Concat(parentPose, it.rel), mask);
			}
		}

	} else {
		
		// insert after parent and downshift
		let newIdx = parentIdx + 1;
		pool.TryInsertObjectAt(id, newIdx, parentIdx, relPose, worldPose, mask);
		auto nextIdx = newIdx;
		if (worldStay) {
			for(auto it : childParents) {
				++nextIdx;
				pool.TryInsertObjectAt(it.id, nextIdx, it.relativeParent + newIdx, it.rel, it.pose, mask);
			}
		} else {
			for (auto it : childParents) {
				++nextIdx;
				let parentIdx = it.relativeParent + newIdx;
				let parentPose = *pool.GetComponentByIndex<C_WORLD_POSE>(parentIdx);
				pool.TryInsertObjectAt(it.id, nextIdx, parentIdx, it.rel, it.mask.Concat(parentPose, it.rel), mask);
			}
		}

		DoShiftIndexes(newIdx, rangeCount);

	}

	return true;
}

bool Hierarchy::TryRelease(ObjectID id) {
	let idxStart = IndexOf(id);
	if (idxStart == INVALID_INDEX)
		return false;

	// destroy children
	let idxEnd = GetDescendentRangeByIndex(idxStart);
	
	for(int idx=idxStart; idx < idxEnd; ++idx)
		for (auto it : listeners)
			it->Hierarchy_WillRemoveObject(this, GetObjectByIndex(idx));

	// TODO: really should be able to remove components by range
	for (int it = idxStart; it < idxEnd; ++it)
		pool.TryReleaseObject_Shift(GetObjectByIndex(idxStart));

	// downshift elements
	let removeCount = (idxEnd - idxStart);
	DoShiftIndexes(idxStart, -removeCount);
	return true;
}

void Hierarchy::DoShiftIndexes(int32 idx, int32 delta) {
	let count = Count();
	let pParent = pool.GetComponentData<1>();
	let start = delta > 0 ? idx + delta : idx;
	for (auto it = start; it < count; ++it)
		if (pParent[it] >= idx)
			pParent[it] += delta;
}

bool Hierarchy::SetMask(ObjectID id, PoseMask mask, ReparentMode mode) {
	let idx = IndexOf(id);
	return idx != INVALID_INDEX && SetMaskByIndex(idx, mask, mode);
}

bool Hierarchy::SetMaskByIndex(int32 idx, PoseMask mask, ReparentMode mode) {

	// early out?
	auto pMask = pool.GetComponentData<C_MASK>();
	if (mask.allIgnoreBits == pMask[idx].allIgnoreBits) {
		return true;
	}

	pMask[idx] = mask;

	auto pParents = pool.GetComponentData<C_PARENT>();
	auto pRelativePoses = pool.GetComponentData<C_RELATIVE_POSE>();
	auto pWorldPoses = pool.GetComponentData<C_WORLD_POSE>();

	let parentIdx = pParents[idx];
	if (parentIdx == INVALID_INDEX) {
		return true;
	}

	if (mode == ReparentMode::MaintainWorldPose) {
		pRelativePoses[idx] = mask.Rebase(pWorldPoses[parentIdx], pWorldPoses[idx]);
	} else {
		pWorldPoses[idx] = mask.Concat(pWorldPoses[parentIdx], pRelativePoses[idx]);
		let idxEnd = GetDescendentRangeByIndex(idx);
		for (auto cidx = idx + 1; cidx != idxEnd; ++cidx)
			pWorldPoses[cidx] = pMask[cidx].Concat(pWorldPoses[pParents[cidx]], pRelativePoses[cidx]);
	}

	return true;
}

#define SET_POSE_PREAMBLE \
	let pParents = pool.GetComponentData<C_PARENT>(); \
	let pRelativePoses = pool.GetComponentData<C_RELATIVE_POSE>(); \
	let pWorldPoses = pool.GetComponentData<C_WORLD_POSE>(); \
	let pMask = pool.GetComponentData<C_MASK>(); \
	let parentIdx = pParents[idx]; \
	let idxEnd = GetDescendentRangeByIndex(idx);

#define SET_POSE_CONCLUSION \
	for (auto cidx = idx + 1; cidx != idxEnd; ++cidx) \
		pWorldPoses[cidx] = pMask[cidx].Concat(pWorldPoses[pParents[cidx]], pRelativePoses[cidx]); \
	return true;


bool Hierarchy::SetRelativePoseByIndex(int32 idx, const HPose & rel) {
	SET_POSE_PREAMBLE
	pRelativePoses[idx] = rel;
	pWorldPoses[idx] = parentIdx >= 0 ? pMask[idx].Concat(pWorldPoses[parentIdx], rel) : rel;
	SET_POSE_CONCLUSION
}

bool Hierarchy::SetRelativePositionByIndex(int32 idx, const vec3& position) {
	SET_POSE_PREAMBLE
	pRelativePoses[idx].position = position;
	pWorldPoses[idx] = parentIdx >= 0 ? pMask[idx].Concat(pWorldPoses[parentIdx], pRelativePoses[idx]) : pRelativePoses[idx];
	SET_POSE_CONCLUSION
}

bool Hierarchy::SetRelativeRotationByIndex(int32 idx, const quat& rotation) {
	SET_POSE_PREAMBLE
	pRelativePoses[idx].rotation = rotation;
	pWorldPoses[idx] = parentIdx >= 0 ? pMask[idx].Concat(pWorldPoses[parentIdx], pRelativePoses[idx]) : pRelativePoses[idx];
	SET_POSE_CONCLUSION
}

bool Hierarchy::SetRelativeScaleByIndex(int32 idx, const vec3& scale) {
	SET_POSE_PREAMBLE
	pRelativePoses[idx].scale = scale;
	pWorldPoses[idx] = parentIdx >= 0 ? pMask[idx].Concat(pWorldPoses[parentIdx], pRelativePoses[idx]) : pRelativePoses[idx];
	SET_POSE_CONCLUSION
}

bool Hierarchy::SetRelativeRigidPoseByIndex(int32 idx, const RPose& pose) {
	SET_POSE_PREAMBLE
	pRelativePoses[idx].rpose = pose;
	pWorldPoses[idx] = parentIdx >= 0 ? pMask[idx].Concat(pWorldPoses[parentIdx], pRelativePoses[idx]) : pRelativePoses[idx];
	SET_POSE_CONCLUSION
}

bool Hierarchy::SetWorldPoseByIndex(int32 idx, const HPose & pose) {
	SET_POSE_PREAMBLE
	pWorldPoses[idx] = pose;
	pRelativePoses[idx] = parentIdx >= 0 ? pMask[idx].Rebase(pWorldPoses[parentIdx], pWorldPoses[idx]) : pWorldPoses[idx];
	SET_POSE_CONCLUSION
}

bool Hierarchy::SetWorldPositionByIndex(int32 idx, const vec3& position) {
	SET_POSE_PREAMBLE
	pWorldPoses[idx].position = position;
	pRelativePoses[idx] = parentIdx >= 0 ? pMask[idx].Rebase(pWorldPoses[parentIdx], pWorldPoses[idx]) : pWorldPoses[idx];
	SET_POSE_CONCLUSION
}

bool Hierarchy::SetWorldRotationByIndex(int32 idx, const quat& rotation) {
	SET_POSE_PREAMBLE
	pWorldPoses[idx].rotation = rotation;
	pRelativePoses[idx] = parentIdx >= 0 ? pMask[idx].Rebase(pWorldPoses[parentIdx], pWorldPoses[idx]) : pWorldPoses[idx];
	SET_POSE_CONCLUSION
}

bool Hierarchy::SetWorldScaleByIndex(int32 idx, const vec3& scale) {
	SET_POSE_PREAMBLE
	pWorldPoses[idx].scale = scale;
	pRelativePoses[idx] = parentIdx >= 0 ? pMask[idx].Rebase(pWorldPoses[parentIdx], pWorldPoses[idx]) : pWorldPoses[idx];
	SET_POSE_CONCLUSION
}

bool Hierarchy::SetWorldRigidPoseByIndex(int32 idx, const RPose& pose) {
	SET_POSE_PREAMBLE
	pWorldPoses[idx].rpose = pose;
	pRelativePoses[idx] = parentIdx >= 0 ? pMask[idx].Rebase(pWorldPoses[parentIdx], pWorldPoses[idx]) : pWorldPoses[idx];
	SET_POSE_CONCLUSION
}

#undef SET_POSE_PREAMBLE
#undef SET_POSE_CONCLUSION

void Hierarchy::SanityCheck() {
	auto pParent = pool.GetComponentData<C_PARENT>();
	auto pRelativePoses = pool.GetComponentData<C_RELATIVE_POSE>();
	auto pWorldPoses = pool.GetComponentData<C_WORLD_POSE>();
	let n = pool.Count();

	for(int it=0; it<n; ++it) {
		DEBUG_ASSERT(*pParent == INVALID_INDEX || *pParent < it);
		DEBUG_ASSERT(!ContainsNaN(*pRelativePoses));
		DEBUG_ASSERT(!ContainsNaN(*pWorldPoses));
		DEBUG_ASSERT(IsNormalized(*pRelativePoses));
		DEBUG_ASSERT(IsNormalized(*pWorldPoses));
		++pParent;
		++pRelativePoses;
		++pWorldPoses;
	}
}