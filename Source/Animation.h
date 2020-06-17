// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#pragma once
#include "Skeleton.h"

// Rigs are attached to SkelAssets to associate
// animation data and IK controllers with them.
class CharacterRig : public ObjectComponent {
public:

	CharacterRig(SkelAsset* aSkel) 
		: ObjectComponent(aSkel->ID()) 
		, pSkel(aSkel)
	{}

	SkelAsset* GetSkelAsset() const { return pSkel; }

private:

	SkelAsset* pSkel;

};

// Animators are attached to Skeletons to pose them.
class Animator : public ObjectComponent {
public:

	Animator(CharacterRig* aRig, Skeleton* aSkel)
		: ObjectComponent(aSkel->ID())
		, pRig(aRig)
		, pSkeleton(aSkel)
	{}

	CharacterRig* GetRig() const { return pRig; }
	Skeleton* GetSkeleton() const { return pSkeleton; }
	

private:

	CharacterRig* pRig;
	Skeleton* pSkeleton;

};

// Animation runtime is the root of the animation system.
class AnimationRuntime : ISkelRegistryListener {
public:

	AnimationRuntime(SkelRegistry* aRegistry);
	~AnimationRuntime();

	AssetDatabase* GetAssets() const { return pRegistry->GetAssets(); }
	Scene* GetScene() const { return pRegistry->GetScene(); }


	CharacterRig* CreateCharacterRig(SkelAsset* skel);
	Animator* AttachAnimatorTo(CharacterRig* rig, Skeleton* skeleton);

private:

	SkelRegistry* pRegistry;
	ObjectPool<StrongRef<CharacterRig>> rigs;
	ObjectPool<StrongRef<Animator>> animators;

	void Skeleton_WillReleaseSkeleton(class SkelRegistry* Caller, ObjectID id) override;
	void Skeleton_WillReleaseSkelAsset(class SkelRegistry* Caller, ObjectID id) override;

};

