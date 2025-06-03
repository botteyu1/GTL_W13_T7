#include "DestructibleStaticMesh.h"

#include "PhysicsManager.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/FObjLoader.h"
#include "World/World.h"

ADestructibleStaticMesh::ADestructibleStaticMesh()
{

    IntactMeshComponent = AddComponent<UStaticMeshComponent>(TEXT("IntactMesh"));
    

    FString MeshName = TEXT("Contents/Box/box1");

    UStaticMesh* StaticMesh = UAssetManager::Get().GetStaticMesh(MeshName);
    if (StaticMesh)
    {
        IntactMeshComponent->SetStaticMesh(StaticMesh);
    }
    
    // PhysicsColliderComponent 생성 및 설정
    // UBoxComponent를 사용한다고 가정
    PhysicsColliderComponent = AddComponent<UPrimitiveComponent>(TEXT("PhysicsCollider"));
    PhysicsColliderComponent->bSimulate = true;
    PhysicsColliderComponent->bApplyGravity = true; // 중력 적용
    PhysicsColliderComponent->RigidBodyType = ERigidBodyType::DYNAMIC; // 동적 리지드바디
    // PhysicsColliderComponent->SetNotifyRigidBodyCollision(true); // OnComponentHit을 받기 위해 필요
    // // 기타 필요한 설정: 크기, 상대 위치 등

    IntactMeshComponent->SetupAttachment(PhysicsColliderComponent); 
    RootComponent = PhysicsColliderComponent;
    
    
}

void ADestructibleStaticMesh::BeginPlay()
{
    Super::BeginPlay();
    
    PhysicsColliderComponent = GetComponentByFName<UPrimitiveComponent>(TEXT("PhysicsCollider"));
    IntactMeshComponent = GetComponentByClass<UStaticMeshComponent>();

    if (IntactMeshComponent)
    {
        FString MeshName = IntactMeshComponent->GetStaticMesh()->GetRenderData()->ObjectName;
        LoadFragmentMeshesByNamePattern(MeshName + TEXT("_Fragment"), 0, 15);
        if (PhysicsColliderComponent)
        {
            PhysicsColliderComponent->AABB = IntactMeshComponent->GetBoundingBox(); // IntactMeshComponent의 AABB를 사용
            // OnComponentHit 델리게이트에 함수 바인딩
            PhysicsColliderComponent->OnComponentHit.AddUObject(this, &ADestructibleStaticMesh::OnBoxHit);


            // FVector ScaledMin = PhysicsColliderComponent->AABB.MinLocation * PhysicsColliderComponent->GetComponentScale3D();
            // FVector ScaledMax = PhysicsColliderComponent->AABB.MaxLocation * PhysicsColliderComponent->GetComponentScale3D();
            //
            // FVector BoxCenter = (ScaledMin + ScaledMax) * 0.5f;
            // FVector BoxExtent = (ScaledMax - ScaledMin) * 0.5f;
            // // 파편 콜라이더의 GeomAttributes 설정
            // AggregateGeomAttributes GeomAttr;
            // GeomAttr.GeomType = EGeomType::EBox;
            // GeomAttr.Extent = BoxExtent; // SetBoxExtent로 설정한 값
            // GeomAttr.Offset = BoxCenter; // 로컬 오프셋
            // PhysicsColliderComponent->GeomAttributes.Add(GeomAttr);
            
        }
    }


}

void ADestructibleStaticMesh::LoadFragmentMeshesByNamePattern(const FString& BaseName, int32 StartIndex, int32 MaxAttempts)
{
    FragmentMeshes.Empty(); // 함수 호출 시 기존 목록을 비웁니다.

    UE_LOG(ELogLevel::Display, TEXT("Attempting to load fragment meshes with base: '%s', starting index: %d"), *BaseName, StartIndex);

    for (int32 i = 0; i < MaxAttempts; ++i)
    {
        int32 CurrentIndex = StartIndex + i;
        // FString::Printf를 사용하여 이름 생성
        FString FragmentMeshName ;
        if (CurrentIndex == 0)
        {
            FragmentMeshName = BaseName;
        }
        else
        {
            FragmentMeshName = FString::Printf(TEXT("%s%d"), *BaseName, CurrentIndex);
        }

        UStaticMesh* LoadedMesh = UAssetManager::Get().GetStaticMesh(FragmentMeshName);

        if (LoadedMesh)
        {
            FragmentMeshes.Add(LoadedMesh);
            UE_LOG(ELogLevel::Display, TEXT("Successfully loaded fragment mesh: %s"), *FragmentMeshName);
        }
        else
        {
            // 해당 이름의 메시를 찾지 못하면, 더 이상 이 패턴의 메시가 없다고 가정하고 중단합니다.
            UE_LOG(ELogLevel::Display, TEXT("Could not load fragment mesh: %s. Stopping search for this pattern."), *FragmentMeshName);
            break; 
        }
    }

    if (FragmentMeshes.Num() > 0)
    {
        UE_LOG(ELogLevel::Display, TEXT("Loaded %d fragment meshes for base: '%s'"), FragmentMeshes.Num(), *BaseName);
    }
    else
    {
        UE_LOG(ELogLevel::Warning, TEXT("No fragment meshes found for base: '%s' with start index %d and %d attempts."), *BaseName, StartIndex, MaxAttempts);
    }
}

