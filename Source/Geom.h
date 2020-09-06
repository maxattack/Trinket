#pragma once
#include "Math.h"
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_query.hpp>

struct AABB {
	vec3 min;
	vec3 max;

	AABB() noexcept {}
	AABB(ForceInit) noexcept : min(0,0,0), max(0,0,0) {}
	AABB(const vec3& pos) noexcept : min(pos), max(pos) {}
	AABB(const vec3& aMin, const vec3& aMax) noexcept : min(aMin), max(aMax) {}

	AABB(const vec3* positions, uint count) noexcept {
		if (count == 0) {
			min = vec3(0,0,0);
			max = vec3(0,0,0);
			return;
		}

		min = positions[0];
		max = positions[0];
		for(uint it = 1; it<count; ++it) {
			min = glm::min(min, positions[it]);
			max = glm::max(max, positions[it]);
		}
			
	}

	vec3 Min() const { return min; }
	vec3 Max() const { return max; }
	vec3 Size() const { return max - min; }
	vec3 Center() const { return 0.5f * (max + min); }
	vec3 Extent() const { return 0.5f * (max - min); }

	bool IsValid() const {
		return min.x <= max.x && min.y <= max.y && min.z <= max.z; 
	}

	bool Contains(const vec3& pos) const {
		return 
			pos.x > min.x && pos.y > min.y && pos.z > min.z &&
			pos.x < max.x && pos.y < max.y && pos.z < max.z ;
	}

	AABB Union(const AABB& other) const {
		return AABB(glm::min(min, other.min), glm::max(max, other.max));
	}

	AABB ExpandTo(const vec3& pos) const {
		return AABB(glm::min(min, pos), glm::max(max, pos));
	}

	AABB Intersection(const AABB& other) const {
		return AABB(glm::max(min, other.min), glm::min(max, other.max));
	}

	bool Overlaps(const AABB& other) const {
		let delta = glm::abs(other.Center() - Center());
		let sum = other.Extent() + Extent();
		return delta.x < sum.x && delta.y < sum.y && delta.z < sum.z;
	}

	vec3 Clamp(const vec3& pos) const {
		return glm::clamp(pos, min, max);
	}

	float Distance(const vec3& pos) const {
		return glm::distance(Clamp(pos), pos);
	}

	float DistanceSq(const vec3& pos) const {
		return glm::distance2(Clamp(pos), pos);
	}

	void GetCorners(vec3* pResults) const {
		for(int it=0; it<8; ++it) {
			const vec3 uvw ( !(it & 0x01), !(it & 0x02), !(it & 0x04) );
			pResults[it] = min + uvw * (max - min);
		}
	}

	AABB GetTransformed(const mat4& xform) const {
		vec3 corners[8];
		GetCorners(corners);
		for(int it=0; it<8; ++it)
			corners[it] = xform * vec4(corners[it], 1.f);
		return AABB(corners, 8);
	}

};

struct Sphere {
	vec3 center;
	float radius;

	Sphere() noexcept {}
	Sphere(ForceInit) noexcept : center(0,0,0), radius(0) {}
	Sphere(const vec3& aCenter) noexcept : center (aCenter), radius(0.f) {}
	Sphere(const vec3& aCenter, float aRadius) noexcept : center(aCenter), radius(aRadius) {}

	Sphere(const vec3* positions, uint count) {
		AABB box (positions, count);
		center = box.Center();
		radius = glm::length(box.Extent());
	}

	bool Contains(const vec3& pos) const {
		return glm::distance2(pos, center) < radius * radius;
	}

	AABB BoundingBox() const { return AABB(center - vec3(radius, radius, radius), center + vec3(radius, radius, radius)); }

	Sphere Union(const Sphere& other) {
		let delta = other.center - center;
		if (glm::isNull(delta, 0.0001f))
			return Sphere(center, glm::max(radius, other.radius));
		
		let direction = glm::normalize(delta);
		let left = center - direction * radius;
		let right = other.center + direction * other.radius;
		return Sphere(0.5f * (left + right), 0.5f * glm::distance(left, right));
	}

	bool Overlaps(const Sphere& other) {
		let sum = radius + other.radius;
		return glm::distance2(other.center, center) < sum * sum;
	}

	vec3 Clamp(const vec3& pos) const {
		return Contains(pos) ? pos : center + radius * glm::normalize(pos - center);
	}

	float Distance(const vec3& pos) const {
		return glm::distance(Clamp(pos), pos);
	}

	float DistanceSq(const vec3& pos) const {
		return glm::distance2(Clamp(pos), pos);
	}
};

struct Plane {
	
	union {
		vec4 vec;
		struct {
			vec3 normal;
			float distance;
		};
	};

	Plane() noexcept {}
	Plane(ForceInit) noexcept : normal(0,0,0), distance(0) {}
	Plane(const vec4& aVec) noexcept : vec(aVec) {}
	Plane(const vec3& aNormal, float aDistance) noexcept : normal(aNormal), distance(aDistance) {}
	Plane(const vec3 aNormal, const vec3& pos) noexcept : normal(aNormal), distance(glm::dot(pos, aNormal)) {}

	float DistanceTo(const vec3& pos) const {
		let offset = pos - normal * distance;
		return glm::dot(offset, normal);
	}

	vec3 ClosestPointTo(const vec3& pos) {
		let ref = normal * distance;
		let offset = pos - ref;
		let dist = glm::dot(offset, normal);
		return ref + offset - (dist * normal);
	}

	bool IsAbove(const vec3& pos) const {
		return DistanceTo(pos) > 0.f;
	}

	bool IsBelow(const vec3& pos) const {
		return DistanceTo(pos) <= 0.f;
	}

	Plane GetTransformedByWorldTransform(const mat4& xform) const {
		return Plane(glm::inverseTranspose(xform) * vec);
	}

	Plane GetTransformedByInvTranspose(const mat4& invTranspose) const {
		return Plane(invTranspose * vec);
	}
};

struct FrustumParams {
	float zNear;
	float zFar;
	float fovX;
	float fovY;
};

struct FrustumPlanes {
	Plane planes[6];
};

