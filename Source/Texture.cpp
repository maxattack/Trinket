// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Texture.h"
#include "Graphics.h"
#include <stb_image.h>
#include <ini.h>

TextureAssetData* ImportTextureAssetDataFromConfig(const char* configPath) {
	using namespace eastl::literals::string_literals;

	struct TextureConfig {
		bool hasTextureSection = false;
		eastl::string path;
	};

	let handler = [](void* user, const char* section, const char* name, const char* value) {
		auto pConfig = (TextureConfig*) user;
		
		#define SECTION(s) strcmp(section, s) == 0
		#define MATCH(n) strcmp(name, n) == 0

		if (SECTION("Texture"))
		{
			pConfig->hasTextureSection = true;
			if (MATCH("path"))
				pConfig->path = "Assets/"s + value;
		}
		
		#undef SECTION		
		#undef MATCH

		return 1;
	};

	TextureConfig config;
	eastl::string iniPath = "Assets/"s + configPath;
	if (ini_parse(iniPath.c_str(), handler, &config))
		return nullptr;

	if (!config.hasTextureSection)
		return nullptr;
	
	int w, h, chan;
	let pData = stbi_load(config.path.c_str(), &w, &h, &chan, 4);
	if (pData == nullptr)
		return nullptr;

	let bytesPerPixel = 4;
	let dataSize = bytesPerPixel * w * h;
	let result = AllocAssetData<TextureAssetData>(sizeof(TextureAssetData) + dataSize);
	result->TextureWidth = (uint16) w;
	result->TextureHeight = (uint16) h;

	AssetDataWriter writer(result, sizeof(TextureAssetData));
	writer.WriteData(pData, dataSize);

	stbi_image_free(pData);

	return result;
}

RefCntAutoPtr<ITexture> LoadTextureHandleFromAsset(Display* pDisplay, const TextureAssetData* pData) {
	RefCntAutoPtr<ITexture> pResult;
	if (pData == nullptr)
		return pResult;

	TextureDesc desc;
	desc.Type = RESOURCE_DIMENSION::RESOURCE_DIM_TEX_2D;
	desc.Width = pData->TextureWidth;
	desc.Height = pData->TextureHeight;
	desc.Format = TEXTURE_FORMAT::TEX_FORMAT_RGBA8_UNORM_SRGB;
	desc.Usage = USAGE::USAGE_STATIC;
	desc.BindFlags = BIND_FLAGS::BIND_SHADER_RESOURCE;

	TextureSubResData texSubResData;
	texSubResData.pData = pData->Data();
	texSubResData.Stride = pData->DataStride();

	TextureData texData;
	texData.NumSubresources = 1;
	texData.pSubResources = &texSubResData;

	pDisplay->GetDevice()->CreateTexture(desc, &texData, &pResult);

	return pResult;
}