void ADestructibleStaticMesh::OnBoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (bIsDestroyed) // 이미 파괴되었다면 무시
    {
        return;
    }

    // 충격량 계산 (NormalImpulse는 충격 방향과 크기를 나타냄)
    float ImpulseMagnitude = NormalImpulse.Size();

    if (ImpulseMagnitude > PhysicsColliderComponent->MinImpactForceToDestroy)
    {
        TriggerDestruction(Hit.ImpactPoint, NormalImpulse.GetSafeNormal(), ImpulseMagnitude);
    }
}

void ADestructibleStaticMesh::TriggerDestruction(FVector HitLocation, FVector ImpulseDirection, float ImpulseMagnitude)
{
    if (bIsDestroyed || !GEngine || !GEngine->PhysicsManager || !GEngine->ActiveWorld)
    {
        return;
    }

    bIsDestroyed = true;

    // 1. 온전한 상자 제거/숨김
    // 시각적 요소 숨기기
    if (IntactMeshComponent)
    {
        IntactMeshComponent->SetVisibility(false); // 시각적 메시를 숨긴다.
        //IntactMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 시각적 메시의 콜리전도 끈다.
    }
    if (PhysicsColliderComponent)
    {
        // PhysX 객체 제거
        if (PhysicsColliderComponent->BodyInstance && PhysicsColliderComponent->BodyInstance->BIGameObject)
        {
            GEngine->PhysicsManager->MarkGameObjectForKill(PhysicsColliderComponent->BodyInstance->BIGameObject);
            PhysicsColliderComponent->BodyInstance->BIGameObject = nullptr; // 중요: 포인터 정리
        }
        PhysicsColliderComponent->SetVisibility(false); // 콜라이더 컴포넌트도 숨김
        //PhysicsColliderComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 콜라이더 충돌 비활성화
    }

    // 2. 파편 생성 및 물리 적용
    UWorld* World = GetWorld(); // AActor::GetWorld() 사용
    if (!World)
    {
        UE_LOG(ELogLevel::Error, TEXT("ADestructibleStaticMesh::TriggerDestruction: GetWorld() returned null. Cannot spawn fragments."));
        return;
    }

    if (FragmentMeshes.IsEmpty())
    {
        UE_LOG(ELogLevel::Warning, TEXT("ADestructibleStaticMesh::TriggerDestruction: FragmentMeshes array is empty. No fragments to spawn."));
        // 이 경우, 그냥 상자를 Destroy() 할 수도 있습니다.
        // Destroy();
        return;
    }

    FVector OriginalBoxLocation = GetActorLocation(); // 원래 상자의 위치
    FRotator OriginalBoxRotation = GetActorRotation(); // 원래 상자의 회전

    const int NumFragmentsToSpawn = FragmentMeshes.Num(); // 로드된 모든 파편 메시를 사용

    for (int32 i = 0; i < NumFragmentsToSpawn; ++i)
    {
        UStaticMesh* FragmentMesh = FragmentMeshes[i % FragmentMeshes.Num()]; // 순환하며 메시 선택
        if (!FragmentMesh)
        {
            UE_LOG(ELogLevel::Warning, TEXT("FragmentMesh at index %d is null."), i);
            continue;
        }

        // 파편 액터 스폰
        // 파편의 초기 위치를 약간 랜덤하게 설정 (상자 중심 기준 또는 충격 지점 기준)
        FVector FragmentInitialLocation = OriginalBoxLocation + FVector(FMath::RandRange(-20.f, 20.f), FMath::RandRange(-20.f, 20.f), FMath::RandRange(0.f, 30.f));
        AActor* FragmentActor = World->SpawnActor<AActor>();

        if (!FragmentActor)
        {
            UE_LOG(ELogLevel::Warning, TEXT("Failed to spawn FragmentActor."));
            continue;
        }

        // 파편 메시 컴포넌트 추가
        UStaticMeshComponent* FragMeshComp = FragmentActor->AddComponent<UStaticMeshComponent>(TEXT("FragmentMesh"));
        FragMeshComp->SetStaticMesh(FragmentMesh);
        // FragMeshComp->RegisterComponent(); // SpawnActor에서 처리될 수 있음. 엔진 구현 확인.

        // 파편 물리 콜라이더 컴포넌트 추가 (UBoxComponent 사용)
        UPrimitiveComponent* FragmentCollider = FragmentActor->AddComponent<UPrimitiveComponent>(TEXT("FragmentCollider"));
        FragMeshComp->SetupAttachment(FragmentCollider); // 메시 컴포넌트에 붙임
        
        FragmentActor->SetRootComponent(FragmentCollider); // 루트로 설정

        FragmentActor->SetActorScale(GetActorScale()); // 원래 상자의 스케일을 유지

        // 파편 콜라이더 크기 설정 
        FVector ScaledMin = FragMeshComp->AABB.MinLocation * FragMeshComp->GetComponentScale3D();
        FVector ScaledMax = FragMeshComp->AABB.MaxLocation * FragMeshComp->GetComponentScale3D();

        FVector BoxCenter = (ScaledMin + ScaledMax) * 0.5f;
        FVector BoxExtent = (ScaledMax - ScaledMin) * 0.5f;
        
        //FragmentCollider->SetBoxExtent(BoxExtent);
        FragmentCollider->SetRelativeLocation(OriginalBoxLocation); 

        // 파편 물리 속성 설정
        FragmentCollider->bSimulate = true;
        FragmentCollider->RigidBodyType = ERigidBodyType::DYNAMIC;
        FragmentCollider->bApplyGravity = true;
        FragmentCollider-> BodyInstance = new FBodyInstance(FragmentCollider);
        FragmentCollider->BodyInstance->MassInKg = FMath::RandRange(0.5f, 2.0f); // 파편 질량 랜덤화

        // 파편 콜라이더의 GeomAttributes 설정
        AggregateGeomAttributes GeomAttr;
        GeomAttr.GeomType = EGeomType::EBox;
        GeomAttr.Extent = BoxExtent; // SetBoxExtent로 설정한 값
        GeomAttr.Offset = BoxCenter; // 로컬 오프셋
        FragmentCollider->GeomAttributes.Add(GeomAttr);
        
        FragmentCollider->CreatePhysXGameObject(); // PhysX 객체 생성

        // PhysX 객체에 힘 적용
        if (FragmentCollider->BodyInstance && FragmentCollider->BodyInstance->BIGameObject && FragmentCollider->BodyInstance->BIGameObject->DynamicRigidBody)
        {
            PxRigidDynamic* PhysXBody = FragmentCollider->BodyInstance->BIGameObject->DynamicRigidBody;

            // 충격 지점에서 파편 위치로 향하는 방향 + 기본 충격 방향 혼합
            FVector Location = FragMeshComp->GetComponentLocation() + GeomAttr.Offset ;
            FVector ScatterDirection = (Location - HitLocation).GetSafeNormal();
            // if (ScatterDirection.IsNearlyZero() || FVector::DotProduct(ScatterDirection, ImpulseDirection) < 0.2f) // 너무 가깝거나 방향이 많이 다르면
            // {
            //     ScatterDirection = ImpulseDirection; // 기본 충격 방향 사용
            // }
            // 약간의 랜덤성 추가
            ScatterDirection += FVector(FMath::RandRange(-0.7f, 0.7f), FMath::RandRange(-0.7f, 0.7f), FMath::RandRange(0.1f, 0.8f));
            ScatterDirection.Normalize();
            
            float FragmentImpulseStrength = ImpulseMagnitude * FMath::RandRange(0.1f, 0.5f) * StrengthMultiplier; // 원래 충격량의 일부 + 랜덤
            PxVec3 PxForce = PxVec3(ScatterDirection.X, ScatterDirection.Y, ScatterDirection.Z) * FragmentImpulseStrength ;
            
            PhysXBody->addForce(PxForce, PxForceMode::eIMPULSE);
            PxRigidBodyExt::addForceAtPos(*PhysXBody, PxForce, PxVec3(HitLocation.X, HitLocation.Y, HitLocation.Z), PxForceMode::eIMPULSE);


            // 회전력 추가 (토크)
            PxVec3 PxTorque = PxVec3(FMath::RandRange(-50.f, 50.f), FMath::RandRange(-50.f, 50.f), FMath::RandRange(-50.f, 50.f));
            PhysXBody->addTorque(PxTorque, PxForceMode::eIMPULSE);
        }

        // 파편 액터 자동 소멸 (예: 5초 후)
        //FragmentActor->SetLifeSpan(5.0f + FMath::RandRange(0.f, 3.f));
    }


    Destroy(); 
}
