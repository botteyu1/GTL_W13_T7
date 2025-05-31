#pragma once
#include "GameFramework/Actor.h"


class AObjectViewCameraActor : public AActor
{
    DECLARE_CLASS(AObjectViewCameraActor, AActor)

public:
    AObjectViewCameraActor() = default;
    virtual ~AObjectViewCameraActor() override = default;

public: 
    virtual void PostSpawnInitialize() override;

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void BeginPlay() override;

    virtual void Tick(float DeltaTime) override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


    class UCameraComponent* GetCameraComponent() const
    {
        return CameraComponent;
    }
    
    UCameraComponent* CameraComponent = nullptr;
};
