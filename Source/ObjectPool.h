// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Object.h"
#include <EASTL/bonus/tuple_vector.h>
#include <EASTL/vector.h>

union SparsePage {
	int32 items[OBJ_INDICES_PER_PAGE];
	SparsePage* pNext; // unused pages form a freelist
};

class SparseObjectArray {
private:

	// Maps ObjectIDs to Compact Array Indexes

	// We'd prefer not to allocate the 4mb or so it would take to
	// map the whole 20 index-bits of an object ID to the compact array,
	// so instead we break the array up into pages to are allocated
	// on demand, and recycled across pools.

	SparsePage* pages[OBJ_MAX_PAGES];
	uint16 counts[OBJ_MAX_PAGES];

public:

	SparseObjectArray() noexcept { Zero(); }
	~SparseObjectArray() { ReleasePages(); }

	bool  HasPageFor(ObjectID id) const { return pages[id.pageIdx] != nullptr; }
	int32 DoGetIndex(ObjectID id) const { return pages[id.pageIdx]->items[id.itemIdx]; }
	void  DoSetIndex(ObjectID id, int32 idx) const { pages[id.pageIdx]->items[id.itemIdx] = idx; }

	void Clear() {
		ReleasePages();
		Zero();
	}

	void IncrementCount(ObjectID id) {
		++counts[id.pageIdx];
		if (!pages[id.pageIdx])
			pages[id.pageIdx] = AllocPage();
	}

	void DecrementCount(ObjectID id) {
		--counts[id.pageIdx];
		if (counts == 0)
			ReleasePage(id.pageIdx);
	}

private:

	void Zero() {
		memset(pages, 0, sizeof(pages));
		memset(counts, 0, sizeof(counts));
	}
	
	void ReleasePages() {
		for (int it = 0; it < OBJ_MAX_PAGES; ++it)
			ReleasePage(it);
	}

	SparsePage* AllocPage();
	void ReleasePage(int32 pageIdx);

};

template<typename... Ts>
class ObjectPool {
private:

	// Object Components are stored in a Structure-of-Arrays, so
	// memory-locality and SIMD alignment can be exploited.  Additionally,
	// multiple pools can re-use the same ObjectIDs to "attach" components
	// from different systems to the same entity.

	// Unlike a full-blown ECS system, the burden of bookkeeping and keeping
	// multiple pools consistent is left the caller. IMO, the problem is
	// too generic for an implementation that is both safe, flexible, and fast.

	// Component<0> will be the ObjectID handles themselves, and combined
	// with the sparse object array implement the "Sparse Set" data structure.
	// Ref: https://programmingpraxis.com/2012/03/09/sparse-sets/

	// EASTL globs all the arrays into a single allocation, so it's best to 
	// reserve space up front to avoid a lot of memory-copies (even those are
	// relatively fast, though, because it's a linear/direct access pattern).

	SparseObjectArray sparse;
	eastl::tuple_vector<ObjectID, Ts...> compact;

public:

	ObjectPool() = default;
	ObjectPool(const ObjectPool&) = default;
	ObjectPool(ObjectPool&&) = default;
	ObjectPool& operator=(const ObjectPool&) = default;

	~ObjectPool() {
		for(auto it : compact)
			eastl::apply([](auto&... vs) { (TryFreeObjectComponent(vs), ...); }, it);	
	}

	bool IsEmpty() const { return compact.size() == 0; }
	int32 Count() const { return static_cast<int32>(compact.size()); }
	bool InRange(int32 idx) const { return idx >= 0 && idx < Count(); }
	bool Contains(ObjectID id) const { return IndexOf(id) != INVALID_INDEX; }

	auto begin() const { return compact.begin(); }
	auto end() const { return compact.end(); }
	auto rbegin() const { return compact.rbegin(); }
	auto rend() const { return compact.rend(); }
	auto operator[](int32 idx) const { return compact[idx]; }

	template<int C> auto GetComponentData() { return compact.get<C>(); }
	template<int C> auto GetComponentData() const { return compact.get<C>(); }
	
