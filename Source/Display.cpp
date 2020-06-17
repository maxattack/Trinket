#include "Display.h"
#include "DiligentCore/Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"


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
	let windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
	pWindow = SDL_CreateWindow(windowName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, windowFlags);
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

}

Display::~Display() {
	if (pContext)
		pContext->Flush();
	SDL_DestroyWindow(pWindow);
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
	}
}

