#pragma once
#include "Common.h"
#include <EASTL/string.h>

// TODO: Cooking/Caching
//       - source assets are compiled/"cooked" to a position-independent POD binary blob (asset-data)
//       - assets-data is cachable, keyed off the INI path it was loaded from
//       - asset-data can be loaded into runtime-handles (physx objects, graphics handles, etc)
//       - asset-data can be saved to an archive for package builds
//       - asset-types, e.g.: Texture, Shader, Material, Mesh, Character, Sublevel

//------------------------------------------------------------------------------------------
// Asset Data Blobs
//
// There are by-design compact and linear in memory, with position-independent offsets,
// so that they can be mmap()ed, memcpy()ed, cached to files, or sent over the wire.
//
// The endianness of the data needs to match the endianness of the host machine, so we
// include a DOM to double-check.

// a combination asset-type and version identifier, GUID instead?
typedef uint32 schema_t; 

// placeholder for a proper "schema hash"?
#define SCHEMA_UNDEFINED 0
#define SCHEMA_TEXTURE   1
#define SCHEMA_MATERIAL  2
#define SCHEMA_MESH      3

struct AssetDataHeader {
	uint32   ByteOrderMarker;
	uint32   ByteCount;
	schema_t Schema; 
};

AssetDataHeader* AllocAssetData(uint32 szInBytes, schema_t schema);

template<typename T>
inline T* AllocAssetData(uint32 szInBytes) {
	CHECK_ASSERT(szInBytes >= sizeof(T));
	return (T*) AllocAssetData(szInBytes, T::SCHEMA);
}

AssetDataHeader* LoadAssetData(const char* path, schema_t schema = 0);
AssetDataHeader* CopyAssetData(const AssetDataHeader* data);
bool TrySaveAssetData(const char* path, AssetDataHeader* data);
void FreeAssetData(AssetDataHeader* data);

//------------------------------------------------------------------------------------------
// Unique-Ptr-Like Reference to Asset Data Blob

class AssetDataRef {
private:
	AssetDataHeader* data;

public:

	AssetDataRef() noexcept : data(nullptr) {}
	AssetDataRef(AssetDataRef&& rval) noexcept : data(rval.data) { rval.data = nullptr; }
	AssetDataRef& operator=(AssetDataRef&& rval) noexcept {
		data = rval.data;
		rval.data = nullptr;
		return *this;
	}

	AssetDataRef(const AssetDataRef&) = delete;
	AssetDataRef& operator=(const AssetDataRef&) = delete;

	AssetDataRef(AssetDataHeader* aData) noexcept : data(aData)  {}

	~AssetDataRef();

	bool ValidBOM() const;
	schema_t Schema() const { return data ? data->Schema : SCHEMA_UNDEFINED; }

	operator bool() const { return data != nullptr; }

	template<typename T>
	T* Get() { 
		return data && data->Schema == T::SCHEMA ? (T*) data : nullptr;
	}

	template<typename T>
	const T* Get() const { 
		return data && data->Schema == T::SCHEMA ? (const T*)data : nullptr;
	}

	void SetData(AssetDataHeader* pData);
};

//------------------------------------------------------------------------------------------
// Helpers for reading/writing Asset Data Blobs

inline uint32 StrByteCount(const char* cstr) { return 1 + uint32(strlen(cstr)); }
inline uint32 StrByteCount(const eastl::string& str) { return 1 + uint32(str.size()); }

class AssetDataWriter {
private:
	union {
		AssetDataHeader* pHeader;
		uint8* pBytes;
	};
	uint32 offset;

public:

	AssetDataWriter(AssetDataHeader* aHeader, uint32 startingOffset = 0);

	void* GetData() const { return pHeader; }
	uint32 GetSize() const { return pHeader->ByteCount; }
	uint32 GetOffset() const { return offset; }
	bool CanWrite(uint32 size) const;

	void Seek(uint32 nbytes);

	template<typename T>
	T* Peek() const {
		CHECK_ASSERT(offset < GetSize());
		return (T*) (pBytes + offset);
	}
	
	template<typename T>
	T* PeekAndSeek() {
		let result = Peek<T>();
		Seek(sizeof(T));
		return result;
	}

	void Poke(const void* data, uint32 size);



	template<typename T>
	void PokeValue(const T& Value) { Poke(&Value, sizeof(T)); }

	void WriteString(const eastl::string& str);
	void WriteString(const char* cstr);
	void WriteData(const void* data, uint32 size);
	
	template<typename T>
	void WriteValue(const T& val) { WriteData(&val, sizeof(val)); }
	
};

class AssetDataReader {
private:
	union {
		const AssetDataHeader *pHeader;
		const uint8* pBytes;
	};
	uint32 offset;

public:

	AssetDataReader(const AssetDataHeader* aHeader, uint32 startingOffset = 0);

	const void* GetData() const { return pHeader; }
	const void* GetCurrent() const { return pBytes + offset; }
	uint32 GetSize() const { return pHeader->ByteCount; }
	bool CanRead(uint32 size) const;

	void Seek(uint32 nBytes);

	template<typename T>
	const T* Peek() const { 
		CHECK_ASSERT(offset < GetSize());
		return (T*) (pBytes + offset);
	}

	template<typename T>
	void ReadValue(T& OutValue) {
		OutValue = *Peek<T>();
		Seek(sizeof(T));
	}

	const char* ReadString();

};

template<typename T>
const T* Peek(const AssetDataHeader* data, uint32 offset) {
	return AssetDataReader(data, offset).Peek<T>();
}

template<typename T>
T* Peek(AssetDataHeader* data, uint32 offset) {
	union {
		AssetDataHeader* pHeader;
		uint8* pBytes;
	} alias;
	alias.pHeader = data;
	return (T*)(alias.pBytes + offset);
}
