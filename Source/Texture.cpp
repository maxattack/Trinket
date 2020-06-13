#include "Texture.h"
#include "Graphics.h"
#include <stb_image.h>

Texture::Texture(ObjectID aID)
	: ObjectComponent(aID)
{}

bool Texture::TryLoad(Graphics* pGraphics, const char* filename) {
	
	if (pTexture)
		return false;

	int w, h, chan;
	let pData = stbi_load(filename, &w, &h, &chan, 4);
	if (!pData)
		return false;

	


	TextureDesc desc;
	desc.Type = RESOURCE_DIMENSION::RESOURCE_DIM_TEX_2D;
	desc.Width = w;
	desc.Height = h;
	desc.Format = TEXTURE_FORMAT::TEX_FORMAT_RGBA8_UNORM_SRGB;
	desc.Usage = USAGE::USAGE_STATIC;
	desc.BindFlags = BIND_FLAGS::BIND_SHADER_RESOURCE;

	TextureSubResData texSubResData;
	texSubResData.pData = pData;
	texSubResData.Stride = 4 * w;


	TextureData texData;
	texData.NumSubresources = 1;
	texData.pSubResources = &texSubResData;

	pGraphics->GetDevice()->CreateTexture(desc, &texData, &pTexture);

	stbi_image_free(pData);

	return IsLoaded();
}

bool Texture::TryUnload() {
	if (!IsLoaded())
		return false;
	pTexture.Release();
	return true;
}


