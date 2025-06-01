#include "FreeCameraActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "LevelEditor/SLevelEditor.h"
#include "UnrealEd/EditorViewportClient.h"
#include "World/World.h"

void AFreeCameraActor::PostSpawnInitialize()
{
    Super::PostSpawnInitialize();
    CameraComponent = AddComponent<UCameraComponent>("UCameraComponent_0");
    SetRootComponent(CameraComponent);
}

UObject* AFreeCameraActor::Duplicate(UObject* InOuter)
{
    return Super::Duplicate(InOuter);
}

void AFreeCameraActor::BeginPlay()
{
    Super::BeginPlay();
    if (CameraComponent == nullptr)
    {
        CameraComponent = GetComponentByClass<UCameraComponent>();
    }

    if (CameraComponent)
    {
        APlayerController* PlayerController = GEngine->ActiveWorld->GetPlayerController();
        
        PlayerController->BindAction("One", 
            [this](float DeltaTime)
            {
                APlayerController* PlayerController = GEngine->ActiveWorld->GetPlayerController();
                FViewTargetTransitionParams TransitionParams;
                TransitionParams.BlendTime = 0.f; // 0.5초 동안 부드럽게 전환
                PlayerController->SetViewTarget(this, TransitionParams);
            });
        PlayerController->BindAction("W", 
            [this](float DeltaTime)
            {
                APlayerController* PlayerController = GEngine->ActiveWorld->GetPlayerController();
                if (PlayerController->PlayerCameraManager->IsViewTargetActor(this))
                {
                    CameraMoveForward(DeltaTime);
                }
            });
        PlayerController->BindAction("D", 
            [this](float DeltaTime)
            {
                if (GEngine->ActiveWorld->GetPlayerController()->PlayerCameraManager->IsViewTargetActor(this))
                {
                    CameraMoveRight(DeltaTime);
                }
            });
        PlayerController->BindAction("S", 
            [this](float DeltaTime)
            {
                if (GEngine->ActiveWorld->GetPlayerController()->PlayerCameraManager->IsViewTargetActor(this))
                {
                    CameraMoveForward(-DeltaTime);
                }
            });
        PlayerController->BindAction("A", 
            [this](float DeltaTime)
            {
                if (GEngine->ActiveWorld->GetPlayerController()->PlayerCameraManager->IsViewTargetActor(this))
                {
                    CameraMoveRight(-DeltaTime);
                }
            });
        PlayerController->BindAction("E", 
            [this](float DeltaTime)
            {
                if (GEngine->ActiveWorld->GetPlayerController()->PlayerCameraManager->IsViewTargetActor(this))
                {
                    CameraMoveUp(DeltaTime);
                }
            });
        PlayerController->BindAction("Q", 
            [this](float DeltaTime)
            {
                if (GEngine->ActiveWorld->GetPlayerController()->PlayerCameraManager->IsViewTargetActor(this))
                {
                    CameraMoveUp(-DeltaTime);
                }
            });
        PlayerController->BindMouseAction("MouseRightMove", 
            [this](const FPointerEvent& InMouseEvent)
            {
                if (GEngine->ActiveWorld->GetPlayerController()->PlayerCameraManager->IsViewTargetActor(this))
                {
                    MouseMove(InMouseEvent);
                }
            });
    }
}

void AFreeCameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AFreeCameraActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void AFreeCameraActor::CameraMoveForward(const float InValue)
{
    std::shared_ptr<FEditorViewportClient> ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();
    
    FVector CurCameraLoc = GetActorLocation();
    CurCameraLoc = CurCameraLoc + GetActorForwardVector() * ActiveViewport->CameraSpeedMultiplier * ActiveViewport->GetCameraSpeedScalar() * InValue;
    SetActorLocation(CurCameraLoc);
}

void AFreeCameraActor::CameraMoveRight(const float InValue)
{
    std::shared_ptr<FEditorViewportClient> ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();
    
    FVector CurCameraLoc = GetActorLocation();
    CurCameraLoc = CurCameraLoc + GetActorRightVector() * ActiveViewport->CameraSpeedMultiplier * ActiveViewport->GetCameraSpeedScalar() * InValue;
    SetActorLocation(CurCameraLoc);
}

void AFreeCameraActor::CameraMoveUp(const float InValue)
{
    std::shared_ptr<FEditorViewportClient> ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();
    
    FVector CurCameraLoc = GetActorLocation();
    CurCameraLoc = CurCameraLoc + GetActorUpVector() * ActiveViewport->CameraSpeedMultiplier * ActiveViewport->GetCameraSpeedScalar() * InValue;
    SetActorLocation(CurCameraLoc);
}

void AFreeCameraActor::CameraRotateYaw(float InValue)
{
    FRotator CurCameraRot = GetActorRotation();
    CurCameraRot.Yaw += InValue;
    SetActorRotation(CurCameraRot);
}

void AFreeCameraActor::CameraRotatePitch(float InValue)
{
    FRotator CurCameraRot = GetActorRotation();
    CurCameraRot.Pitch = FMath::Clamp(CurCameraRot.Pitch - InValue, -89.f, 89.f);;
    SetActorRotation(CurCameraRot);
}

void AFreeCameraActor::MouseMove(const FPointerEvent& InMouseEvent)
{
    APlayerController* PlayerController = GEngine->ActiveWorld->GetPlayerController();
    if (PlayerController->PlayerCameraManager->IsViewTargetActor(this))
    {
        const auto& [DeltaX, DeltaY] = InMouseEvent.GetCursorDelta();
        {
            CameraRotateYaw(DeltaX * 0.1f);  // X 이동에 따라 좌우 회전
            CameraRotatePitch(DeltaY * 0.1f);  // Y 이동에 따라 상하 회전
        }
    }
}
