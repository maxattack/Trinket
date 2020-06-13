// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Assets.h"
#include "World.h"

// TODO: handle World_WillReleaseObject

namespace physx {
	class PxFoundation;
	class PxPhysics;
	class PxDefaultCpuDispatcher;
	class PxScene;
	class PxMaterial;
	class PxRigidStatic;
	class PxRigidDynamic;

#if TRINKET_EDITOR
	class PxCooking;
#endif
}

class Physics {
private:
	AssetDatabase *pAssets;
	World* pWorld;
	physx::PxFoundation* pFoundation;
	physx::PxPhysics* pPhysics;
	physx::PxDefaultCpuDispatcher* pDispatcher;
#if TRINKET_EDITOR
	physx::PxCooking* pCooking;
#endif

	physx::PxScene* pDefaultScene;
	physx::PxMaterial* pDefaultMaterial;
	physx::PxRigidStatic *pGroundPlane;

	float worldToSimScale = 1.f;
	float simToWorldScale = 1.f;
	float timeAccum = 0.f;
	float fixedDeltaTime = 0.01f;
	float fixedTime;

	enum ComponentType { C_OBJECT_ID, C_BODY, C_PREV_POSE };
	ObjectPool<physx::PxRigidDynamic*, RPose> rigidBodies;

public:

	Physics(AssetDatabase* aAssets, World* aWorld);
	~Physics();

	void TryAddGroundPlane();
	bool TryAttachRigidbodyTo(ObjectID id);
	bool TryAttachBoxTo(ObjectID id, float extent, float density);

	void Tick(float dt);


};