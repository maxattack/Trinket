// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Common.h"
#include <foundation/PxTransform.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/transform.hpp>

#define RPOSE_IDENTITY  (RPose(ForceInit::Default))
#define HPOSE_IDENTITY (HPose(ForceInit::Default))
#define RELATIVE_POSE_IDENTITY (RelativePose(ForceInit::Default))

union AliasVec3 {
	vec3 v;
	physx::PxVec3 px;

	AliasVec3(const vec3& a) noexcept : v(a) {}
	AliasVec3(const physx::PxVec3& a) noexcept : px(a) {}

};

union AliasQuat {
	quat q;
	physx::PxQuat px;
	AliasQuat(const quat& a) noexcept : q(a) {}
	AliasQuat(const physx::PxQuat& a) noexcept : px(a) {}
};

inline physx::PxVec3 ToPx(const vec3 v) { return AliasVec3(v).px; }
inline physx::PxQuat ToPx(const quat q) { return AliasQuat(q).px; }
inline vec3 FromPx(physx::PxVec3 v) { return AliasVec3(v).v; }
inline quat FromPx(physx::PxQuat q) { return AliasQuat(q).q; }



struct RPose {
	
	// rigid poses simplify things by omitting scale

	quat rotation;
	vec3 position;

	RPose() noexcept = default;
	RPose(const RPose&) noexcept = default;
	RPose(RPose&&) noexcept = default;
	RPose& operator=(const RPose&) noexcept = default;

	explicit RPose(ForceInit) noexcept : rotation(1.f, 0.f, 0.f, 0.f), position(0.f, 0.f, 0.f) {}
	explicit RPose(vec3 aPosition) noexcept : rotation(1.f, 0.f, 0.f, 0.f), position(aPosition) {}
	explicit RPose(quat aRotation) noexcept : rotation(aRotation), position(0.f, 0.f, 0.f) {}
	explicit RPose(quat aRotation, vec3 aPosition) noexcept : rotation(aRotation), position(aPosition) {}
	explicit RPose(const physx::PxTransform& px) noexcept : rotation(FromPx(px.q)), position(FromPx(px.p)) {}

	mat4 ToMatrix() const { 
		//return glm::translate(position) * mat4(rotation); 
		const mat3 result(rotation);
		return mat4(
			vec4(result[0], 0.f),
			vec4(result[1], 0.f),
			vec4(result[2], 0.f),
			vec4(position, 1.f)
		);
	}
	physx::PxTransform ToPX() const { return physx::PxTransform(ToPx(position), ToPx(rotation)); }

	RPose operator*(const RPose& rhs) const { return RPose(rotation * rhs.rotation, position + rotation * (rhs.position)); }

	RPose Inverse() const {
		let inverseRotation = glm::inverse(rotation);
		return RPose(inverseRotation, inverseRotation * (-position));
	}

	vec3 TransformPosition(const vec3& localPosition) const { return position + rotation * localPosition; }
	vec3 TransformVector(const vec3& localVector) const { return rotation * localVector; }

	vec3 InvTransformPosition(const vec3& worldPosition) const { return glm::inverse(rotation) * (worldPosition - position); }
	vec3 InvTransformVector(const vec3& worldVector) const { return glm::inverse(rotation) * worldVector; }

	vec3 Right() const { return rotation * vec3(1,0,0); }
	vec3 Up() const { return rotation * vec3(0,1,0); }
	vec3 Forward() const { return rotation * vec3(0,0,1); }

	static RPose NLerp(const RPose& lhs, const RPose& rhs, float t) {
		return RPose(
			glm::normalize(glm::lerp(lhs.rotation, rhs.rotation, t)),
			glm::mix(lhs.position, rhs.position, t)
		);
	}
};

struct HPose {

	// "Hierarchy" Pose Transforms are similar, but not strictly analogous,
	// to affine transforms. In particular, the scales are multiplied 
	// component-wise, without respect to rotation, so skew cannot be
	// introduced.

	union {
		RPose rpose;
		struct {
			quat rotation;
			vec3 position;
		};
	};
	vec3 scale;

	HPose() noexcept = default;
	HPose(const HPose&) noexcept = default;
	HPose(HPose&&) noexcept = default;
	HPose& operator=(const HPose&) noexcept = default;

	explicit HPose(ForceInit) noexcept : rotation(1.f, 0.f, 0.f, 0.f), position(0.f, 0.f, 0.f), scale(1.f, 1.f, 1.f) {}
	explicit HPose(vec3 aPosition) noexcept : rotation(1.f, 0.f, 0.f, 0.f), position(aPosition), scale(1.f, 1.f, 1.f) {}
	explicit HPose(quat aRotation) noexcept : rotation(aRotation), position(0.f, 0.f, 0.f), scale(1.f, 1.f, 1.f) {}
	explicit HPose(quat aRotation, vec3 aPosition) noexcept : rotation(aRotation), position(aPosition), scale(1.f, 1.f, 1.f) {}
	explicit HPose(RPose apose) noexcept : rotation(apose.rotation), position(apose.position), scale(1.f, 1.f, 1.f) {}
	explicit HPose(quat aRotation, vec3 aPosition, vec3 aScale) noexcept : rotation(aRotation), position(aPosition), scale(aScale) {}
	explicit HPose(physx::PxTransform& px) noexcept : HPose(RPose(px)) {}

