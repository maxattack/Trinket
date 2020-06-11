#pragma once
#include "Listener.h"
#include "Name.h"
#include "ObjectPool.h"

// TODO: Filesystem Abstraction (physfs?)

class AssetDatabase;

class IAssetListener {
public:
	virtual void Database_WillReleaseAsset(AssetDatabase* caller, ObjectID id) {}
};


class AssetDatabase {
private:

	enum Components { C_HANDLE, C_NAME, C_REF_COUNT };

	ObjectMgr<Name, int32> mgr;   // shared resource objects
	ListenerList<IAssetListener> listeners;

public:

	AssetDatabase();
	~AssetDatabase();

	void AddListener(IAssetListener* listener) { listeners.TryAdd(listener); }
	void RemoveListener(IAssetListener* listener) { listeners.TryRemove_Swap(listener); }

	bool IsValid(ObjectID id) const { return mgr.GetPool().Contains(id); }

	ObjectID CreateObject(Name name);
	void AddRef(ObjectID id);
	void Release(ObjectID id);

	Name  GetName(ObjectID id) const;
	ObjectID FindAsset(Name name) const;
	void TryRename(ObjectID id, Name name);

};

template<typename... Ts>
class UnorderedAssetPool : public ObjectPool<Ts...>, IAssetListener
{
private:
	AssetDatabase* pAssets;

public:

	UnorderedAssetPool(AssetDatabase* db) : pAssets(db) { if (pAssets) pAssets->AddListener(this); }
	~UnorderedAssetPool() { if (pAssets) pAssets->RemoveListener(this); }

	virtual void Database_WillReleaseAsset(AssetDatabase* caller, ObjectID id) override { this->TryReleaseObject_Swap(id); }
};
