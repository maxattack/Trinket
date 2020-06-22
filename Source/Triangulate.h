// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Math.h"

inline int GetTriangleCount(int loopLength) { return loopLength - 2; }
bool Triangulate(uint32* outIndex, const vec3* points, int npoints);
