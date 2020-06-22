// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Math.h"

#define MAX_EDGE_LOOP_LENGTH 50
#define TRIANGULATE_RESULT_CAPACITY ((50-2) * 3)

inline int GetTriangleCount(int loopLength) { return loopLength - 2; }

// True = Input was Convex
// False - Input was Concave
bool Triangulate(uint32* outIndex, const vec3* points, int npoints);
