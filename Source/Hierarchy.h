#pragma once
#include "Listener.h"
#include "ObjectPool.h"
#include "Pose.h"

//------------------------------------------------------------------------------------------
// Represents a transform-tree

class Hierarchy;

class IHierarchyListener {
public:

	virtual void Hierarchy_DidAddObject(Hierarchy* hierarchy, ObjectID id) {}
	virtual void Hierarchy_WillRemoveObject(Hierarchy* hierarchy, ObjectID id) {}

};

enum class ReparentMode {
	MaintainWorldPose,
	MaintainRelativePose
};

class Hierarchy : public ObjectComponent {
private:

	enum Components { C_HANDLE, C_PARENT, C_RELATIVE_POSE, C_WORLD_POSE, C_MASK };
	ObjectPool<int32, HPose, HPose, PoseMask> pool;
	ListenerList<IHierarchyListener> listeners;

public:

	Hierarchy(ObjectID id) noexcept;

	// WORLD METHODS

	void AddListener(IHierarchyListener* listener) { listeners.TryAdd(listener); }
	void RemoveListener(IHierarchyListener* listener) { listeners.TryRemove_Swap(listener); }

	const ObjectID* GetObjects() const { return pool.GetComponentData<C_HANDLE>(); }
	ObjectID GetObjectByIndex(int32 idx) const { return *pool.GetComponentByIndex<C_HANDLE>(idx); }

	auto begin() { return pool.begin(); }
	auto begin() const { return pool.begin(); }
	auto end() { return pool.end(); }
	auto end() const { return pool.end(); }
	auto operator[](int idx) { return pool[idx]; }
	auto operator[](int idx) const { return pool[idx]; }

	// HIERARCHY METHODS

	bool Contains(ObjectID id) const { return pool.Contains(id); }
	int32 Count() const { return pool.Count(); }
	int32 IndexOf(ObjectID id) const { return pool.IndexOf(id); }
	
	bool HasChildren(ObjectID id) const;
	bool HasChildrenByIndex(int32 idx) const;
	int32 GetDescendentRangeByIndex(int32 startIndex) const;
	int32 GetDescendentCount(ObjectID id) const;
	
	bool IsRoot(ObjectID id) const;
	bool IsRootIndex(int32 idx) const { return GetParentIndexByIndex(idx) < 0; }

	ObjectID GetParent(ObjectID id) const;
	int32 GetParentIndexByIndex(int32 idx) const { return *pool.GetComponentByIndex<C_PARENT>(idx); }
	const int32* GetParentData() const { return pool.GetComponentData<C_PARENT>(); }

	int32 GetDepth(ObjectID id) const;
	int32 GetDepthByIndex(int32 idx) const;

	bool TryAdd(ObjectID id, ObjectID parent = OBJECT_NIL);
	bool TryRelease(ObjectID id);
	bool TryReparent(ObjectID id, ObjectID underParent = OBJECT_NIL, ReparentMode mode = ReparentMode::MaintainWorldPose);

	// TRANSFORM METHODS

	const HPose* GetRelativePose(ObjectID id) const { return pool.TryGetComponent<C_RELATIVE_POSE>(id); }
	const HPose* GetRelativePoseByIndex(int32 idx) const { return pool.GetComponentByIndex<C_RELATIVE_POSE>(idx); }
	const HPose* GetRelativePoseData() const { return pool.GetComponentData<C_RELATIVE_POSE>(); }
	
	const HPose* GetWorldPose(ObjectID id) const { return pool.TryGetComponent<C_WORLD_POSE>(id); }
	const HPose* GetWorldPoseByIndex(int32 idx) const { return pool.GetComponentByIndex<C_WORLD_POSE>(idx); }
	const HPose* GetWorldPoseData() const { return pool.GetComponentData<C_WORLD_POSE>(); }

	const PoseMask* GetMask(ObjectID id) const { return pool.TryGetComponent<C_MASK>(id); }
	const PoseMask* GetMaskByIndex(int32 idx) const { return pool.GetComponentByIndex<C_MASK>(idx); }
	const PoseMask* GetMaskData() const { return pool.GetComponentData<C_MASK>(); }

	bool SetMask(ObjectID id, PoseMask mask, ReparentMode mode = ReparentMode::MaintainWorldPose);
	bool SetMaskByIndex(int32 idx, PoseMask mask, ReparentMode mode = ReparentMode::MaintainWorldPose);

	bool SetRelativePose(ObjectID id, const HPose& pose) { let idx = IndexOf(id); return idx != INVALID_INDEX && SetRelativePoseByIndex(idx, pose); }
	bool SetRelativePosition(ObjectID id, const vec3& pos) { let idx = IndexOf(id); return idx != INVALID_INDEX && SetRelativePositionByIndex(idx, pos); }
	bool SetRelativeRotation(ObjectID id, const quat& rot) { let idx = IndexOf(id); return idx != INVALID_INDEX && SetRelativeRotationByIndex(idx, rot); }
	bool SetRelativeScale(ObjectID id, const vec3& scale) { let idx = IndexOf(id); return idx != INVALID_INDEX && SetRelativeScaleByIndex(idx, scale); }
	bool SetRelativeRigidPose(ObjectID id, const RPose& pose) { let idx = IndexOf(id); return idx != INVALID_INDEX && SetRelativeRigidPoseByIndex(idx, pose); }

	bool SetRelativePoseByIndex(int32 idx, const HPose& pose);
	bool SetRelativePositionByIndex(int32 idx, const vec3& position);
	bool SetRelativeRotationByIndex(int32 idx, const quat& rotation);
	bool SetRelativeScaleByIndex(int32 idx, const vec3& scale);
	bool SetRelativeRigidPoseByIndex(int32 idx, const RPose& pose);

