// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Listener.h"
#include "Name.h"
#include "ObjectPool.h"

// TODO: Filesystem Abstraction (physfs?)
class AssetDatabase;

//------------------------------------------------------------------------------------------
// Weak-ptr wrapper to check for use-after-free

template<typename T>
class AssetRef {
private:
	T* ptr;
	#if TRINKET_CHECKED
	ObjectID id;
	#endif

public:
	AssetRef() noexcept = default;
	AssetRef(const AssetRef<T>&) noexcept = default;
	AssetRef(AssetRef<T>&&) noexcept = default;
	AssetRef<T>& operator=(const AssetRef<T>&) noexcept = default;

	AssetRef(ForceInit) noexcept
		: ptr(nullptr) {
		#if TRINKET_CHECKED	
		id = OBJECT_NIL;
		#endif
	}

	AssetRef(T* aPtr) noexcept
		: ptr(aPtr) 
	{
		#if TRINKET_CHECKED	
		id = ptr->ID();
		CHECK_ASSERT(id.IsFingerprinted());
		#endif
	}

	inline T* GetComponent(const AssetDatabase* pAssets);
	inline T* GetComponent(const AssetDatabase* pAssets) const;

	void Reset() {
		ptr = nullptr;
		#if TRINKET_CHECKED	
		id = OBJECT_NIL;
		#endif
	}
};

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

//------------------------------------------------------------------------------------------
// AssetRef Impl

template<typename T>
inline T* AssetRef<T>::GetComponent(const AssetDatabase* pAssets) {
	CHECK_ASSERT(ptr == nullptr || pAssets->IsValid(id));
	return ptr;
}

template<typename T>
inline T* AssetRef<T>::GetComponent(const AssetDatabase* pAssets) const {
	CHECK_ASSERT(ptr == nullptr || pAssets->IsValid(id));
	return ptr;
}
