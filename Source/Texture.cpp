#include "Texture.h"
#include "Graphics.h"
#include <stb_image.h>

Texture::Texture(ObjectID aID)
	: ObjectComponent(aID)
	, pData(0)
	, allocWidth(0)
	, allocHeight(0)
{}

Texture::~Texture() {
	if (pData)
		stbi_image_free(pData);
}

bool Texture::Alloc(int w, int h) {
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

	int srcChannelCount;
	pData = stbi_load(filename, &allocWidth, &allocHeight, &srcChannelCount, 4);
	return pData != nullptr;
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

	return IsLoaded();
}

bool Texture::TryUnload() {
	if (!IsLoaded())
		return false;
	pTexture.Release();
	return true;
}


