#include "CarActor.h"
#include <Camera/CameraComponent.h>
#include <Engine/Engine.h>
#include "World/World.h"
#include <GameFramework/SpringArmComponent.h>

ACarActor::ACarActor()
{
    UCarComponent* Car = AddComponent<UCarComponent>("Car");
    Camera = AddComponent<UCameraComponent>("Camera");
    Camera->SetRelativeLocation(FVector(1, 0, 0) * -15.f + FVector(0, 0, 7.5f));
    Camera->AttachToComponent(Car);
}

UObject* ACarActor::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    return NewComponent;
}

void ACarActor::BeginPlay()
{
    if (Camera == nullptr)
    {
        Camera = GetComponentByClass<UCameraComponent>();
    }
    if (Camera)
    {
        GEngine->ActiveWorld->GetPlayerController()->BindAction("R",
            [this](float DeltaTime)
            {
                FViewTargetTransitionParams TransitionParams;
                TransitionParams.BlendTime = 1.f; // 0.5초 동안 부드럽게 전환
                GEngine->ActiveWorld->GetPlayerController()->SetViewTarget(this, TransitionParams);
            });
    }
}