	template<int C> auto TryGetComponent(ObjectID id) { let idx = IndexOf(id); return idx != INVALID_INDEX ? GetComponentByIndex<C>(idx) : nullptr; }
	template<int C> auto TryGetComponent(ObjectID id) const { let idx = IndexOf(id); return idx != INVALID_INDEX ? GetComponentByIndex<C>(idx) : nullptr; }
	template<int C> auto& DoGetComponent(ObjectID id) { return *GetComponentByIndex<C>(IndexOf(id)); }
	template<int C> auto& DoGetComponent(ObjectID id) const { return *GetComponentByIndex<C>(IndexOf(id)); }
	template<int C> auto GetComponentByIndex(int32 idx) { DEBUG_ASSERT(InRange(idx)); return compact.get<C>() + idx; }
	template<int C> auto GetComponentByIndex(int32 idx) const { DEBUG_ASSERT(InRange(idx)); return compact.get<C>() + idx; }

	void ReserveCompact(int32 count) { compact.reserve(count); }
	void ShrinkCompact() { compact.shrink_to_fit(); }

	int32 IndexOf(ObjectID id) const {
		if (!id.IsNil() && sparse.HasPageFor(id)) {
			let idx = sparse.DoGetIndex(id);
			if (idx < Count() && compact.get<0>()[idx] == id)
				return idx;
		}

		return INVALID_INDEX;
	}
	
	bool TryAppendObject(ObjectID id, const Ts&... args) {
		if (id.IsNil() || Contains(id))
			return false;

		sparse.IncrementCount(id);
		sparse.DoSetIndex(id, Count());
		compact.push_back(id, args...); //compact.emplace_back(std::forward<ObjectID>(id), std::forward<Ts>(args)...);
		return true;
	}
	
	bool TryInsertObjectAt(ObjectID id, int32 index, const Ts&... args) {

		// fast path?
		if (index >= Count())
			return TryAppendObject(id, args...); // std::forward<Ts>(args)...);

		// early out?
		if (index < 0 || id.IsNil() || Contains(id))
			return false;

		sparse.IncrementCount(id);
		sparse.DoSetIndex(id, index);

		// insert + upshift
		compact.insert(compact.begin() + index, id, args...); //compact.insert(compact.begin() + index, std::forward<ObjectID>(id), std::forward<Ts>(args)...);
		let pHandles = compact.get<0>();
		let n = Count();
		for(int it=index + 1; it<n; ++it)
			sparse.DoSetIndex(pHandles[it], it);

		return true;
	}

	bool TryInsertObjectBefore(ObjectID id, ObjectID other, const Ts&... args) {
		let idx = IndexOf(other);
		return idx != INVALID_INDEX && TryInsertObjectAt(id, idx, args...); // std::forward<Ts>(args)...);
	}
	
	bool TryInsertObjectAfter(ObjectID id, ObjectID other, const Ts&... args) {
		let idx = IndexOf(other);
		return idx != INVALID_INDEX && TryInsertObjectAt(id, idx + 1, args...); // std::forward<Ts>(args)...);
	}

	void DoReleaseCompactRange_Shift(int32 idxStart, int32 idxEnd) {
		
		// release pages
		let itStart = compact.begin() + idxStart;
		let itEnd = compact.begin() + idxEnd;
		{
			let pHandles = compact.get<0>();
			for (auto idx = idxStart; idx != idxEnd; ++idx)
				sparse.DecrementCount(pHandles[idx]);
		}

		// erase compact range
		for(auto it=itStart; it!=itEnd; ++it)
			eastl::apply([](auto&... vs) { (TryFreeObjectComponent(vs), ...); }, *it);

		compact.erase(itStart, itEnd);

		// update downshifted indices
		{
			let n = Count();
			let pHandles = compact.get<0>();
			for (int32 it = idxStart; it < n; ++it)
				sparse.DoSetIndex(pHandles[it], it);
		}
	}

