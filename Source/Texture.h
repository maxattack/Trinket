#pragma once
#include "GraphicsPlatform.h"
#include "Object.h"

class Texture : public ObjectComponent {
private:


	RefCntAutoPtr<ITexture> pTexture;

	void* pData;
	int allocWidth, allocHeight;

public:
	Texture(ObjectID aID);
	~Texture();

	bool IsLoaded() const { return pTexture; }

	ITextureView* GetSRV() { return pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE); }

	bool Alloc(int w, int h);
	bool AllocFile(const char* filename);
	bool Dealloc();

	bool TryLoad(Graphics* pGraphics);
	bool TryUnload();


};
