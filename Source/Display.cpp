#include "Display.h"
#include "DiligentCore/Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"

#define DEPTH_BUFFER_FORMAT TEX_FORMAT_D32_FLOAT

void GraphicsDebugMessageCallback(
	enum DEBUG_MESSAGE_SEVERITY Severity,
	const Char* Message,
	const Char* Function,
	const Char* File,
	int Line
) {
	using namespace std;
	cout << "[GRAPHICS] " << Message << endl;
}

Display::Display(const char* windowName) {

	SDL_Rect rect;
	SDL_GetDisplayBounds(0, &rect);

	// split the difference between my laptop and pc haha
	int w = 1920;
	int h = 1080;
	if (rect.w == 1920) {
		w = 1280;
		h = 720;
	}

	let windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
	pWindow = SDL_CreateWindow(windowName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, windowFlags);
	CHECK_ASSERT(pWindow != nullptr);
	HWND hwnd = GetActiveWindow(); // TODO: way to get this from pWindow?

	// Rendering is routed through the Diligent Graphics API, which performs
	// - Backend Abstraction
	// - Debug Validation

	// create device
	EngineD3D12CreateInfo EngineCI;
	EngineCI.DebugMessageCallback = &GraphicsDebugMessageCallback;
	#if _DEBUG
	EngineCI.EnableDebugLayer = true;
	#endif
	auto GetEngineFactoryD3D12 = LoadGraphicsEngineD3D12();
	auto* pFactoryD3D12 = GetEngineFactoryD3D12();
	pFactoryD3D12->CreateDeviceAndContextsD3D12(EngineCI, &pDevice, &pContext);
	pEngineFactory = pFactoryD3D12;


	// create swap chain
	SwapChainDesc SCDesc;
	pFactoryD3D12->CreateSwapChainD3D12(pDevice, pContext, SCDesc, FullScreenModeDesc{}, Win32NativeWindow{ hwnd }, &pSwapChain);
	pSwapChain->SetMaximumFrameLatency(1);

	// determine MSAA support
	let& ColorFmtInfo = pDevice->GetTextureFormatInfoExt(pSwapChain->GetDesc().ColorBufferFormat);
	let& DepthFmtInfo = pDevice->GetTextureFormatInfoExt(DEPTH_BUFFER_FORMAT);
	let SupportedSampleCounts = ColorFmtInfo.SampleCounts & DepthFmtInfo.SampleCounts;
	MSAA_Count = 
		SupportedSampleCounts & 0x04 ? 4 : 
		SupportedSampleCounts & 0x02 ? 2 : 
		1;
	CreateMSAARenderTarget();
}

Display::~Display() {
	if (pContext)
		pContext->Flush();
	SDL_DestroyWindow(pWindow);
}

ivec2 Display::GetScreenSize() const {
	ivec2 result;
	SDL_GetWindowSize(pWindow, &result.x, &result.y);
	return result;
}

void Display::HandleEvent(const SDL_Event& ev) {
	let resize = ev.type == SDL_WINDOWEVENT && (
		ev.window.event == SDL_WINDOWEVENT_RESIZED ||
		ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED
		);
	if (resize) {
		int Width, Height;
		SDL_GetWindowSize(pWindow, &Width, &Height);
		pSwapChain->Resize(Width, Height);
		CreateMSAARenderTarget();
	}
}

void Display::SetMultisamplingTargetAndClear() {
	ITextureView* pRTV = GetRenderTargetView();
	ITextureView* pDSV = GetDepthTargetView();
	pContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	pContext->ClearRenderTarget(pRTV, (float*) &clearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	pContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

}

void Display::ResolveMultisampling() {
	if (IsMultisampling()) {
		// Resolve multi-sampled render taget into the current swap chain back buffer.
		auto pCurrentBackBuffer = pSwapChain->GetCurrentBackBufferRTV()->GetTexture();

		ResolveTextureSubresourceAttribs ResolveAttribs;
		ResolveAttribs.SrcTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
		ResolveAttribs.DstTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
		pContext->ResolveTextureSubresource(pMSColorRTV->GetTexture(), pCurrentBackBuffer, ResolveAttribs);
		ITextureView* pRTV = pSwapChain->GetCurrentBackBufferRTV();
		ITextureView* pDSV = pSwapChain->GetDepthBufferDSV();
		pContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	}
}

void Display::Present() {

	bool vsync = true;
	pSwapChain->Present(vsync ? 1 : 0);

}

void Display::CreateMSAARenderTarget() {
	if (!IsMultisampling())
		return;

	const auto& SCDesc = pSwapChain->GetDesc();
	// Create window-size multi-sampled offscreen render target
	TextureDesc ColorDesc;
	ColorDesc.Name = "Multisampled render target";
	ColorDesc.Type = RESOURCE_DIM_TEX_2D;
	ColorDesc.BindFlags = BIND_RENDER_TARGET;
	ColorDesc.Width = SCDesc.Width;
	ColorDesc.Height = SCDesc.Height;
	ColorDesc.MipLevels = 1;
	ColorDesc.Format = SCDesc.ColorBufferFormat;
	bool NeedsSRGBConversion = pDevice->GetDeviceCaps().IsD3DDevice() && (ColorDesc.Format == TEX_FORMAT_RGBA8_UNORM_SRGB || ColorDesc.Format == TEX_FORMAT_BGRA8_UNORM_SRGB);
	if (NeedsSRGBConversion) {
		// Internally Direct3D swap chain images are not SRGB, and ResolveSubresource
		// requires source and destination formats to match exactly or be typeless.
		// So we will have to create a typeless texture and use SRGB render target view with it.
		ColorDesc.Format = ColorDesc.Format == TEX_FORMAT_RGBA8_UNORM_SRGB ? TEX_FORMAT_RGBA8_TYPELESS : TEX_FORMAT_BGRA8_TYPELESS;
	}

	// Set the desired number of samples
	ColorDesc.SampleCount = MSAA_Count;
	// Define optimal clear value
	ColorDesc.ClearValue.Format = SCDesc.ColorBufferFormat;
	ColorDesc.ClearValue.Color[0] = clearColor.r;
	ColorDesc.ClearValue.Color[1] = clearColor.g;
	ColorDesc.ClearValue.Color[2] = clearColor.b;
	ColorDesc.ClearValue.Color[3] = clearColor.a;
	RefCntAutoPtr<ITexture> pColor;
	pDevice->CreateTexture(ColorDesc, nullptr, &pColor);

	// Store the render target view
	pMSColorRTV.Release();
	if (NeedsSRGBConversion) {
		TextureViewDesc RTVDesc;
		RTVDesc.ViewType = TEXTURE_VIEW_RENDER_TARGET;
		RTVDesc.Format = SCDesc.ColorBufferFormat;
		pColor->CreateView(RTVDesc, &pMSColorRTV);
	} else {
		pMSColorRTV = pColor->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
	}


	// Create window-size multi-sampled depth buffer
	TextureDesc DepthDesc = ColorDesc;
	DepthDesc.Name = "Multisampled depth buffer";
	DepthDesc.Format = DEPTH_BUFFER_FORMAT;
	DepthDesc.BindFlags = BIND_DEPTH_STENCIL;
	// Define optimal clear value
	DepthDesc.ClearValue.Format = DepthDesc.Format;
	DepthDesc.ClearValue.DepthStencil.Depth = 1;
	DepthDesc.ClearValue.DepthStencil.Stencil = 0;

	RefCntAutoPtr<ITexture> pDepth;
	pDevice->CreateTexture(DepthDesc, nullptr, &pDepth);
	// Store the depth-stencil view
	pMSDepthDSV = pDepth->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);


}