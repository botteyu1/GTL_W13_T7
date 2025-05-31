#include "ObjectViewCameraActor.h"

#include "Engine/Engine.h"
#include "World/World.h"

void AObjectViewCameraActor::PostSpawnInitialize()
{
    AActor::PostSpawnInitialize();
}

UObject* AObjectViewCameraActor::Duplicate(UObject* InOuter)
{
    return AActor::Duplicate(InOuter);
}

void AObjectViewCameraActor::BeginPlay()
{
    AActor::BeginPlay();
    if (CameraComponent == nullptr)
    {
        CameraComponent = GetComponentByClass<UCameraComponent>();
    }

    if (CameraComponent)
    {
        GEngine->ActiveWorld->GetPlayerController()->BindAction("C", 
            [this](float DeltaTime)
            {
                FViewTargetTransitionParams TransitionParams;
                TransitionParams.BlendTime = 1.f; // 0.5초 동안 부드럽게 전환
                GEngine->ActiveWorld->GetPlayerController()->SetViewTarget(this, TransitionParams);
            });
    }
}

void AObjectViewCameraActor::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);
}

void AObjectViewCameraActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    AActor::EndPlay(EndPlayReason);
}
