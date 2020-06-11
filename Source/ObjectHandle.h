#pragma once
#include "ObjectPool.h"

enum class ObjectTag : uint32 {
	UNDEFINED = 0,
	SCENE_OBJECT,
	SUBLEVEL_OBJECT,
	MATERIAL_ASSET,
	MESH_ASSET,
};

union ObjectHandle {
	// pack tagged object handles into a void* so we can use them
	// as "light user data" in lua (which incurs no GC overhead), or
	// as "user data" on physx objects.

	void* p;
	struct {
		ObjectTag tag;
		ObjectID id;
	};

	ObjectHandle() noexcept {}
	ObjectHandle(ForceInit) noexcept : p(nullptr) {}
	ObjectHandle(void* a) noexcept : p(a) {}
	ObjectHandle(ObjectTag atype, ObjectID aid) noexcept : tag(atype), id(aid) {}

	bool IsUndefined() const { return tag == ObjectTag::UNDEFINED; }
	bool IsSceneObject() const { return tag == ObjectTag::SCENE_OBJECT; }
	bool IsSublevel() const { return tag == ObjectTag::SUBLEVEL_OBJECT; }
	bool IsMaterialAsset() const { return tag == ObjectTag::MATERIAL_ASSET; }
	bool IsMeshAsset() const { return tag == ObjectTag::MESH_ASSET; }
	bool IsNil() const { return id.IsNil(); }
};

static_assert(sizeof(ObjectHandle) == sizeof(void*));
