#pragma once
#include "CameraActor.h"
#include "GameFramework/Actor.h"


class AFreeCameraActor : public ACameraActor
{
    DECLARE_CLASS(AFreeCameraActor, ACameraActor)

public:
    AFreeCameraActor() = default;
    virtual ~AFreeCameraActor() override = default;

public: 
    virtual void PostSpawnInitialize() override;

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void BeginPlay() override;

    virtual void Tick(float DeltaTime) override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


    //기능들 CameraMoveComponent로 뺴는게 좋을 듯
    void CameraMoveForward(const float InValue);

    void CameraMoveRight(const float InValue);
    void CameraMoveUp(const float InValue);

    void CameraRotateYaw(float InValue);
    void CameraRotatePitch(float InValue);
    void MouseMove(const FPointerEvent& InMouseEvent);



    
};



