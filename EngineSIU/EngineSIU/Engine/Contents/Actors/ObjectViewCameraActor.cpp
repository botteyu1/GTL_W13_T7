#include "ObjectViewCameraActor.h"

#include "Engine/Engine.h"
#include "World/World.h"

void AObjectViewCameraActor::PostSpawnInitialize()
{
    Super::PostSpawnInitialize();
    CameraComponent = AddComponent<UCameraComponent>("UCameraComponent_0");
    SetRootComponent(CameraComponent);
}

UObject* AObjectViewCameraActor::Duplicate(UObject* InOuter)
{
    return Super::Duplicate(InOuter);
}

void AObjectViewCameraActor::BeginPlay()
{
    Super::BeginPlay();
    if (CameraComponent == nullptr)
    {
        CameraComponent = GetComponentByClass<UCameraComponent>();
    }

    if (CameraComponent)
    {
        GEngine->ActiveWorld->GetPlayerController()->BindAction("Four", 
            [this](float DeltaTime)
            {
                FViewTargetTransitionParams TransitionParams;
                TransitionParams.BlendTime = 0.f; // 0.5초 동안 부드럽게 전환
                GEngine->ActiveWorld->GetPlayerController()->SetViewTarget(this, TransitionParams);
            });
    }
}

void AObjectViewCameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AObjectViewCameraActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}
