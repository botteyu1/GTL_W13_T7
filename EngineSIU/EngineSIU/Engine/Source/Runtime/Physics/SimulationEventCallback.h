#pragma once
#include <PxPhysicsAPI.h>
#include "Engine/EngineTypes.h" // FHitResult 등

class UPrimitiveComponent; // 전방 선언
class AActor; // 전방 선언

class FSimulationEventCallback : public physx::PxSimulationEventCallback
{
public:
    FSimulationEventCallback();

    virtual void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override;
    virtual void onWake(physx::PxActor** actors, physx::PxU32 count) override;
    virtual void onSleep(physx::PxActor** actors, physx::PxU32 count) override;
    virtual void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;
    virtual void onAdvance(const physx::PxRigidBody*const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override;
    
    // 핵심: 충돌 이벤트 처리
    virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;
};
