// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "ObjectPool.h"
#include "Name.h"
#include "Hierarchy.h"
#include "Math.h"
#include "Listener.h"

class Scene;

//------------------------------------------------------------------------------------------
// Weak-ptr wrapper to check for use-after-free

template<typename T>
class SceneRef {
private:
	T* ptr;
	#if TRINKET_CHECKED
	ObjectID id;
	#endif

public:
	SceneRef() noexcept = default;
	SceneRef(const SceneRef<T>&) noexcept = default;
	SceneRef(SceneRef<T>&&) noexcept = default;
	SceneRef<T>& operator=(const SceneRef<T>&) noexcept = default;

	SceneRef(ForceInit) noexcept
		: ptr(nullptr) 
	{
		#if TRINKET_CHECKED	
		id = OBJECT_NIL;
		#endif
	}

	SceneRef(T* aPtr) noexcept
		: ptr(aPtr)
	{
		#if TRINKET_CHECKED	
		id = ptr->ID();
		CHECK_ASSERT(!id.IsFingerprinted());
		#endif
	}

	inline T* GetComponent(const Scene* pScene);
	inline T* GetComponent(const Scene* pScene) const;

	void Reset() {
		ptr = nullptr;
		#if TRINKET_CHECKED	
		id = OBJECT_NIL;
		#endif
	}
};

//------------------------------------------------------------------------------------------
// Interface for listening to world events

class ISceneListener {
public:
	virtual void Scene_WillReleaseObject(Scene* caller, ObjectID id) {}
};

//------------------------------------------------------------------------------------------
// A world represents a complete scene, partitioned into hierarchical layers.
// TODO: Support Clone()ing worlds for play-in-editor (PIE).
//       (alternatively, spawn new processes for PIE?)

class Scene : IHierarchyListener
{
private:

	enum Components { C_HANDLE, C_NAME };
	enum SublevelComponent { C_HIERARCHY = 1 };
	enum SceneComponents { C_SUBLEVEL = 1 };

	ObjectMgr<Name> mgr;
	ObjectPool<StrongRef<Hierarchy>> sublevels;
	ObjectPool<SceneRef<Hierarchy>> sceneObjects;
	ListenerList<ISceneListener> listeners;

public:

	Scene();
	~Scene();

	void AddListener(ISceneListener* listener) { listeners.TryAdd(listener); }
	void RemoveListener(ISceneListener* listener) { listeners.TryRemove_Swap(listener); }

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
// SceneRef Impl

template<typename T>
inline T* SceneRef<T>::GetComponent(const Scene* pScene) {
	CHECK_ASSERT(ptr == nullptr || pScene->IsValid(id));
	return ptr;
}

template<typename T>
inline T* SceneRef<T>::GetComponent(const Scene* pScene) const {
	CHECK_ASSERT(ptr == nullptr || pScene->IsValid(id));
	return ptr;
}
