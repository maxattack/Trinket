// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Math.h"

struct fit_plane {
	vec3 center;
	vec3 normal;
};

fit_plane FindFitPlane(vec3* pPoints, int npoints, vec3 guess);
