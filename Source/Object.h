#pragma once
#include "Common.h"


#define OBJ_ITEM_BITS        15
#define OBJ_PAGE_BITS        5
#define OBJ_INDICES_PER_PAGE (1<<OBJ_ITEM_BITS)
#define OBJ_MAX_PAGES        (1<<OBJ_PAGE_BITS)
#define OBJ_INDEX_BITS       (OBJ_ITEM_BITS + OBJ_PAGE_BITS)
#define MAX_OBJECTS          (1<<OBJ_INDEX_BITS)
#define OBJECT_NIL           (ObjectID(ForceInit::Default))

union ObjectID {
	uint32 handle;

	struct {
		// implementation details -- ordinarily we'd hide this, but
		// we really want to the compiler to optimize this code as 
		// much as possible.
		uint32 itemIdx : OBJ_ITEM_BITS;
		uint32 pageIdx : OBJ_PAGE_BITS;
		uint32 version : 11;
		uint32 fingerprint : 1;
	};

	ObjectID() = default;
	ObjectID(const ObjectID&) = default;
	ObjectID(ObjectID&&) = default;
	ObjectID& operator=(const ObjectID&) = default;

	explicit ObjectID(uint32 aHandle) noexcept : handle(aHandle) {}
	explicit ObjectID(ForceInit) noexcept : handle(0xffffffff) {}

	bool IsNil() const { return handle == 0xffffffff; }
	bool IsFingerprinted() const { return fingerprint != 0; }

	bool operator==(const ObjectID& rhs) const { return handle == rhs.handle; }
	bool operator!=(const ObjectID& rhs) const { return handle != rhs.handle; }
	bool operator<(const ObjectID& rhs) const { return handle < rhs.handle; }
	bool operator>(const ObjectID& rhs) const { return handle > rhs.handle; }
	bool operator<=(const ObjectID& rhs) const { return handle <= rhs.handle; }
	bool operator>=(const ObjectID& rhs) const { return handle >= rhs.handle; }

	ObjectID NextVersion() const {
		auto result = *this;
		++result.version;
		return result;
	}

	ObjectID NextIndex() const {
		auto result = *this;
		++result.itemIdx;
		if (result.itemIdx == 0)
			++result.pageIdx;
		return result;
	}

	ObjectID WithFingerprint(bool fingerprint) {
		auto result = *this;
		result.fingerprint = fingerprint ? 1 : 0;
		return result;
	}
};

// "Objects" are a logical concept, so there's no requirement for object-components
// to subtype the following interface. It exists only  to eliminate boilerplate and 
// automate free()ing pointers when removed from an ObjectPool.

// Maybe give it a more narrowly-tailored name? PoolOwnedComponent?

class ObjectComponent {
private:
	ObjectID id;

public:
	ObjectComponent(ObjectID aID) noexcept : id(aID) {}
	ObjectComponent(ObjectComponent&&) = default;

	// no copies
	ObjectComponent(const ObjectComponent&) = delete;
	ObjectComponent& operator=(const ObjectComponent&) = delete;

	virtual ~ObjectComponent() {}

	ObjectID ID() const { return id; }
};

// Wrappers for new/delete
template<class T, class... Args>
inline T* NewObjectComponent(Args&&... args) {
	static_assert(std::is_convertible<T*, ObjectComponent*>::value);
	return new T(std::forward<Args>(args)...);
}
inline void FreeObjectComponent(ObjectComponent* obj) {
	delete obj;
}

// Generic method for releasing components
template<typename T>
void TryFreeObjectComponent(T&& p) {
	if constexpr (std::is_convertible<T, ObjectComponent*>::value) {
		FreeObjectComponent(p);
	}
}

template<typename T>
inline T* DerefPP(T** ppComponent) {
	return ppComponent ? *ppComponent : nullptr;
}
