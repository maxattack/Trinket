// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Display.h"
#include "Object.h"
#include "Math.h"

class Texture : public ObjectComponent {
private:


	RefCntAutoPtr<ITexture> pTexture;

	void* pData = nullptr;
	uint32 allocWidth = 0;
	uint32 allocHeight = 0;
	uint32 loadWidth = 0;
	uint32 loadHeight = 0;

public:
	Texture(ObjectID aID);
	~Texture();

	bool IsLoaded() const { return pTexture; }
	ITextureView* GetSRV() { return pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE); }

	uint32 AllocWidth() const { return allocWidth; }
	uint32 AllocHeight() const { return allocHeight; }
	uvec2 AllocSize() const { return uvec2(allocWidth, allocHeight); }

	uint32 LoadWidth() const { return loadWidth; }
	uint32 LoadHeight() const { return loadHeight; }
	uvec2 LoadSize() const { return uvec2(loadWidth, loadHeight); }


	bool Alloc(uint32 w, uint32 h);
	bool AllocFile(const char* filename);
	bool Dealloc();

	bool TryLoad(Graphics* pGraphics);
	bool TryUnload();


};
