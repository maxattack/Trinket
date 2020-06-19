// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Texture.h"
#include "Graphics.h"
#include <stb_image.h>
#include <ini.h>

TextureDataHeader* LoadTextureAssetDataFromConfig(const char* configPath) {

	struct TextureConfig {
		eastl::string path;
	};

	let handler = [](void* user, const char* section, const char* name, const char* value) {
		auto pConfig = (TextureConfig*) user;
		
		#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

		if (MATCH("Texture", "path"))
			pConfig->path = value;
		
		#undef MATCH

		return 1;
	};

	TextureConfig config;
	if (ini_parse(configPath, handler, &config))
		return nullptr;

	config.path = "Assets/" + config.path;
	
	int w, h, chan;
	let pData = stbi_load(config.path.c_str(), &w, &h, &chan, 4);
	if (pData == nullptr)
		return nullptr;

	let bytesPerPixel = 4;
	let dataSize = bytesPerPixel * w * h;
	let result = (TextureDataHeader*) AllocAssetData(sizeof(TextureDataHeader) + dataSize);

	result->TextureWidth = (uint32) w;
	result->TextureHeight = (uint32) h;
	memcpy(result->TextureData(), pData, dataSize);

	stbi_image_free(pData);

	return result;
}





Texture::Texture(ObjectID aID)
	: ObjectComponent(aID)
{}

Texture::~Texture() {
	if (pData)
		stbi_image_free(pData);
}

bool Texture::Alloc(uint32 w, uint32 h) {
	let earlyOut = pData != nullptr || w * h == 0;
	if (earlyOut)
		return false;

	// TODO: make sure we're using the same allocator as STB
	// TODO: make sure w/h are PoT?


	pData = malloc(size_t(w) * size_t(h) * 4);
	if (!pData)
		return false; 

	allocWidth = w;
	allocHeight = h;
	return true;
}

bool Texture::AllocFile(const char* filename) {
	if (pData)
		return false;

	int w, h, chan;
	pData = stbi_load(filename, &w, &h, &chan, 4);
	if (pData == nullptr)
		return false;

	allocWidth = w;
	allocHeight = h;
	return true;
}

bool Texture::Dealloc() {
	if (!pData) 
		return false;

	stbi_image_free(pData);
	allocWidth = 0;
	allocHeight = 0;
	pData = nullptr;
	return true;
}

bool Texture::TryLoad(Graphics* pGraphics) {

	let earlyOut = IsLoaded() || pData == nullptr || allocWidth * allocHeight == 0;
	if (earlyOut)
		return false;

	TextureDesc desc;
	desc.Type = RESOURCE_DIMENSION::RESOURCE_DIM_TEX_2D;
	desc.Width = allocWidth;
	desc.Height = allocHeight;
	desc.Format = TEXTURE_FORMAT::TEX_FORMAT_RGBA8_UNORM_SRGB;
	desc.Usage = USAGE::USAGE_STATIC;
	desc.BindFlags = BIND_FLAGS::BIND_SHADER_RESOURCE;

	TextureSubResData texSubResData;
	texSubResData.pData = pData;
	texSubResData.Stride = 4 * allocWidth;


	TextureData texData;
	texData.NumSubresources = 1;
	texData.pSubResources = &texSubResData;

	pGraphics->GetDevice()->CreateTexture(desc, &texData, &pTexture);
	if (!IsLoaded())
		return false;

	loadWidth = allocWidth;
	loadHeight = allocHeight;
	return true;
}

bool Texture::TryUnload() {
	if (!IsLoaded())
		return false;
	pTexture.Release();
	loadWidth = 0;
	loadHeight = 0;
	return true;
}