	bool TryReleaseObject_Swap(ObjectID id) {
		
		// early out?
		if (!sparse.HasPageFor(id))
			return false;
		let idx = sparse.DoGetIndex(id);
		let count = Count();
		let pHandles = compact.get<0>();
		if (idx >= count || pHandles[idx] != id)
			return false;
		
		// free all ObjectComponent pointers
		eastl::apply([](auto&... vs) { (TryFreeObjectComponent(vs), ...); }, compact[idx]);

		// swap with end?
		let lastIdx = count - 1;
		if (idx < lastIdx) {
			let tail = pHandles[lastIdx];
			compact[idx] = eastl::move(compact.back());
			sparse.DoSetIndex(tail, idx);
		}

		// pop the end
		compact.pop_back();

		sparse.DecrementCount(id);
		return true;
	
	}
	
	bool TryReleaseObject_Shift(ObjectID id) {
		// early out?
		if (!sparse.HasPageFor(id))
			return false;
		let idx = sparse.DoGetIndex(id);
		let oldCount = Count();
		let pHandles = compact.get<0>();
		if (idx >= oldCount || pHandles[idx] != id)
			return false;

		// free all ObjectComponent pointers
		eastl::apply([](auto&... vs) { (TryFreeObjectComponent(vs), ...); }, compact[idx]);

		// erase + downshift
		compact.erase(compact.begin() + idx);
		let newCount = Count();
		for (int it = idx; it < newCount; ++it) {
			sparse.DoSetIndex(pHandles[it], it);
		}

		sparse.DecrementCount(id);
		return true;
	}

	void Clear() {

		// free all ObjectComponent pointers
		for(auto it : compact)
			eastl::apply([](auto&... vs) { (TryFreeObjectComponent(vs), ...); }, it);

		sparse.Clear();
		compact.clear();
	}

};

//------------------------------------------------------------------------------------------
// Responsible for generating unique handles for other object pools, and maintaining
// the global (unordered) components (e.g. Name, RefCount, DebugHooks, etc)

template<typename... Ts>
class ObjectMgr {
private:
	ObjectPool<Ts...> pool;
	eastl::vector<ObjectID> recycledIDs;
	ObjectID nextID;

public:

	ObjectMgr(bool fingerprint = false) noexcept
		: nextID(ObjectID(0).WithFingerprint(fingerprint)) 
	{}


	ObjectID CreateObject(const Ts&... args) {
		ObjectID result (ForceInit::Default);

		// Prefer to recycle IDs, to reduce the amount of sparse-page-fragmentation.
		if (recycledIDs.size() > 0) {
			result = recycledIDs.back().NextVersion();
			recycledIDs.pop_back();
		} else {
			result = nextID;
			nextID = nextID.NextIndex();
		}

		if (!pool.TryAppendObject(result, args...)) // std::forward<Ts>(args)...))
			result = OBJECT_NIL;

		return result;
	}

	const ObjectPool<Ts...>& GetPool() const { return pool; }

	template<int C> auto TryGetComponent(ObjectID id) { return pool.TryGetComponent<C>(id); }
	template<int C> auto GetComponentByIndex(int32 idx) { return pool.GetComponentByIndex<C>(idx); }
	template<int C> auto GetComponentData() { return pool.GetComponentData<C>(); }

	void ReserveCompact(int32 count) { 
		pool.ReserveCompact(count); 
		recycledIDs.reserve(count);
	}

	void ShrinkCompact() { 
		pool.ShrinkCompact(); 
		recycledIDs.shrink_to_fit();
	}

	bool ReleaseObject(ObjectID id) {
		if (!pool.TryReleaseObject_Swap(id))
			return false;

		recycledIDs.push_back(id);
		return true;
	}

};

// TODO: template the (const Ts&... args) methods receivers to take forwarded arguments
//       (requires bugfix to east, bug filed: https://github.com/electronicarts/EASTL/issues/369)
// TODO: Remove/Shift a whole-range