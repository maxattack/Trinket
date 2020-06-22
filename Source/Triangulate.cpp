// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Triangulate.h"
#include "FitPlane.h"
#include <glm/gtx/perpendicular.hpp>

#include <cstdio>

float Area2(vec2 p1, vec2 p2, vec2 p3) {
	// twice the triangle area (still works)
	return fabsf(//0.5f * (
		p1.x * (p2.y - p3.y) + 
		p2.x * (p3.y - p1.y) +
		p3.x * (p1.y - p2.y)
	);
}

float Det(vec2 u, vec2 v) {
	return u.x * v.y - u.y * v.x;
}

float Sign(vec2 p1, vec2 p2, vec2 p3) {
	return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}


bool Contains(vec2 p1, vec2 p2, vec2 p3, vec2 p) {
	//
	//let A = Area2(p1, p2, p3);
	//let A1 = Area2(p, p2, p3);
	//let A2 = Area2(p1, p, p3);
	//let A3 = Area2(p1, p2, p);
	//let diff = (A1 + A2 + A3) - A;
	//let epsilon = 0.0000001f;
	//return diff < epsilon;

	//
	//let v0 = p1;
	//let v1 = p2 - p1;
	//let v2 = p3 - p1;
	//let a = (Det(p, v2) - Det(v0, v2)) / Det(v1, v2);
	//let b = (Det(p, v1) - Det(v0, v1)) / Det(v1, v2);
	//return a > 0.f && b > 0.f && a + b < 1.f;


	let d1 = Sign(p, p1, p2);
	let d2 = Sign(p, p2, p3);
	let d3 = Sign(p, p3, p1);
	let has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
	let has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
	return !(has_neg && has_pos);
}

bool IsConvex(const vec2* p, int n) {
	// via thundergod bourke http://paulbourke.net/geometry/polygonmesh/
	int flag = 0;
	for (int i = 0; i < n; i++) {
		let j = (i + 1) % n;
		let k = (i + 2) % n;
		let z =  (p[j].x - p[i].x) * (p[k].y - p[j].y) - (p[j].y - p[i].y) * (p[k].x - p[j].x);
		if (z < 0.f)
			flag |= 1;
		else if (z > 0.f)
			flag |= 2;
		if (flag == 3)
			return false;
	}
	return true;
}

#define MAX_EDGE_LOOP_LENGTH 32

bool Triangulate(uint32* outIndex, const vec3* points, int npoints) {
	CHECK_ASSERT(npoints >= 3);
	CHECK_ASSERT(npoints <= MAX_EDGE_LOOP_LENGTH);

	// degenerate case
	let triangleCount = npoints - 2;
	if (triangleCount == 1) {
		for(int it=0; it<npoints; ++it)
			outIndex[it] = it;
		return true;
	}
	
	// fit a plane to the face
	//let fitPlane = FindFitPlane(points, npoints);
	//let z = fitPlane.normal;
	let center = FindCenter(points, npoints);
	vec3 z = glm::normalize( glm::cross(points[1] - points[0], points[2] - points[0]) );
	vec3 x = glm::normalize( glm::perp(points[1] - points[0], z) );
	vec3 y = glm::cross(z, x);
	mat3 unrotate = glm::transpose(mat3(x, y, z));

	// project the face to a 2D polygon
	vec2 polygon[MAX_EDGE_LOOP_LENGTH];
	for(int it=0; it<npoints; ++it)
		polygon[it] = vec2(unrotate * (points[it] - center));
	
	// convex-case
	if (IsConvex(polygon, npoints)) {
		for(int it=0; it<triangleCount; ++it) {
			outIndex[3*it+0] = 0;
			outIndex[3*it+1] = it + 1;
			outIndex[3*it+2] = it + 2;
		}
		return true;
	}

	// fixup winding order?
	float signedArea = 0.f;
	for (int it = 0; it < npoints; ++it) {
		let prev = polygon[it];
		let next = polygon[(it + 1) % npoints];
		signedArea += (next.x - prev.x) * (next.y + prev.y);
	}
	if (signedArea > 0.f) {
		z = -z;
		vec3 y = glm::cross(z, x);
		mat3 unrotate = glm::transpose(mat3(x, y, z));
		for (int it = 0; it < npoints; ++it)
			polygon[it] = vec2(unrotate * (points[it] - center));
	}

	//printf("---------------------\n");
	//for(int it=0; it<npoints; ++it) {
	//	printf("%.4f, %.4f,\n", polygon[it].x, polygon[it].y);
	//}
	//printf("---------------------\n");

	// ear-cutting case
	struct ear_t {
		uint32 p1;
		uint32 p2;
		uint32 p3;
	};

	ear_t ears[MAX_EDGE_LOOP_LENGTH];
	uint32 currLoop[MAX_EDGE_LOOP_LENGTH];
	int earCount = 0;
	int currLen = npoints;
	for(int it=0; it<npoints; ++it)
		currLoop[it] = it;


	let convexCheck = [&](vec2 prev, vec2 curr, vec2 next) {
		return ( (curr.x - prev.x) * (next.y - curr.y) - (curr.y - prev.y) * (next.x - curr.x) ) > 0.f;
	};

	let isConvex = [&](int idx) -> bool {
		let prev = polygon[currLoop[(idx + currLen - 1) % currLen]];
		let curr = polygon[currLoop[idx]];
		let next = polygon[currLoop[(idx + 1) % currLen]];
		return convexCheck(prev, curr, next);
	};

	let isEar = [&](int idx) -> bool {
		if (!isConvex((idx + 1) % currLen))
			return false;
		let p1 = polygon[currLoop[idx]];
		let p2 = polygon[currLoop[(idx + 1) % currLen]];
		let p3 = polygon[currLoop[(idx + 2) % currLen]];
		return 
			convexCheck(p1, p2, p3) &&
			convexCheck(p2, p3, p1) &&
			convexCheck(p3, p1, p2);
	};

	while(currLen > 3) {

		// find next ear
		int earIdx = -1;
		for(int it=0; it<currLen; ++it) {
			if (isEar(it)) {
				earIdx = it;
				break;
			}
		}
		if (earIdx == -1)
			earIdx = 0;

		// add it to the ear list
		ear_t ear;
		ear.p1 = currLoop[earIdx];
		ear.p2 = currLoop[(earIdx + 1) % currLen];
		ear.p3 = currLoop[(earIdx + 2) % currLen];
		ears[earCount] = ear;
		++earCount;

		// remove midpoint of the ear from the loop
		let removeIndex = (earIdx + 1) % currLen;
		--currLen;
		for(int it=removeIndex; it<currLen; ++it)
			currLoop[it] = currLoop[it+1];

	}

	// last ear
	ears[earCount] = ear_t{ currLoop[0], currLoop[1], currLoop[2] };
	++earCount;

	CHECK_ASSERT(earCount == triangleCount);

	// write ears
	for(int it=0; it<earCount; ++it) {
		outIndex[3 * it + 0] = ears[it].p1;
		outIndex[3 * it + 1] = ears[it].p2;
		outIndex[3 * it + 2] = ears[it].p3;
	}

	return false;
}
