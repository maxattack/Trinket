// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Math.h"

struct fit_plane {
	vec3 center;
	vec3 normal;
};

vec3 FindCenter(const vec3* points, int npoints);
fit_plane FindFitPlane(const vec3* points, int npoints);
fit_plane FindFitPlane(const vec3* points, int npoints, const vec3& guess);
fit_plane FindFitPlane(const vec3* points, int npoints, const vec3& center, const vec3& guess);
