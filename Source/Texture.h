#pragma once
#include "Diligent.h"
#include "Object.h"

class Texture : public ObjectComponent {
private:
	RefCntAutoPtr<ITexture> pTexture;
	RefCntAutoPtr<ITextureView> pTextureView;

public:
	Texture(ObjectID aID);

	bool IsLoaded() const { return pTexture; }

	ITextureView* GetSRV() { return pTextureView; }


	bool TryLoad(Graphics* pGraphics, const char* filename);
	bool TryUnload();

};