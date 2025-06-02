#include "SimulationEventCallback.h"
#include "Components/PrimitiveComponent.h" // UPrimitiveComponent 정의
#include "GameFramework/Actor.h"          // AActor 정의
#include "PhysicsEngine/BodyInstance.h"   // FBodyInstance 정의
#include "Engine/Engine.h"                // GEngine 접근 (필요시)
#include "PhysicsManager.h"             // FPhysicsManager 접근 (필요시)
#include "Math/Vector.h"                // FVector 등

FSimulationEventCallback::FSimulationEventCallback() {}

void FSimulationEventCallback::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) 
{
    // UE_LOG(ELogLevel::Display, TEXT("Constraint broken."));
}
void FSimulationEventCallback::onWake(physx::PxActor** actors, physx::PxU32 count) 
{
    // UE_LOG(ELogLevel::Display, TEXT("Actor woken."));
}
void FSimulationEventCallback::onSleep(physx::PxActor** actors, physx::PxU32 count) 
{
    // UE_LOG(ELogLevel::Display, TEXT("Actor slept."));
}
void FSimulationEventCallback::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) 
{
    // UE_LOG(ELogLevel::Display, TEXT("Trigger event."));
    // 여기에 오버랩 이벤트 처리 로직을 넣을 수 있습니다.
    // (onContact와 유사하게 UPrimitiveComponent를 가져와서 OnComponentBeginOverlap/EndOverlap 호출)
}
void FSimulationEventCallback::onAdvance(const physx::PxRigidBody*const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) {}

void FSimulationEventCallback::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
{
    physx::PxActor* actor0 = pairHeader.actors[0];
    physx::PxActor* actor1 = pairHeader.actors[1];

    if (!actor0 || !actor1) return;

    FBodyInstance* bodyInstance0 = static_cast<FBodyInstance*>(actor0->userData);
    FBodyInstance* bodyInstance1 = static_cast<FBodyInstance*>(actor1->userData);

    if (!bodyInstance0 || !bodyInstance1) return;

    UPrimitiveComponent* primComp0 = bodyInstance0->OwnerComponent;
    UPrimitiveComponent* primComp1 = bodyInstance1->OwnerComponent;

    if (!primComp0 || !primComp1) return;

    AActor* ownerActor0 = primComp0->GetOwner();
    AActor* ownerActor1 = primComp1->GetOwner();

    if (!ownerActor0 || !ownerActor1) return;

    for (physx::PxU32 i = 0; i < nbPairs; ++i)
    {
        const physx::PxContactPair& cp = pairs[i];

        // eNOTIFY_TOUCH_FOUND 이벤트가 발생했고, 실제 접촉점이 있는 경우에만 처리
        if ((cp.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND) && cp.contactCount > 0)
        {
            // UE_LOG(ELogLevel::Display, TEXT("Contact detected between %s and %s"), *primComp0->GetName(), *primComp1->GetName());

            // PxContactPairPoint를 저장할 버퍼 (충분한 크기로 할당)
            // cp.contactCount는 이 PxContactPair에 대한 실제 접촉점 수를 나타냅니다.
            TArray<physx::PxContactPairPoint> contactPairPoints;
            contactPairPoints.SetNum(cp.contactCount); 

            // extractContacts 함수를 사용하여 접촉점 정보 추출
            PxU32 nbExtractedPoints = cp.extractContacts(contactPairPoints.GetData(), cp.contactCount);

            for (PxU32 ptIdx = 0; ptIdx < nbExtractedPoints; ++ptIdx)
            {
                const physx::PxContactPairPoint& contactPoint = contactPairPoints[ptIdx];

                FHitResult hitResult;
                hitResult.ImpactPoint = FVector(contactPoint.position.x, contactPoint.position.y, contactPoint.position.z);
                // contactPoint.normal은 shape1에서 shape0을 향하는 법선일 수 있습니다. (문서 확인 필요)
                // PxContactPairPoint 문서: "The normal direction points from the second shape to the first shape."
                // 즉, actor1에서 actor0으로 향하는 법선입니다.
                // primComp0 입장에서의 충돌 법선은 이 방향이 맞습니다.
                hitResult.ImpactNormal = FVector(contactPoint.normal.x, contactPoint.normal.y, contactPoint.normal.z);
                hitResult.HitActor = ownerActor1;
                hitResult.Component = primComp1;
                hitResult.Distance = -contactPoint.separation; // separation이 음수면 침투

                // PxContactPairPoint.impulse는 이미 충격량 벡터입니다.
                FVector normalImpulseVector = FVector(contactPoint.impulse.x, contactPoint.impulse.y, contactPoint.impulse.z);

                // primComp0에 대한 OnComponentHit 호출
                //if (primComp0->OnComponentHit.IsBound())
                {
                    primComp0->OnComponentHit.Broadcast(primComp0, ownerActor1, primComp1, normalImpulseVector, hitResult);
                }

                // primComp1에 대한 OnComponentHit 호출 (정보 반전)
                FHitResult hitResultOther;
                hitResultOther.ImpactPoint = hitResult.ImpactPoint;
                hitResultOther.ImpactNormal = -hitResult.ImpactNormal; // 법선 반전 (actor0 -> actor1)
                hitResultOther.HitActor = ownerActor0;
                hitResultOther.Component = primComp0;
                hitResultOther.Distance = hitResult.Distance;

                // 충격량은 크기는 같고 방향은 반대
                FVector normalImpulseVectorOther = -normalImpulseVector;

                //if (primComp1->OnComponentHit.IsBound())
                {
                    primComp1->OnComponentHit.Broadcast(primComp1, ownerActor0, primComp0, normalImpulseVectorOther, hitResultOther);
                }
            }
        }
    }
}
