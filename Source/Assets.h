// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Listener.h"
#include "Name.h"
#include "ObjectPool.h"
#include "AssetData.h"

// TODO: Filesystem Abstraction (physfs?)


class AssetDatabase;

//------------------------------------------------------------------------------------------
// Asset Event Listener Interface

class IAssetListener {
public:
	virtual void Database_WillReleaseAsset(AssetDatabase* caller, ObjectID id) {}
};

//------------------------------------------------------------------------------------------
// Asset Database

class AssetDatabase {
private:

	enum Components { C_HANDLE, C_NAME, C_REF_COUNT };

	ObjectMgr<Name, int32> mgr;   // shared resource objects
	ObjectPool<AssetDataRef> data;
	ListenerList<IAssetListener> listeners;

public:

	AssetDatabase();
	~AssetDatabase();

	// event interface
	void AddListener(IAssetListener* listener) { listeners.TryAdd(listener); }
	void RemoveListener(IAssetListener* listener) { listeners.TryRemove_Swap(listener); }

	// asset object lifecycle
	ObjectID CreateObject(Name name);
	void AddRef(ObjectID id);
	void Release(ObjectID id);

	bool IsValid(ObjectID id) const { return mgr.GetPool().Contains(id); }

	// asset names
	Name  GetName(ObjectID id) const;
	ObjectID FindAsset(Name name) const;
	void TryRename(ObjectID id, Name name);

	// asset-data in-memory caching
	void CacheAssetData(ObjectID id, AssetDataHeader* pData);
	void ClearAssetData(ObjectID id);

	template<typename T>
	const T* GetAssetData(ObjectID id) const {
		let pRef = data.TryGetComponent<1>(id);
		return pRef ? pRef->Get<T>() : nullptr;
	}

};