	mat4 ToMatrix() const { 
		//return glm::translate(position) * mat4(rotation) * glm::scale(scale); 
		const mat3 result(rotation);
		return mat4(
			vec4(scale.x * result[0], 0.f),
			vec4(scale.y * result[1], 0.f),
			vec4(scale.z * result[2], 0.f),
			vec4(position, 1.f)
		);
	}

	physx::PxTransform ToPX() const { return rpose.ToPX(); }

	HPose operator*(const HPose& rhs) const {
		return HPose(
			rotation * rhs.rotation,
			position + rotation * (scale * rhs.position),
			scale * rhs.scale
		);
	}

	HPose Inverse() const {
		let inverseRotation = glm::inverse(rotation);
		let inverseScale = 1.f / scale;
		return HPose(inverseRotation, inverseRotation * (-position * inverseScale), inverseScale);
	}

	vec3 TransformPosition(const vec3& localPosition) const { return position + rotation * (scale * localPosition); }
	vec3 TransformVector(const vec3& localVector) const { return rotation * (scale * localVector); }

	vec3 InvTransformPosition(const vec3& worldPosition) const { return glm::inverse(rotation) * ((worldPosition - position) / scale); }
	vec3 InvTransformVector(const vec3& worldVector) const { return glm::inverse(rotation) * (worldVector / scale); }

	vec3 Right() const { return rotation * vec3(1, 0, 0); }
	vec3 Up() const { return rotation * vec3(0, 1, 0); }
	vec3 Forward() const { return rotation * vec3(0, 0, 1); }

};

union PoseMask {

	struct {
		uint8 ignorePosition : 1;
		uint8 ignoreRotation : 1;
		uint8 ignoreScale : 1;
	};
	struct {
		uint8 allIgnoreBits : 3;
	};
	uint8 ignoreFlags;

	PoseMask() noexcept = default;
	PoseMask(const PoseMask&) noexcept = default;
	PoseMask(PoseMask&&) noexcept = default;
	PoseMask& operator=(const PoseMask&) = default;

	explicit PoseMask(ForceInit) noexcept : ignoreFlags(0) {}
	explicit PoseMask(uint8 aFlags) noexcept : ignoreFlags(aFlags) {}
	explicit PoseMask(bool aPosition, bool aRotation, bool aScale) noexcept : ignorePosition(aPosition), ignoreRotation(aRotation), ignoreScale(aScale) {}

	RPose Apply(const RPose& pose) const {
		let p = float(ignorePosition);
		let r = float(ignoreRotation);
		return RPose(
			quat(r, 0.f, 0.f, 0.f) + (1.f - r) * pose.rotation,
			(1.f - p) * pose.position
		);
	}

	HPose Apply(const HPose& pose) const { 
		//return HPose(
		//	ignoreRotation ? quat(1, 0, 0, 0) : pose.rotation,
		//	ignorePosition ? vec3(0, 0, 0) : pose.position,
		//	ignoreScale ? vec3(1, 1, 1) : pose.scale
		//);
		let p = float(ignorePosition);
		let r = float(ignoreRotation);
		let s = float(ignoreScale);
		return HPose(
			quat(r, 0.f, 0.f, 0.f) + (1.f - r) * pose.rotation,
			(1.f - p) * pose.position,
			vec3(s, s, s) + (1.f - s) * pose.scale
		);
	}

	HPose Concat(const HPose& lhs, const HPose& rhs) const { 
		return Apply(lhs) * rhs; 
	}
	
	HPose Rebase(const HPose& lhs, const HPose& desiredPose) const {
		// desiredPose = lhs * relative
		// relative = lhs^-1 * world
		return allIgnoreBits == 7 ? desiredPose : Apply(lhs).Inverse() * desiredPose;
		
	}
};

inline bool ContainsNaN(float f) { return glm::isnan(f); }
inline bool ContainsNaN(const vec3& v) { return glm::isnan(v.x) || glm::isnan(v.y) || glm::isnan(v.z); }
inline bool ContainsNaN(const quat& q) { return glm::isnan(q.x) || glm::isnan(q.y) || glm::isnan(q.z) || glm::isnan(q.w); }
inline bool ContainsNaN(const RPose& p) { return ContainsNaN(p.position) || ContainsNaN(p.rotation); }
inline bool ContainsNaN(const HPose& p) { return ContainsNaN(p.rpose) || ContainsNaN(p.scale); }
inline bool IsNormalized(const quat& q) { return glm::epsilonEqual(glm::dot(q, q), 1.f, 0.00001f); }
inline bool IsNormalized(const RPose& p) { return IsNormalized(p.rotation); }
inline bool IsNormalized(const HPose& p) { return IsNormalized(p.rotation); }

// TODO: Splines