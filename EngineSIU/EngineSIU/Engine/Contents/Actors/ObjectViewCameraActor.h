#pragma once
#include "CameraActor.h"
#include "GameFramework/Actor.h"


class AObjectViewCameraActor : public ACameraActor
{
    DECLARE_CLASS(AObjectViewCameraActor, ACameraActor)

public:
    AObjectViewCameraActor() = default;
    virtual ~AObjectViewCameraActor() override = default;

public: 
    virtual void PostSpawnInitialize() override;

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void BeginPlay() override;

    virtual void Tick(float DeltaTime) override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;



};
