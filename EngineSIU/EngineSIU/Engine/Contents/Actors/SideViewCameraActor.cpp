#include "SideViewCameraActor.h"

#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "World/World.h"

void ASideViewCameraActor::PostSpawnInitialize()
{
    AActor::PostSpawnInitialize();

    CameraComponent = AddComponent<UCameraComponent>("UCameraComponent_0");
    SetRootComponent(CameraComponent);
}

UObject* ASideViewCameraActor::Duplicate(UObject* InOuter)
{
    return AActor::Duplicate(InOuter);
}

void ASideViewCameraActor::BeginPlay()
{
    AActor::BeginPlay();
    if (CameraComponent == nullptr)
    {
        CameraComponent = GetComponentByClass<UCameraComponent>();
    }

    if (CameraComponent)
    {
        GEngine->ActiveWorld->GetPlayerController()->BindAction("Z", 
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
    AActor::Tick(DeltaTime);
}

void ASideViewCameraActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    AActor::EndPlay(EndPlayReason);
}
