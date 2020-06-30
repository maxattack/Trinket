// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once

// Include IMGUI editor?
#ifndef TRINKET_EDITOR
#	define TRINKET_EDITOR 1
#endif

// Include QA/Test features (e.g. wireframe debug-draw)
#ifndef TRINKET_TEST
#	define TRINKET_TEST 1
#endif

// Include Bounds/Reference Checkes?
#ifndef TRINKET_CHECKED
#	if _DEBUG
#		define TRINKET_CHECKED 1
#	else
#		define TIRNKET_CHECKED 0
#	endif
#endif


#define let const auto
#define INVALID_INDEX (-1)
enum class ForceInit { Default };

typedef unsigned int uint;

#define GLM_FORCE_LEFT_HANDED 1
#define GLM_FORCE_INTRINSICS 1
#include <glm/fwd.hpp>

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
typedef glm::ivec2   ivec2;
typedef glm::uvec2   uvec2;

#if TRINKET_CHECKED
#	include <cassert>
#	define CHECK_ASSERT(x) assert(x)
#else
#	define CHECK_ASSERT(x)
#endif

