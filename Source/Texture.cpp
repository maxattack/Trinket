#include "Texture.h"
#include "Graphics.h"
#include "DiligentTools/TextureLoader/interface/TextureLoader.h"
#include "DiligentTools/TextureLoader/interface/TextureUtilities.h"

Texture::Texture(ObjectID aID)
	: ObjectComponent(aID)
{}

bool Texture::TryLoad(Graphics* pGraphics, const char* filename) {
	
	if (pTexture)
		return false;

	TextureLoadInfo loadInfo;
	loadInfo.IsSRGB = true;
	
	CreateTextureFromFile(filename, loadInfo, pGraphics->GetDevice(), &pTexture);
	pTextureView = pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
	return IsLoaded();
}

bool Texture::TryUnload() {
	if (!IsLoaded())
		return false;
	pTexture.Release();
	return true;
}


