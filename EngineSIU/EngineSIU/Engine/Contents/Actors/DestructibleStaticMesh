#pragma once
#include "GameFramework/Actor.h"


class UBodySetup;
class UStaticMesh;
class UPrimitiveComponent;
class UStaticMeshComponent;

class ADestructibleStaticMesh : public AActor
{
    DECLARE_CLASS(ADestructibleStaticMesh, AActor)

public:
    ADestructibleStaticMesh();
    virtual ~ADestructibleStaticMesh() override = default;

    UStaticMeshComponent* IntactMeshComponent = nullptr;

    // 물리 및 충돌 감지를 위한 UPrimitiveComponent (예: BoxComponent)
    UPrimitiveComponent* PhysicsColliderComponent  = nullptr; 

    TArray<UStaticMesh*> FragmentMeshes;

    UBodySetup* FragmentBodySetupTemplate; // 파편들이 사용할 BodySetup 템플릿

    

    const float StrengthMultiplier = 3.0f;

    bool bIsDestroyed = false;

    void OnBoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    void TriggerDestruction(FVector HitLocation, FVector ImpulseDirection, float ImpulseMagnitude);
    
protected:
    virtual void BeginPlay() override;

    /**
 * 지정된 기본 이름과 숫자 패턴을 사용하여 파편 스태틱 메시들을 로드합니다.
 * 예: BaseName = "Contents/Meshes/BoxFragment", StartIndex = 1
 * "Contents/Meshes/BoxFragment1", "Contents/Meshes/BoxFragment2", ... 등을 로드 시도합니다.
 * @param BaseName 로드할 메시의 기본 경로 및 이름 (숫자 제외)
 * @param StartIndex 파일 이름에 붙일 시작 숫자
 * @param MaxAttempts 최대 시도 횟수 (무한 루프 방지)
 */
    void LoadFragmentMeshesByNamePattern(const FString& BaseName, int32 StartIndex = 1, int32 MaxAttempts = 100);
    
};
