// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Assets.h"

AssetDatabase::AssetDatabase() : mgr(true) {
	mgr.ReserveCompact(1024);
}

AssetDatabase::~AssetDatabase() {
}

ObjectID AssetDatabase::CreateObject(Name name) {
	return mgr.CreateObject(name, 1);
}

void AssetDatabase::AddRef(ObjectID id) {
	let idx = mgr.GetPool().IndexOf(id);
	if (idx == INVALID_INDEX)
		return;

	++(*mgr.GetComponentByIndex<C_REF_COUNT>(idx));

}

void AssetDatabase::Release(ObjectID id) {
	let idx = mgr.GetPool().IndexOf(id);
	if (idx == INVALID_INDEX)
		return;
	let pRefCount = mgr.GetComponentByIndex<C_REF_COUNT>(idx);
	--(*pRefCount);
	if ((*pRefCount) == 0) {
		for (auto listener : listeners)
			listener->Database_WillReleaseAsset(this, id);
		mgr.ReleaseObject(id);
	}
}

Name  AssetDatabase::GetName(ObjectID id) const {
	let pName = mgr.GetPool().TryGetComponent<C_NAME>(id);
	return pName ? *pName : Name(ForceInit::Default);
}

ObjectID AssetDatabase::FindAsset(Name name) const {
	let pNames = mgr.GetPool().GetComponentData<C_NAME>();
	let n = mgr.GetPool().Count();
	for (int it = 0; it < n; ++it) {
		if (name == pNames[it])
			return *mgr.GetPool().GetComponentByIndex<C_HANDLE>(it);
	}
	return OBJECT_NIL;
}

void AssetDatabase::TryRename(ObjectID id, Name name) {
	if (let pName = mgr.TryGetComponent<C_NAME>(id))
		*pName = name;
}
//
//eastl::string AssetDatabase::GetConfigPath(ObjectID id) const {
//	let pName = mgr.GetPool().TryGetComponent<C_NAME>(id);
//	return pName ? "Assets/" + pName->GetString() + ".ini" : "";
//}

void AssetDatabase::RegisterAssetData(ObjectID id, AssetDataHeader* pData) {
	data.TryAppendObject(id, pData);
}

void AssetDatabase::ClearAssetData(ObjectID id) {
	data.TryReleaseObject_Swap(id);
}
