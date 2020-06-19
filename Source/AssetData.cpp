#include "AssetData.h"
#include <cstdio>
#include <cstring>

#define ASSET_BOM (0xC0FFEE42)

void* AllocAssetData(uint32_t szInBytes) {
	CHECK_ASSERT(szInBytes >= sizeof(AssetDataHeader));
	AssetDataHeader* Result = (AssetDataHeader*)malloc(szInBytes);
	Result->ByteOrderMarker = ASSET_BOM;
	Result->ByteCount = szInBytes;
	return Result;
}

void* LoadAssetData(const char* path, uint32_t szMin) {
	FILE* pFile;
	let error = fopen_s(&pFile, path, "rb");
	if (error) {
		return nullptr;
	}

	if (pFile == nullptr)
		return nullptr;

	AssetDataHeader header;
	if (fread(&header, sizeof(AssetDataHeader), 1, pFile) != sizeof(AssetDataHeader)) {
		fclose(pFile);
		return nullptr;
	}
	if (header.ByteOrderMarker != ASSET_BOM) {
		fclose(pFile);
		return nullptr;
	}

	if (header.ByteCount < szMin) {
		fclose(pFile);
		return nullptr;
	}

	let result = (uint8*)AllocAssetData(header.ByteCount);
	let bytesRemaining = header.ByteCount - sizeof(AssetDataHeader);
	let bytesRead = bytesRemaining > 0 ? fread(result + sizeof(AssetDataHeader), 1, bytesRemaining, pFile) : 0;
	if (bytesRemaining != bytesRead) {
		fclose(pFile);
		FreeAssetData(result);
		return nullptr;
	}

	fclose(pFile);
	return result;
}

void* CopyAssetData(void* data) {
	if (!data)
		return nullptr;

	let sz = static_cast<AssetDataHeader*>(data)->ByteCount;
	let result = malloc(sz);
	memcpy(result, data, sz);
	return result;
}

bool TrySaveAssetData(const char* path, void* data) {
	FILE* pFile;
	let error = fopen_s(&pFile, path, "wb");
	if (error)
		return false;
	
	let header = static_cast<AssetDataHeader*>(data);
	fwrite(data, 1, header->ByteCount, pFile);

	return true;

}

void FreeAssetData(void* data) {
	free(data);
}
