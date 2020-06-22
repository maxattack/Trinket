// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Triangulate.h"
#include "FitPlane.h"
#include <glm/gtx/perpendicular.hpp>

float Area2(vec2 p1, vec2 p2, vec2 p3) {
	// twice the triangle area (still works)
	return fabsf(//0.5f * (
		p1.x * (p2.y - p3.y) + 
		p2.x * (p3.y - p1.y) +
		p3.x * (p1.y - p2.y)
	);
}

bool Contains(vec2 p1, vec2 p2, vec2 p3, vec2 p) {
	let A = Area2(p1, p2, p3);
	let A1 = Area2(p, p2, p3);
	let A2 = Area2(p1, p, p3);
	let A3 = Area2(p1, p2, p);
	let diff = A - (A1 + A2 + A3);
	let epsilon = 0.0001f;
	return diff < epsilon && diff > -epsilon;
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

void Triangulate(uint32* outIndex, const vec3* points, int npoints) {
	CHECK_ASSERT(npoints >= 3);
	CHECK_ASSERT(npoints <= MAX_EDGE_LOOP_LENGTH);

	// degenerate case
	let triangleCount = npoints - 2;
	if (triangleCount == 1) {
		for(int it=0; it<npoints; ++it)
			outIndex[it] = it;
		return;
	}
	
	// fit a plane to the face
	let fitPlane = FindFitPlane(points, npoints);
	let y = fitPlane.normal;
	let x = glm::normalize( glm::perp(points[1] - points[0], y) );
	let z = glm::cross(x, y);
	let unrotate = glm::transpose(mat3(x, y, z));

	// project the face to a 2D polygon
	vec2 polygon[MAX_EDGE_LOOP_LENGTH];
	for(int it=0; it<npoints; ++it)
		polygon[it] = vec2(unrotate * (points[it] - fitPlane.center));
	
	// convex-case
	if (IsConvex(polygon, npoints)) {
		for(int it=0; it<triangleCount; ++it) {
			outIndex[3*it] = 0;
			outIndex[3*it] = it + 1;
			outIndex[3*it] = it + 2;
		}
		return;
	}

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

	let isEar = [&](int idx) {
		let idx1 = currLoop[idx];
		let idx2 = currLoop[(idx + 1) % currLen];
		let idx3 = currLoop[(idx + 2) % currLen];
		let p1 = polygon[idx1];
		let p2 = polygon[idx2];
		let p3 = polygon[idx3];
		for(int it=0; it<currLen-3; ++it) {
			let idxn = (idx + 3 + it) % currLen;
			let p = polygon[idxn];
			if (Contains(p1, p2, p3, p))
				return false;
		}
		return true;
	};

	while(currLen > 3) {

		// find next ear
		int earIdx = 0;
		for(int it=0; it<currLen; ++it) {
			if (isEar(it)) {
				earIdx = it;
				break;
			}
		}

		// add it to the ear list
		ear_t ear;
		ear.p1 = currLoop[earIdx];
		ear.p2 = currLoop[(earIdx + 1) % currLen];
		ear.p3 = currLoop[(earIdx + 2) % currLen];
		ears[earCount] = ear;
		++earCount;

		// remove midpoint of the ear from the loop
		let removeIndex = (earIdx + 1) % currLen;
		for(int it=removeIndex; it<currLen; ++it)
			currLoop[it] = currLoop[it+1];
		--currLen;

	}

	// last ear
	ears[earCount] = ear_t{ currLoop[0], currLoop[1], currLoop[2] };
	++earCount;

	// write ears
	for(int it=0; it<earCount; ++it) {
		let it3 = it + it + it;
		outIndex[it3    ] = ears[it].p1;
		outIndex[it3 + 1] = ears[it].p2;
		outIndex[it3 + 2] = ears[it].p3;
	}

}
