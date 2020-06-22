// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "FitPlane.h"

static float Max3(float x, float y, float z) { return glm::max(x, glm::max(y, z)); }
static float Max4(float x, float y, float z, float w) { return glm::max(glm::max(x, y), glm::max(z, w)); }
static float MaxEntry(const vec3& vec) { return Max3(vec.x, vec.y, vec.z); }
static float MaxEntry(const vec4& vec) { return Max4(vec.x, vec.y, vec.z, vec.w); }
static float MaxEntry(const mat3& mat) { return Max3(MaxEntry(mat[0]), MaxEntry(mat[1]), MaxEntry(mat[2])); }
static float MaxEntry(const mat4& mat) { return Max4(MaxEntry(mat[0]), MaxEntry(mat[1]), MaxEntry(mat[2]), MaxEntry(mat[3])); }

vec3 FindEigenVectorAssociatedWithLargestEigenValue(const mat3& mat, vec3 v) {
	float Scale = MaxEntry(mat);
	float ScaleInv = 1.f / Scale;
	mat3 mc = mat * ScaleInv;

	// do "8 iterations per iteration"
	mc = mc * mc;
	mc = mc * mc;
	mc = mc * mc;

	for(int it=0; it<8; ++it) {
		v = mc * v;
		v = mc * v;
		v = glm::normalize(v);
	}

	return v;
}

vec3 FindCenter(const vec3* points, int npoints) {
	vec3 sum(0.f, 0.f, 0.f);
	for (auto it = 0; it < npoints; ++it)
		sum += points[it];
	return sum * (1.f / npoints);
}

fit_plane FindFitPlane(const vec3* points, int npoints) {
	CHECK_ASSERT(npoints > 2);
	let center = FindCenter(points, npoints);
	let guess = glm::normalize( glm::cross(points[1] - points[0], points[2] - points[0]) );
	return FindFitPlane(points, npoints, center, guess);
}

fit_plane FindFitPlane(const vec3* points, int npoints, const vec3& guess) {
	let center = FindCenter(points, npoints);
	return FindFitPlane(points, npoints, center, guess);
}

fit_plane FindFitPlane(const vec3* points, int npoints, const vec3& center, const vec3& guess) {
	// finds a regression-plane normal using a least-squares method

	float sumXX = 0.0f, sumXY = 0.0f, sumXZ = 0.0f;
	float sumYY = 0.0f, sumYZ = 0.0f;
	float sumZZ = 0.0f;
	
	for (int it = 0; it < npoints; it++) {
		let diff = points[it] - center;
		sumXX += diff.x * diff.x;
		sumXY += diff.x * diff.y;
		sumXZ += diff.x * diff.z;
		sumYY += diff.y * diff.y;
		sumYZ += diff.y * diff.z;
		sumZZ += diff.z * diff.z;
	}
	
	const mat3 m(
		sumXX, sumXY, sumXZ,
		sumXY, sumYY, sumYZ,
		sumXZ, sumYZ, sumZZ
	);
	
	let mi = glm::inverse(m);
	let normal = FindEigenVectorAssociatedWithLargestEigenValue(mi, guess);
	let sign = glm::sign(glm::dot(normal, guess));
	return fit_plane { center, sign * normal };
}
