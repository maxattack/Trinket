#pragma once
#include "Common.h"

// TODO: Cooking/Caching
//       - source assets are compiled/"cooked" to a position-independent POD binary blob (asset-data)
//       - assets-data is cachable, keyed off the INI path it was loaded from
//       - asset-data can be loaded into runtime-handles (physx objects, graphics handles, etc)
//       - asset-data can be saved to an archive for package builds
//       - asset-types, e.g.: Texture, Shader, Material, Mesh, Character, Sublevel

struct AssetDataHeader {
	uint32_t ByteOrderMarker;
	uint32_t ByteCount;
	// SchemaHash?
};

void* AllocAssetData(uint32_t szInBytes);               // Allocate asset data with an appropriate header
void* LoadAssetData(const char* path, uint32_t szMin);  // Load asset data from a file
void* CopyAssetData(void* data);                        // Copy asset data
bool TrySaveAssetData(const char* path, void* data);       // save out data to disc
void FreeAssetData(void* data);                         // Free asset data either loaded or allocated above

template<typename T, typename HeaderT>
T* GetDataAt(HeaderT* data, uint32_t offset) {
	return (T*)(((uint8*)data) + offset);
}

class AssetDataRef {
private:
	void* data;

public:

	AssetDataRef() noexcept : data(nullptr) {}
	AssetDataRef(AssetDataRef&& rval) noexcept : data(rval.data) { rval.data = nullptr; }

	AssetDataRef(const AssetDataRef&) = delete;
	AssetDataRef& operator=(const AssetDataRef&) = delete;

	template<typename HeaderT>
	AssetDataRef(HeaderT* aData) noexcept : data(aData)  {}

	~AssetDataRef() {
		if (data)
			FreeAssetData(data);
	}

	operator bool() const { return data != nullptr; }

	template<typename T>
	T* Get() { return (T*) data; }

	template<typename T>
	const T* Get() const { return (const T*) data; }
};
