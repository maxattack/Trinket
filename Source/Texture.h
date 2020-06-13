#pragma once
#include "Diligent.h"
#include "Object.h"

class Texture : public ObjectComponent {
private:
	RefCntAutoPtr<ITexture> pTexture;

public:
	Texture(ObjectID aID);

	bool IsLoaded() const { return pTexture; }

	ITextureView* GetSRV() { return pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE); }


	bool TryLoad(Graphics* pGraphics, const char* filename);
	bool TryUnload();

};