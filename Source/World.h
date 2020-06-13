#pragma once
#include "ObjectPool.h"
#include "Name.h"
#include "Hierarchy.h"
#include "Pose.h"
#include "Listener.h"

class World;

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
	ObjectPool<Hierarchy*> sublevels;
	ObjectPool<ObjectID> sceneObjects;
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

	ObjectID GetSublevel(ObjectID id) { let result = sceneObjects.TryGetComponent<C_SUBLEVEL>(id); return result ? *result : OBJECT_NIL; }
	Hierarchy* GetSublevelHierarchyFor(ObjectID id) { return DerefPP(sublevels.TryGetComponent<C_HIERARCHY>(GetSublevel(id))); }

	void SanityCheck();

private:

	void Hierarchy_DidAddObject(Hierarchy* hierarchy, ObjectID id) override;
	void Hierarchy_WillRemoveObject(Hierarchy* hierarchy, ObjectID id) override;
};

//------------------------------------------------------------------------------------------
// Component-pools that auto-remove released handles

template<typename... Ts>
class OrderedWorldPool	: public ObjectPool<Ts...>, IWorldListener
{
private:
	World* pWorld;

public:

	OrderedWorldPool(World* aWorld) : pWorld(aWorld) { if (pWorld) pWorld->AddListener(this); }
	~OrderedWorldPool() { if (pWorld) pWorld->RemoveListener(this); }

	void World_WillReleaseObject(World* caller, ObjectID id) override { this->TryReleaseObject_Shift(id); }
};

template<typename... Ts>
class UnorderedWorldPool : public ObjectPool<Ts...>, IWorldListener
{
private:
	World* pWorld;

public:

	UnorderedWorldPool(World* aWorld) : pWorld(aWorld) { if (pWorld) pWorld->AddListener(this); }
	~UnorderedWorldPool() { if (pWorld) pWorld->RemoveListener(this); }

	void World_WillReleaseObject(World* caller, ObjectID id) override { this->TryReleaseObject_Swap(id); }
};
