#pragma once
#include "GameFramework/Actor.h"




class ASideViewCameraActor : public AActor
{
    DECLARE_CLASS(ASideViewCameraActor, AActor)

public:
    ASideViewCameraActor() = default;
    virtual ~ASideViewCameraActor() override = default;

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
