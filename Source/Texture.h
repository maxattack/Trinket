// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "AssetData.h"
#include "Display.h"
#include "ObjectPool.h"
#include "Math.h"
#include "Name.h"

// TODO:
// New Data Fields: Compression, Clamp/Wrap, Filters, MipMaps, Channels
// Separate CPU Image-Data from GPU Texture-Handle?

struct TextureAssetData : AssetDataHeader {
	static const schema_t SCHEMA = SCHEMA_TEXTURE;

	uint16 TextureWidth;
	uint16 TextureHeight;

	uint32 DataStride() const { return (TextureWidth << 2); }
	uint32 DataSize() const { return (TextureWidth << 2) * TextureHeight; }
	const uint8* Data() const { return Peek<uint8>(this, sizeof(TextureAssetData)); }
};

TextureAssetData* ImportTextureAssetDataFromSource(const char* configPath);
RefCntAutoPtr<ITexture> LoadTextureHandleFromAsset(Display* pDisplay, const TextureAssetData* pData);

class World;

class TextureRegistry {
public:

	TextureRegistry(World* aWorld);
	~TextureRegistry();

	bool HasTexture(ObjectID id) { return textures.Contains(id); }
	ITexture* LoadTexture(ObjectID id, const TextureAssetData* pData);
	ITexture* GetTexture(ObjectID id) { let pRef = textures.TryGetComponent<1>(id); return pRef ? *pRef : nullptr; }
	ITexture* FindTexture(Name path);

private:
	World* pWorld;
	ObjectPool<RefCntAutoPtr<ITexture>> textures;

};