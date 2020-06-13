// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Common.h"

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
