#include "AssetData.h"
#include <cstdio>
#include <cstring>

#define ASSET_BOM (0xC0FFEE42)

AssetDataHeader* AllocAssetData(uint32 szInBytes, schema_t schema) {
	CHECK_ASSERT(szInBytes >= sizeof(AssetDataHeader));
	AssetDataHeader* Result = (AssetDataHeader*)malloc(szInBytes);
	Result->ByteOrderMarker = ASSET_BOM;
	Result->ByteCount = szInBytes;
	Result->Schema = schema;
	return Result;
}

AssetDataHeader* LoadAssetData(const char* path, schema_t schema) {
	FILE* pFile;
	let error = fopen_s(&pFile, path, "rb");
	if (error) {
		return nullptr;
	}

	if (pFile == nullptr)
		return nullptr;

	AssetDataHeader header;

	let success = 
		fread(&header, sizeof(AssetDataHeader), 1, pFile) == sizeof(AssetDataHeader) && 
		header.ByteOrderMarker == ASSET_BOM &&
		header.ByteCount >= sizeof(AssetDataHeader) &&
		(schema == SCHEMA_UNDEFINED || header.Schema == schema);
	if (!success) {
		fclose(pFile);
		return nullptr;
	}

	let result = AllocAssetData(header.ByteCount, schema);
	let bytesRemaining = header.ByteCount - sizeof(AssetDataHeader);
	let bytesRead = bytesRemaining > 0 ? fread(result + 1, 1, bytesRemaining, pFile) : 0;
	if (bytesRemaining != bytesRead) {
		fclose(pFile);
		FreeAssetData(result);
		return nullptr;
	}

	fclose(pFile);
	return result;
}

AssetDataHeader* CopyAssetData(const AssetDataHeader* data) {
	if (!data)
		return nullptr;

	let sz = data->ByteCount;
	let result = (AssetDataHeader*) malloc(sz);
	memcpy(result, data, sz);
	return result;
}

bool TrySaveAssetData(const char* path, AssetDataHeader* data) {
	FILE* pFile;
	let error = fopen_s(&pFile, path, "wb");
	if (error)
		return false;
	
	fwrite(data, 1, data->ByteCount, pFile);
	return true;
}

void FreeAssetData(AssetDataHeader* data) {
	free(data);
}

AssetDataRef::~AssetDataRef() { 
	if (data)
		FreeAssetData(data);
}

bool AssetDataRef::ValidBOM() const {
	return data && data->ByteOrderMarker == ASSET_BOM;
}

void AssetDataRef::SetData(AssetDataHeader* pData) {
	if (data)
		FreeAssetData(data);
	data = pData;
}

AssetDataWriter::AssetDataWriter(AssetDataHeader* aHeader, uint32 startingOffset)
	: pHeader(aHeader)
	, offset(startingOffset)
{
	CHECK_ASSERT(pHeader != nullptr);
	CHECK_ASSERT(pHeader->ByteOrderMarker == ASSET_BOM);
	CHECK_ASSERT(offset <= pHeader->ByteCount);
}

bool AssetDataWriter::CanWrite(uint32 size) const { 
	return offset + size <= pHeader->ByteCount; 
}

void AssetDataWriter::Seek(uint32 nbytes) {
	CHECK_ASSERT(offset + nbytes <= pHeader->ByteCount);
	offset += nbytes;
}

void AssetDataWriter::Poke(const void* data, uint32 size) {
	CHECK_ASSERT(offset + size <= pHeader->ByteCount);
	memcpy(pBytes + offset, data, size);
}

void AssetDataWriter::WriteString(const char* cstr) {
	WriteData(cstr, StrByteCount(cstr));
}

void AssetDataWriter::WriteString(const eastl::string& str) {
	WriteData(str.c_str(), StrByteCount(str));
}

void AssetDataWriter::WriteData(const void* data, uint32 size) {
	Poke(data, size);
	Seek(size);
}

AssetDataReader::AssetDataReader(const AssetDataHeader* aHeader, uint32 startingOffset)
	: pHeader(aHeader)
	, offset(startingOffset)
{
	CHECK_ASSERT(pHeader != nullptr);
	CHECK_ASSERT(pHeader->ByteOrderMarker == ASSET_BOM);
	CHECK_ASSERT(offset <= pHeader->ByteCount);
}

void AssetDataReader::Seek(uint32 nBytes) {
	CHECK_ASSERT(offset + nBytes <= pHeader->ByteCount);
	offset += nBytes;
}

bool AssetDataReader::CanRead(uint32 size) const {
	return offset + size <= pHeader->ByteCount;
}

const char* AssetDataReader::ReadString() {
	CHECK_ASSERT(offset < GetSize());
	let result = Peek<char>();
	Seek(StrByteCount(result));
	return result;
}
