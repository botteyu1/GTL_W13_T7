#include "GravitySkeletalActor.h"

#include "Components/SkeletalMeshComponent.h"

AGravitySkeletalActor::AGravitySkeletalActor()
{
    USkeletalMeshComponent* SkeletalMeshComp = AddComponent<USkeletalMeshComponent>(TEXT("SkeletalMesh"));

    SkeletalMeshComp->bSimulate = true;
    SkeletalMeshComp->bApplyGravity = true; // 중력 적용
    SkeletalMeshComp->RigidBodyType = ERigidBodyType::DYNAMIC; // 동적 리지드바디
    SkeletalMeshComp->MinImpactForceToDestroy  = 500.0f; // 충돌로 파괴되는 최소 힘 설정

    USkeletalMesh* SkeletalMesh = UAssetManager::Get().GetSkeletalMesh(FName("Contents/Toad/Toad"));
    if (SkeletalMesh)
    {
        SkeletalMeshComp->SetSkeletalMeshAsset(SkeletalMesh);
    }
    SkeletalMeshComp->SetRelativeScale3D(FVector(0.3f, 0.3f, 0.3f)); // 스케일 조정

    UPrimitiveComponent* PhysicsColliderComponent = AddComponent<UPrimitiveComponent>(TEXT("PhysicsCollider"));
    PhysicsColliderComponent->bSimulate = true;
    PhysicsColliderComponent->bApplyGravity = true; // 중력 적용
    PhysicsColliderComponent->RigidBodyType = ERigidBodyType::DYNAMIC; // 동적 리지드바디
    
    PhysicsColliderComponent->MinImpactForceToDestroy  = 500.0f; // 충돌로 파괴되는 최소 힘 설정
    SkeletalMeshComp->SetupAttachment(PhysicsColliderComponent);
    PhysicsColliderComponent->AABB = SkeletalMeshComp->GetBoundingBox(); // IntactMeshComponent의 AABB를 사용
    
    RootComponent = PhysicsColliderComponent;
    
}

void AGravitySkeletalActor::BeginPlay()
{
    Super::BeginPlay();
}
