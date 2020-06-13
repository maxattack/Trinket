#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <type_traits>
#include <SDL.h>
#include <SDL_syswm.h>

#define let const auto

#define GLM_FORCE_LEFT_HANDED 1
#define GLM_FORCE_INTRINSICS 1
#include <glm/glm.hpp>

typedef glm::int8    int8;
typedef glm::int16   int16;
typedef glm::int32   int32;
typedef glm::int64   int64;
typedef glm::uint8   uint8;
typedef glm::uint16  uint16;
typedef glm::uint32  uint32;
typedef glm::uint64  uint64;
typedef glm::float32 float32;
typedef glm::float64 float64;
typedef glm::vec2    vec2;
typedef glm::vec3    vec3;
typedef glm::vec4    vec4;
typedef glm::mat3    mat3;
typedef glm::mat4    mat4;
typedef glm::quat    quat;

#define INVALID_INDEX (-1)
#define TRINKET_EDITOR 1
#define TRINKET_TEST 1

enum class ForceInit { Default };

#if _DEBUG
#define DEBUG_ASSERT(x) assert(x)
#else
#define DEBUG_ASSERT(x)
#endif