	bool SetWorldPose(ObjectID id, const HPose& pose) { let idx = IndexOf(id); return idx != INVALID_INDEX && SetWorldPoseByIndex(idx, pose); }
	bool SetWorldPosition(ObjectID id, const vec3& pos) { let idx = IndexOf(id); return idx != INVALID_INDEX && SetWorldPositionByIndex(idx, pos); }
	bool SetWorldRotation(ObjectID id, const quat& rot) { let idx = IndexOf(id); return idx != INVALID_INDEX && SetWorldRotationByIndex(idx, rot); }
	bool SetWorldScale(ObjectID id, const vec3& scale) { let idx = IndexOf(id); return idx != INVALID_INDEX && SetWorldScaleByIndex(idx, scale); }
	bool SetWorldRigidPose(ObjectID id, const RPose& pose) { let idx = IndexOf(id); return idx != INVALID_INDEX && SetWorldRigidPoseByIndex(idx, pose); }


	bool SetWorldPoseByIndex(int32 idx, const HPose& pose);
	bool SetWorldPositionByIndex(int32 idx, const vec3& position);
	bool SetWorldRotationByIndex(int32 idx, const quat& rotation);
	bool SetWorldScaleByIndex(int32 idx, const vec3& scale);
	bool SetWorldRigidPoseByIndex(int32 idx, const RPose& pose);

	// TODO: Add Batch-Update Versions (e.g. for physics updates)

	void SanityCheck();

private:
	void DoShiftIndexes(int32 idx, int32 delta);

};

//------------------------------------------------------------------------------------------
// Iterate over roots

class HierarchyRootIterator {
private:
	const Hierarchy* pHierarchy;
	int32 idxCurrent;

public:
	HierarchyRootIterator(const Hierarchy* hierarchy)
		: pHierarchy(hierarchy),
		idxCurrent(0)
	{}

	bool MoveNext() {
		let n = pHierarchy->Count();
		do { ++idxCurrent; } while(idxCurrent < n && pHierarchy->GetParentIndexByIndex(idxCurrent) >= 0);
		return idxCurrent < n;
	}

	const Hierarchy& GetHierarchy() const { return *pHierarchy; }
	ObjectID GetObject() const { return pHierarchy->GetObjectByIndex(idxCurrent); }
	int32 GetIndex() const { return idxCurrent; }

};

//------------------------------------------------------------------------------------------
// Iterate over parents

class HierarchyParentIterator {
private:
	const Hierarchy* pHierarchy;
	int32 idxCurrent;

public:

	HierarchyParentIterator(const Hierarchy* hierarchy, ObjectID id)
		: pHierarchy(hierarchy)
		, idxCurrent(hierarchy->IndexOf(id))
	{}

	HierarchyParentIterator(const Hierarchy* hierarchy, int32 idx)
		: pHierarchy(hierarchy)
		, idxCurrent(idx) 
	{}


	bool MoveNext() { 
		if (idxCurrent != INVALID_INDEX) 
			idxCurrent = pHierarchy->GetParentIndexByIndex(idxCurrent); 
		return idxCurrent >= 0; 
	}

	const Hierarchy& GetHierarchy() const { return *pHierarchy; }
	ObjectID GetObject() const { return pHierarchy->GetObjectByIndex(idxCurrent); }
	int32 GetIndex() const { return idxCurrent; }
};


//------------------------------------------------------------------------------------------
// Iterate over all the descendents of an object in the hierarchy
// TODO: invalidation detection?

class HierarchyDescendentIterator {
private:
	const Hierarchy* pHierarchy;
	int32 idxCurrent = INVALID_INDEX;
	int32 idxEnd = INVALID_INDEX;

public:


	HierarchyDescendentIterator(const Hierarchy* hierarchy, ObjectID parent)
		: pHierarchy(hierarchy)
		, idxCurrent(hierarchy->IndexOf(parent)) 
	{
		if (idxCurrent != INVALID_INDEX)
			idxEnd = hierarchy->GetDescendentRangeByIndex(idxCurrent);
	}

	HierarchyDescendentIterator(const Hierarchy* hierarchy, int32 idx)
		: pHierarchy(hierarchy)
		, idxCurrent(idx) 
		, idxEnd(hierarchy->GetDescendentRangeByIndex(idx))
	{}

	bool MoveNext() { return idxCurrent != INVALID_INDEX && (++idxCurrent) < idxEnd; }

	const Hierarchy& GetHierarchy() const { return *pHierarchy; }
	ObjectID GetObject() const { return pHierarchy->GetObjectByIndex(idxCurrent); }
	int32 GetIndex() const { return idxCurrent; }

};

//------------------------------------------------------------------------------------------
// Iterate over just the direct-children of an object in the hierarchy

class HierarchyDirectDescendentIterator {
private:
	HierarchyDescendentIterator it;
	int32 idxParent;

public:

	HierarchyDirectDescendentIterator(Hierarchy* hierarchy, ObjectID parent)
		: it(hierarchy, parent)
		, idxParent(it.GetIndex()) 			
	{}

	HierarchyDirectDescendentIterator(Hierarchy* hierarchy, int32 idx)
		: it(hierarchy, idx)
		, idxParent(idx)
	{}


	bool MoveNext() {
		bool active = false;
		do { active = it.MoveNext(); } while(active && GetHierarchy().GetParentIndexByIndex(GetIndex()) != idxParent);
		return active;
	}

	const Hierarchy& GetHierarchy() const { return it.GetHierarchy(); }
	ObjectID GetObject() const { return it.GetObject(); }
	int32 GetIndex() const { return it.GetIndex(); }
};
