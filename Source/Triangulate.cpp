// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Triangulate.h"
#include <glm/gtx/perpendicular.hpp>

#include <cstdio>

bool Contains(vec2 p1, vec2 p2, vec2 p3, vec2 p) {
	let Sign = [](vec2 p1, vec2 p2, vec2 p3) -> float {
		return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
	};
	let d1 = Sign(p, p1, p2);
	let d2 = Sign(p, p2, p3);
	let d3 = Sign(p, p3, p1);
	let has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
	let has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
	return !(has_neg && has_pos);
}

static float CrossCheck(vec2 prev, vec2 curr, vec2 next) {
	return (curr.x - prev.x) * (next.y - curr.y) - (curr.y - prev.y) * (next.x - curr.x);	
}

static bool IsConvex(const vec2* p, int n) {
	// via thundergod bourke http://paulbourke.net/geometry/polygonmesh/
	int flag = 0;
	for (int i = 0; i < n; i++) {
		let j = (i + 1) % n;
		let k = (i + 2) % n;
		let z = CrossCheck(p[i], p[j], p[k]);
		
		if (z < 0.f)
			flag |= 1;
		else if (z > 0.f)
			flag |= 2;

		if (flag == 3)
			return false;

	}
	return true;
}

bool Triangulate(uint32* outIndex, const vec3* points, int npoints) {
	CHECK_ASSERT(npoints >= 3);
	CHECK_ASSERT(npoints <= MAX_EDGE_LOOP_LENGTH);

	// degenerate cases
	let triangleCount = npoints - 2;
	if (triangleCount == 1) {
		for(int it=0; it<npoints; ++it)
			outIndex[it] = it;
		return true;
	}
	
	// centering helps with precision (could also rescale?)
	vec3 sum(0.f, 0.f, 0.f);
	for (auto it = 0; it < npoints; ++it)
		sum += points[it];
	let center = sum * (1.f / npoints);


	// project the face to a 2D polygon
	let z = glm::normalize( glm::cross(points[0] - center, points[1] - center) );
	let x = glm::normalize( glm::perp(points[1] - points[0], z) );
	let y = glm::cross(z, x);
	let unrotate = glm::transpose(mat3(x, y, z));
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

	// ear-cutting case
	struct ear_t {
		uint32 p1;
		uint32 p2;
		uint32 p3;
	};

	ear_t ears[MAX_EDGE_LOOP_LENGTH-2];
	uint32 currLoop[MAX_EDGE_LOOP_LENGTH];
	int earCount = 0;
	int currLen = npoints;

	// make sure winding order is correct
	// via https://stackoverflow.com/a/1165943
	//float signedArea = 0.f;
	//for (int it = 0; it < npoints; ++it) {
	//	let prev = polygon[it];
	//	let next = polygon[(it + 1) % npoints];
	//	signedArea -= (next.x - prev.x) * (next.y + prev.y);
	//}
	// faster check: http://www.faqs.org/faqs/graphics/algorithms-faq/
	int sm = 0;
	for(int it=1; it<npoints; ++it) {
		let smaller = 
			(polygon[it].y < polygon[sm].y) || 
			(polygon[it].y == polygon[sm].y && polygon[it].x < polygon[sm].x);
		if (smaller)
			sm = it;
	}
	let signedArea = CrossCheck(
		polygon[(sm + npoints - 1) % npoints], 
		polygon[sm],
		polygon[(sm + 1) % npoints]
	);
	if (signedArea < 0.f) {
		for (int it = 0; it < npoints; ++it)
			currLoop[it] = npoints - 1 - it;
	} else {
		for (int it = 0; it < npoints; ++it)
			currLoop[it] = it;
	}


	bool bBadEar = false;

	while(currLen > 3) {

		// find next ear
		int earIdx = -1;
		for(int idx=0; idx<currLen; ++idx) {
			let p1 = polygon[currLoop[idx]];
			let p2 = polygon[currLoop[(idx + 1) % currLen]];
			let p3 = polygon[currLoop[(idx + 2) % currLen]];
			let convex = 
				CrossCheck(p1, p2, p3) > 0.f ;//&&
				//CrossCheck(p2, p3, p1) > 0.f &&
				//CrossCheck(p3, p1, p2) > 0.f;
			if (!convex)
				continue;

			bool bContains = false;
			for(int it=0; it < currLen - 3; ++it) {
				let p = polygon[currLoop[(idx + 3 + it) % currLen]];
				if (Contains(p1, p2, p3, p)) {
					bContains = true;
					break;
				}
			}
			if (bContains)
				continue;

			
			earIdx = idx;
			break;
		}
		if (earIdx == -1) {
			earIdx = 0;
			bBadEar = true;
		}

		// add it to the ear list
		ear_t ear;
		let idxMid = (earIdx + 1) % currLen;
		ear.p1 = currLoop[earIdx];
		ear.p2 = currLoop[idxMid];
		ear.p3 = currLoop[(earIdx + 2) % currLen];
		ears[earCount] = ear;
		++earCount;

		// remove midpoint of the ear from the loop
		--currLen;
		for(int it=idxMid; it<currLen; ++it)
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

	//if (bBadEar) {
	//	printf("---------------------\n");
	//	for(int it=0; it<npoints; ++it) {
	//		printf("%.4f, %.4f,\n", polygon[it].x, polygon[it].y);
	//	}
	//	printf("---------------------\n");
	//	for(int it=0; it<earCount; ++it)
	//		printf("%d, %d, %d,\n", ears[it].p1, ears[it].p2, ears[it].p3);
	//	printf("---------------------\n");
	//}

	return false;
}
