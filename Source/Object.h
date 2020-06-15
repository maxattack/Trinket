// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

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
// to subtype the following interface. You can just add any copy or move-able 
// value to an object pool. However, sometimes we'd like to reference these 
// components directly with a pointer, in which case the following base-type
// automates including heap-pointers in object pools which are auto free()ed.

class ObjectComponent {
private:
	ObjectID id;

public:
	ObjectComponent(ObjectID aID) noexcept : id(aID) {}
	ObjectComponent(ObjectComponent&&) noexcept = default;
	ObjectComponent& operator=(ObjectComponent&&) noexcept = default;

	// no copies
	ObjectComponent(const ObjectComponent&) = delete;
	ObjectComponent& operator=(const ObjectComponent&)= delete;

	virtual ~ObjectComponent() {}

	ObjectID ID() const { return id; }
};

template<typename T>
class StrongRef {
private:
	T* ptr;

public:
	StrongRef() noexcept : ptr(nullptr) {};
	StrongRef(T* aPtr) noexcept : ptr(aPtr) {}

	StrongRef& operator=(StrongRef<T>&& other) noexcept {
		ptr = other.ptr;
		other.ptr = nullptr;
		return *this;
	}

	StrongRef(StrongRef<T>&& other) noexcept
		: ptr(other.ptr) 
	{
		other.ptr = nullptr;
	}

	// no copy or assignment
	StrongRef(const StrongRef<T>&) = delete;
	StrongRef& operator=(const StrongRef<T>&) = delete;

	~StrongRef() {
		if (ptr)
			FreeObjectComponent(ptr);
	}

	inline bool operator==(const T* rhs) { return ptr == rhs; }
	inline bool operator!=(const T* rhs) { return ptr != rhs; }

	inline T* operator->() { return ptr; }
	inline const T* operator->() const { return ptr; }

	inline operator T*() { return ptr; }
	inline operator const T*() const { return ptr; }

	inline T* Get() { return ptr; }
	inline T* Get() const { return ptr; }
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

// ptr-to-ptr null coalescing helpers
template<typename T>
inline T* DerefPP(StrongRef<T>* ppComponent) {
	return ppComponent ? ppComponent->Get() : nullptr;
}
template<typename T>
inline T* DerefPP(T** ppComponent) {
	return ppComponent ? *ppComponent : nullptr;
}

