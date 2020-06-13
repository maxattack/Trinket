// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "ObjectPool.h"

static_assert(sizeof(ObjectID) == 4);
static_assert(OBJ_MAX_PAGES * OBJ_INDICES_PER_PAGE == MAX_OBJECTS);

struct SparsePageFreelist {
	SparsePage *pNext = nullptr;

#if _DEBUG
	// don't need to free pages when the process leaves, but in debug
	// memory-leak detectors get cranku, so we do it there, at least
	~SparsePageFreelist() {
		while (let it = pNext) {
			pNext = it->pNext;
			free(it);
		}
	}
#endif
};

static SparsePageFreelist gSparsePageFreelist;

SparsePage* SparseObjectArray::AllocPage() {
	if (let result = gSparsePageFreelist.pNext) {
		gSparsePageFreelist.pNext = gSparsePageFreelist.pNext->pNext;
		memset(result, 0, sizeof(SparsePage));
		return (SparsePage*) result;
	}

	return (SparsePage*) calloc(1, sizeof(SparsePage));
}

void SparseObjectArray::ReleasePage(int32 pageIdx) {
	if (let page = pages[pageIdx]) {
		page->pNext = gSparsePageFreelist.pNext;
		gSparsePageFreelist.pNext = page;
		pages[pageIdx] = nullptr;
	}
}

//void TestAutofree() {
//	using namespace std;
//	class ObjTest : public ObjectComponent {
//	public:
//		ObjTest(ObjectID id) : ObjectComponent(id) { cout << "ADD" << endl; }
//		~ObjTest() { cout << "DELETE" << endl; }
//	};
//	{
//		ObjectPool<ObjTest*> pool;
//		let test = NewObjectComponent<ObjTest>(ObjectID(0x01));
//		pool.TryAppendObject(test->ID(), test);
//	}
//}

//void TestObjects() {
//	using namespace std;
//
//	struct alignas(32) AVec {
//		float x, y, z, w;
//		float a, b, c, d; 
//		AVec() noexcept {}
//	};
//
//	ObjectMgr<int, eastl::string, float, AVec> mgr;
//	ObjectPool<int> pool;
//
//	mgr.ReserveCompact(1024);
//	pool.ReserveCompact(1024);
//
//
//	let id1 = mgr.CreateObject(1, "one", 1.f, AVec());
//	let id2 = mgr.CreateObject(2, "two", 2.f, AVec());
//	let id3 = mgr.CreateObject(3, "three", 3.f, AVec());
//	mgr.ReleaseObject(id2);
//
//	pool.TryAppendObject(id1, 101);
//	pool.TryAppendObject(id3, 303);
//	//pool.Clear();
//
//	cout << "mgr count: " << mgr.Count() << endl;
//	cout << "pool count: " << pool.Count() << endl;
//
//	cout << "id3 name: " << mgr.GetComponent<2>(id3)->c_str() << endl;
//
//	cout << "ID1 = " << id1.pageIdx << endl;
//	for(auto it : mgr) {
//		cout << "Object ID = " << it.handle << endl;
//	}
//
//	for(auto it : pool) {
//		let code = eastl::get<1>(it);
//		cout << "Code = " << code << endl;
//	}
//
//
//}