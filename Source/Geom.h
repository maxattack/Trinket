#pragma once
#include "Math.h"

struct AABB {
	vec3 center;
	vec3 extent;

	AABB() noexcept {}
	AABB(ForceInit) noexcept : center(0,0,0), extent(0,0,0) {}
	AABB(const vec3& aCenter, const vec3& aExtent) : center(aCenter), extent(aExtent) {}

	vec3 Min() const { return center - extent; }
	vec3 Max() const { return center + extent; }

	bool Contains(const vec3& pos) {
		let delta = glm::abs(pos - center);
		return delta.x < extent.x && delta.y < extent.y && delta.z < extent.z;
	}

	AABB Union(const AABB& other) {
		let min = glm::min(Min(), other.Min());
		let max = glm::max(Max(), other.Max());
		return AABB(0.5f * (min + max), 0.5f * (max - min));
	}

	bool Overlaps(const AABB& other) {
		let delta = glm::abs(other.center - center);
		let sum = other.extent + extent;
		return delta.x < sum.x && delta.y < sum.y && delta.z < sum.z;
	}

};

