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

	void AddListener(IAssetListener* listener) { listeners.TryAdd(listener); }
	void RemoveListener(IAssetListener* listener) { listeners.TryRemove_Swap(listener); }

	bool IsValid(ObjectID id) const { return mgr.GetPool().Contains(id); }

	ObjectID CreateObject(Name name);
	void AddRef(ObjectID id);
	void Release(ObjectID id);

	Name  GetName(ObjectID id) const;
	ObjectID FindAsset(Name name) const;
	void TryRename(ObjectID id, Name name);

	void RegisterAssetData(ObjectID id, AssetDataHeader* pData);
	void ClearAssetData(ObjectID id);

	template<typename T>
	const T* GetAssetData(ObjectID id) const {
		let pRef = data.TryGetComponent<1>(id);
		return pRef ? pRef->Get<T>() : nullptr;
	}

	//template<typename T>
	//ObjectID RecacheAssetFromConfig(const char* configPath) {
	//	let pData = LoadAssetDataFromConfig<T>(configPath);
	//	if (!pData)
	//		return OBJECT_NIL;
	//	const Name configName(configPath);
	//	ObjectID id = FindAsset(configName);
	//	if (id.IsNil())
	//		id = CreateObject(configName);
	//	if (id.IsNil()) {
	//		FreeAssetData(pData);
	//		return OBJECT_NIL;
	//	}
	//	let pRef = data.TryGetComponent<1>(id);
	//	if (pRef) {
	//		pRef->SetData(data);
	//	} else if (!data.TryAppendObject(id, pData)) {
	//		FreeAssetData(pData);
	//		return OBJECT_NIL;
	//	}
	//	return id;
	//}

};
