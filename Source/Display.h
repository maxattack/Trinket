// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Common.h"
#include "Math.h"
#include <SDL.h>
#include <SDL_syswm.h>

// Diligent Engine Config
#ifndef PLATFORM_WIN32
#define PLATFORM_WIN32 1
#endif
#ifndef ENGINE_DLL
#define ENGINE_DLL 1
#endif
#ifndef D3D12_SUPPORTED
#define D3D12_SUPPORTED 1
#endif

#include "DiligentCore/Common/interface/BasicMath.hpp"
#include "DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/Shader.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/SwapChain.h"

using namespace Diligent;
class Graphics;
class Texture;
class Material;
class Mesh;

class Display {
public:

	Display(const char* windowName);
	~Display();

	SDL_Window* GetWindow() { return pWindow; }

	IRenderDevice* GetDevice() { return pDevice; }
	IEngineFactory* GetEngineFactory() { return pEngineFactory; }
	IDeviceContext* GetContext() { return pContext; }
	ISwapChain* GetSwapChain() { return pSwapChain; }

	float GetAspect() const { let& SCD = pSwapChain->GetDesc(); return float(SCD.Width) / float(SCD.Height); }
	
	bool IsMultisampling() const { return MSAA_Count > 1; }
	int GetMultisampleCount() const { return MSAA_Count; }

	ITextureView* GetRenderTargetView() { return IsMultisampling() ? pMSColorRTV : pSwapChain->GetCurrentBackBufferRTV(); }
	ITextureView* GetDepthTargetView() { return IsMultisampling() ? pMSDepthDSV : pSwapChain->GetDepthBufferDSV(); }

	void HandleEvent(const SDL_Event& aEvent);

	void SetMultisamplingTargetAndClear();
	void ResolveMultisampling();
	void Present();

private:

	void CreateMSAARenderTarget();

	SDL_Window* pWindow;
	RefCntAutoPtr<IRenderDevice>  pDevice;
	RefCntAutoPtr<IDeviceContext> pContext;
	RefCntAutoPtr<IEngineFactory> pEngineFactory;
	RefCntAutoPtr<ISwapChain>     pSwapChain;

	vec4 clearColor = vec4(0.1f, 0.1f, 0.2f, 0.0f);

	// Should this belong in graphics?
	uint32 MSAA_Count = 4;
	RefCntAutoPtr<ITextureView> pMSColorRTV;
	RefCntAutoPtr<ITextureView> pMSDepthDSV;

};