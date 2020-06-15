// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "ObjectPool.h"
#include "Name.h"
#include "Hierarchy.h"
#include "Math.h"
#include "Listener.h"

class World;

//------------------------------------------------------------------------------------------
// Weak-ptr wrapper to check for use-after-free

template<typename T>
class WorldRef {
private:
	T* ptr;
	#if TRINKET_CHECKED
	ObjectID id;
	#endif

public:
	WorldRef() noexcept = default;
	WorldRef(const WorldRef<T>&) noexcept = default;
	WorldRef(WorldRef<T>&&) noexcept = default;
	WorldRef<T>& operator=(const WorldRef<T>&) noexcept = default;

	WorldRef(ForceInit) noexcept
		: ptr(nullptr) 
	{
		#if TRINKET_CHECKED	
		id = OBJECT_NIL;
		#endif
	}

	WorldRef(T* aPtr) noexcept
		: ptr(aPtr)
	{
		#if TRINKET_CHECKED	
		id = ptr->ID();
		CHECK_ASSERT(!id.IsFingerprinted());
		#endif
	}

	inline T* GetComponent(const World* pWorld);
	inline T* GetComponent(const World* pWorld) const;

	void Reset() {
		ptr = nullptr;
		#if TRINKET_CHECKED	
		id = OBJECT_NIL;
		#endif
	}
};

//------------------------------------------------------------------------------------------
// Interface for listening to world events

class IWorldListener {
public:
	virtual void World_WillReleaseObject(World* caller, ObjectID id) {}
};

//------------------------------------------------------------------------------------------
// A world represents a complete scene, partitioned into hierarchical layers.
// TODO: Support Clone()ing worlds for play-in-editor (PIE).
//       (alternatively, spawn new processes for PIE?)

class World : IHierarchyListener
{
private:

	enum Components { C_HANDLE, C_NAME };
	enum SublevelComponent { C_HIERARCHY = 1 };
	enum SceneComponents { C_SUBLEVEL = 1 };

	ObjectMgr<Name> mgr;
	ObjectPool<StrongRef<Hierarchy>> sublevels;
	ObjectPool<WorldRef<Hierarchy>> sceneObjects;
	ListenerList<IWorldListener> listeners;

public:

	World();
	~World();

	void AddListener(IWorldListener* listener) { listeners.TryAdd(listener); }
	void RemoveListener(IWorldListener* listener) { listeners.TryRemove_Swap(listener); }

	ObjectID CreateObject(Name name);
	ObjectID CreateSublevel(Name name);

	Name  GetName(ObjectID id) const;
	ObjectID FindObject(Name name) const;

	void TryRename(ObjectID id, Name name);
	void TryReleaseObject(ObjectID id);

	bool IsValid(ObjectID id) const { return mgr.GetPool().Contains(id); }
	
	bool IsSceneObject(ObjectID id) { return sceneObjects.Contains(id); }
	int GetSceneObjectCount() const { return sceneObjects.Count(); }
	ObjectID GetSceneObjectByIndex(int32 idx) const { return *sceneObjects.GetComponentByIndex<C_HANDLE>(idx); }
	
	bool IsSublevel(ObjectID id) { return sublevels.Contains(id); }
	int GetSublevelCount() const { return sublevels.Count(); }
	ObjectID GetSublevelByIndex(int32 idx) const { return *sublevels.GetComponentByIndex<C_HANDLE>(idx); }
	Hierarchy* GetHierarchy(ObjectID id) { return DerefPP(sublevels.TryGetComponent<C_HIERARCHY>(id)); }
	Hierarchy* GetHierarchyByIndex(int32 idx) { return *sublevels.GetComponentByIndex<C_HIERARCHY>(idx); }

	ObjectID GetSublevel(ObjectID id) { let result = sceneObjects.TryGetComponent<C_SUBLEVEL>(id); return result ? result->GetComponent(this)->ID() : OBJECT_NIL; }
	Hierarchy* GetSublevelHierarchyFor(ObjectID id) { let result = sceneObjects.TryGetComponent<C_SUBLEVEL>(id); return result ? result->GetComponent(this) : nullptr; }

	void SanityCheck();

private:

	void Hierarchy_DidAddObject(Hierarchy* hierarchy, ObjectID id) override;
	void Hierarchy_WillRemoveObject(Hierarchy* hierarchy, ObjectID id) override;
};

//------------------------------------------------------------------------------------------
// WorldRef Impl

template<typename T>
inline T* WorldRef<T>::GetComponent(const World* pWorld) {
	CHECK_ASSERT(ptr == nullptr || pWorld->IsValid(id));
	return ptr;
}

template<typename T>
inline T* WorldRef<T>::GetComponent(const World* pWorld) const {
	CHECK_ASSERT(ptr == nullptr || pWorld->IsValid(id));
	return ptr;
}
