#include "SideViewCameraActor.h"

#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "World/World.h"

void ASideViewCameraActor::PostSpawnInitialize()
{
    Super::PostSpawnInitialize();

    CameraComponent = AddComponent<UCameraComponent>("UCameraComponent_0");
    SetRootComponent(CameraComponent);
}

UObject* ASideViewCameraActor::Duplicate(UObject* InOuter)
{
    return Super::Duplicate(InOuter);
}

void ASideViewCameraActor::BeginPlay()
{
    Super::BeginPlay();
    if (CameraComponent == nullptr)
    {
        CameraComponent = GetComponentByClass<UCameraComponent>();
    }

    if (CameraComponent)
    {
        GEngine->ActiveWorld->GetPlayerController()->BindAction("Q", 
            [this](float DeltaTime)
            {
                FViewTargetTransitionParams TransitionParams;
                TransitionParams.BlendTime = 1.f; // 0.5초 동안 부드럽게 전환
                GEngine->ActiveWorld->GetPlayerController()->SetViewTarget(this, TransitionParams);
            });
    }
}

void ASideViewCameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ASideViewCameraActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}
