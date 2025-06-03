#include "SLevelEditor.h"
#include <fstream>
#include <ostream>
#include <sstream>
#include "EngineLoop.h"
#include "UnrealClient.h"
#include "WindowsCursor.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "Engine/EditorEngine.h"
#include "Slate/Widgets/Layout/SSplitter.h"
#include "SlateCore/Widgets/SWindow.h"
#include "UnrealEd/EditorViewportClient.h"

extern FEngineLoop GEngineLoop;


SLevelEditor::SLevelEditor()
    : HSplitter(nullptr)
    , VSplitter(nullptr)
    , bMultiViewportMode(false)
{
}

void SLevelEditor::Initialize(uint32 InEditorWidth, uint32 InEditorHeight)
{
    EditorWidth = InEditorWidth * 0.8f;
    EditorHeight = InEditorHeight - 104.f;
    
    ResizeEditor(EditorWidth, EditorHeight);
    
    VSplitter = new SSplitterV();
    VSplitter->Initialize(FRect(0.0f, 0.f, EditorWidth, EditorHeight));
    
    HSplitter = new SSplitterH();
    HSplitter->Initialize(FRect(0.f, 0.0f, EditorWidth, EditorHeight));
    
    FRect Top = VSplitter->SideLT->GetRect();
    FRect Bottom = VSplitter->SideRB->GetRect();
    FRect Left = HSplitter->SideLT->GetRect();
    FRect Right = HSplitter->SideRB->GetRect();

    for (size_t i = 0; i < 4; i++)
    {
        EViewScreenLocation Location = static_cast<EViewScreenLocation>(i);
        FRect Rect;
        switch (Location)
        {
        case EViewScreenLocation::EVL_TopLeft:
            Rect.TopLeftX = Left.TopLeftX;
            Rect.TopLeftY = Top.TopLeftY;
            Rect.Width = Left.Width;
            Rect.Height = Top.Height;
            break;
        case EViewScreenLocation::EVL_TopRight:
            Rect.TopLeftX = Right.TopLeftX;
            Rect.TopLeftY = Top.TopLeftY;
            Rect.Width = Right.Width;
            Rect.Height = Top.Height;
            break;
        case EViewScreenLocation::EVL_BottomLeft:
            Rect.TopLeftX = Left.TopLeftX;
            Rect.TopLeftY = Bottom.TopLeftY;
            Rect.Width = Left.Width;
            Rect.Height = Bottom.Height;
            break;
        case EViewScreenLocation::EVL_BottomRight:
            Rect.TopLeftX = Right.TopLeftX;
            Rect.TopLeftY = Bottom.TopLeftY;
            Rect.Width = Right.Width;
            Rect.Height = Bottom.Height;
            break;
        default:
            return;
        }
        ViewportClients[i] = std::make_shared<FEditorViewportClient>();
        ViewportClients[i]->Initialize(Location, Rect);
    }
    
    ActiveViewportClient = ViewportClients[0];
    
    LoadConfig();

    FSlateAppMessageHandler* Handler = GEngineLoop.GetAppMessageHandler();

    Handler->OnPIEModeStartDelegate.AddLambda([this]()
        {
            this->RegisterPIEInputDelegates();
        });

    Handler->OnPIEModeEndDelegate.AddLambda([this]()
        {
            this->RegisterEditorInputDelegates();
        });

    // Register Editor input when first initialization. 
    RegisterEditorInputDelegates();
}

void SLevelEditor::Tick(float DeltaTime)
{
    for (std::shared_ptr<FEditorViewportClient> Viewport : ViewportClients)
    {
        Viewport->Tick(DeltaTime);
    }
}

void SLevelEditor::Release()
{
    delete VSplitter;
    delete HSplitter;
}

void SLevelEditor::ResizeEditor(uint32 InEditorWidth, uint32 InEditorHeight)
{
    if (InEditorWidth == EditorWidth && InEditorHeight == EditorHeight)
    {
        return;
    }

    if (GEngineLoop.bPendingGame)
    {
        EditorWidth = InEditorWidth;
        EditorHeight = InEditorHeight;
    }
    else
    {
        EditorWidth = InEditorWidth * 0.8f;
        EditorHeight = InEditorHeight - 104.f;
    }

    if (HSplitter && VSplitter)
    {
        HSplitter->OnResize(EditorWidth, EditorHeight);
        VSplitter->OnResize(EditorWidth, EditorHeight);
        ResizeViewports();
    }
}

void SLevelEditor::SelectViewport(const FVector2D& Point)
{
    for (int i = 0; i < 4; i++)
    {
        if (ViewportClients[i]->IsSelected(Point))
        {
            SetActiveViewportClient(i);
            return;
        }
    }
}

void SLevelEditor::ResizeViewports()
{
    if (bMultiViewportMode)
    {
        if (GetViewports()[0])
        {
            for (int i = 0; i < 4; ++i)
            {
                GetViewports()[i]->ResizeViewport(
                    VSplitter->SideLT->GetRect(),
                    VSplitter->SideRB->GetRect(),
                    HSplitter->SideLT->GetRect(),
                    HSplitter->SideRB->GetRect()
                );
            }
        }
    }
    else
    {
        float y = GEngineLoop.bPendingGame ? 0.0f : 72.f;
        ActiveViewportClient->GetViewport()->ResizeViewport(FRect(0.0f, y, EditorWidth , EditorHeight ));
    }
}

void SLevelEditor::SetEnableMultiViewport(bool bIsEnable)
{
    bMultiViewportMode = bIsEnable;
    ResizeViewports();
}

bool SLevelEditor::IsMultiViewport() const
{
    return bMultiViewportMode;
}

void SLevelEditor::LoadConfig()
{
    auto Config = ReadIniFile(IniFilePath);

    int32 WindowX = FMath::Max(GetValueFromConfig(Config, "WindowX", 0), 0);
    int32 WindowY = FMath::Max(GetValueFromConfig(Config, "WindowY", 0), 0);
    int32 WindowWidth = GetValueFromConfig(Config, "WindowWidth", EditorWidth);
    int32 WindowHeight = GetValueFromConfig(Config, "WindowHeight", EditorHeight);
    if (WindowWidth > 100 && WindowHeight > 100)
    {
        MoveWindow(GEngineLoop.AppWnd, WindowX, WindowY, WindowWidth, WindowHeight, true);
    }
    bool Zoomed = GetValueFromConfig(Config, "Zoomed", false);
    if (Zoomed)
    {
        ShowWindow(GEngineLoop.AppWnd, SW_MAXIMIZE);
    }
    
    FEditorViewportClient::Pivot.X = GetValueFromConfig(Config, "OrthoPivotX", 0.0f);
    FEditorViewportClient::Pivot.Y = GetValueFromConfig(Config, "OrthoPivotY", 0.0f);
    FEditorViewportClient::Pivot.Z = GetValueFromConfig(Config, "OrthoPivotZ", 0.0f);
    FEditorViewportClient::OrthoSize = GetValueFromConfig(Config, "OrthoZoomSize", 10.0f);

    SetActiveViewportClient(GetValueFromConfig(Config, "ActiveViewportIndex", 0));
    bMultiViewportMode = GetValueFromConfig(Config, "bMultiView", false);
    if (bMultiViewportMode)
    {
        SetEnableMultiViewport(true);
    }
    else
    {
        SetEnableMultiViewport(false);
    }
    
    for (size_t i = 0; i < 4; i++)
    {
        ViewportClients[i]->LoadConfig(Config);
    }
    
    if (HSplitter)
    {
        HSplitter->LoadConfig(Config);
    }
    if (VSplitter)
    {
        VSplitter->LoadConfig(Config);
    }

    ResizeViewports();
}

void SLevelEditor::SaveConfig()
{
    TMap<FString, FString> config;
    if (HSplitter)
    {
        HSplitter->SaveConfig(config);
    }
    if (VSplitter)
    {
        VSplitter->SaveConfig(config);
    }
    for (size_t i = 0; i < 4; i++)
    {
        ViewportClients[i]->SaveConfig(config);
    }
    ActiveViewportClient->SaveConfig(config);

    RECT WndRect = {};
    GetWindowRect(GEngineLoop.AppWnd, &WndRect);
    config["WindowX"] = std::to_string(WndRect.left);
    config["WindowY"] = std::to_string(WndRect.top);
    config["WindowWidth"] = std::to_string(WndRect.right - WndRect.left);
    config["WindowHeight"] = std::to_string(WndRect.bottom - WndRect.top);
    config["Zoomed"] = std::to_string(IsZoomed(GEngineLoop.AppWnd));
    
    config["bMultiView"] = std::to_string(bMultiViewportMode);
    config["ActiveViewportIndex"] = std::to_string(ActiveViewportClient->ViewportIndex);
    config["ScreenWidth"] = std::to_string(EditorWidth);
    config["ScreenHeight"] = std::to_string(EditorHeight);
    config["OrthoPivotX"] = std::to_string(ActiveViewportClient->Pivot.X);
    config["OrthoPivotY"] = std::to_string(ActiveViewportClient->Pivot.Y);
    config["OrthoPivotZ"] = std::to_string(ActiveViewportClient->Pivot.Z);
    config["OrthoZoomSize"] = std::to_string(ActiveViewportClient->OrthoSize);
    WriteIniFile(IniFilePath, config);
}

TMap<FString, FString> SLevelEditor::ReadIniFile(const FString& FilePath)
{
    TMap<FString, FString> config;
    std::ifstream file(*FilePath);
    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '[' || line[0] == ';')
        {
            continue;
        }
        std::istringstream ss(line);
        std::string key, value;
        if (std::getline(ss, key, '=') && std::getline(ss, value))
        {
            config[key] = value;
        }
    }
    return config;
}

void SLevelEditor::WriteIniFile(const FString& FilePath, const TMap<FString, FString>& Config)
{
    std::ofstream file(*FilePath);
    for (const auto& pair : Config)
    {
        file << *pair.Key << "=" << *pair.Value << "\n";
    }
}

void SLevelEditor::RegisterEditorInputDelegates() 
{
    FSlateAppMessageHandler* Handler = GEngineLoop.GetAppMessageHandler();
    
    // Clear current delegate functions
    for (const FDelegateHandle& Handle : InputDelegatesHandles)
    {
        Handler->OnKeyCharDelegate.Remove(Handle);
        Handler->OnKeyDownDelegate.Remove(Handle);
        Handler->OnKeyUpDelegate.Remove(Handle);
        Handler->OnMouseDownDelegate.Remove(Handle);
        Handler->OnMouseUpDelegate.Remove(Handle);
        Handler->OnMouseDoubleClickDelegate.Remove(Handle);
        Handler->OnMouseWheelDelegate.Remove(Handle);
        Handler->OnMouseMoveDelegate.Remove(Handle);
        Handler->OnRawMouseInputDelegate.Remove(Handle);
        Handler->OnRawKeyboardInputDelegate.Remove(Handle);
    }

    InputDelegatesHandles.Add(Handler->OnMouseDownDelegate.AddLambda([this](const FPointerEvent& InMouseEvent)
        {
            if (ImGui::GetIO().WantCaptureMouse) return;

            switch (InMouseEvent.GetEffectingButton())  // NOLINT(clang-diagnostic-switch-enum)
            {
            case EKeys::LeftMouseButton:
            {
                if (const UEditorEngine* EdEngine = Cast<UEditorEngine>(GEngine))
                {
                    if (!EdEngine->GetSelectedActors().IsEmpty())
                    {
                        USceneComponent* TargetComponent = EdEngine->GetSelectedComponent();
                        
                        if (TargetComponent == nullptr)
                        {
                            TargetComponent = EdEngine->GetSelectedActor()->GetRootComponent();
                        }

                        if (TargetComponent)
                        {
                            // 초기 Actor와 Cursor의 거리차를 저장
                            const FViewportCamera* ViewTransform = ActiveViewportClient->GetViewportType() == LVT_Perspective
                                                                ? &ActiveViewportClient->PerspectiveCamera
                                                                : &ActiveViewportClient->OrthogonalCamera;
                            
                            FVector RayOrigin, RayDir;
                            ActiveViewportClient->DeprojectFVector2D(FWindowsCursor::GetClientPosition(), RayOrigin, RayDir);
                            
                            const FVector TargetLocation = TargetComponent->GetComponentLocation();
                            ActorStartLocation = TargetLocation;
                            const float TargetDist = FVector::Distance(ViewTransform->GetLocation(), TargetLocation);
                            const FVector TargetRayEnd = RayOrigin + RayDir * TargetDist;
                            TargetDiff = TargetLocation - TargetRayEnd;

                            InitialActorLocations.Empty();
                            for (AActor* SelectedActor : EdEngine->GetSelectedActors())
                            {
                                if (SelectedActor && SelectedActor->GetRootComponent())
                                {
                                    InitialActorLocations.Add(SelectedActor->GetActorLocation());
                                }
                                else
                                {
                                    InitialActorLocations.Add(FVector::ZeroVector);
                                }
                            }
                        }
                    }
                }
                break;
            }
            case EKeys::RightMouseButton:
            {
                if (!InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
                {
                    FWindowsCursor::SetShowMouseCursor(false);
                    MousePinPosition = InMouseEvent.GetScreenSpacePosition();
                }
                break;
            }
            default:
                break;
            }

            // 마우스 이벤트가 일어난 위치의 뷰포트를 선택
            if (bMultiViewportMode)
            {
                POINT Point;
                GetCursorPos(&Point);
                ScreenToClient(GEngineLoop.AppWnd, &Point);
                FVector2D ClientPos = FVector2D{ static_cast<float>(Point.x), static_cast<float>(Point.y) };
                SelectViewport(ClientPos);
                VSplitter->OnPressed({ ClientPos.X, ClientPos.Y });
                HSplitter->OnPressed({ ClientPos.X, ClientPos.Y });
            }
        }));

    InputDelegatesHandles.Add(Handler->OnMouseMoveDelegate.AddLambda([this](const FPointerEvent& InMouseEvent)
        {
            if (ImGui::GetIO().WantCaptureMouse) return;

            // Splitter 움직임 로직
            if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
            {
                const auto& [DeltaX, DeltaY] = InMouseEvent.GetCursorDelta();

                bool bSplitterDragging = false;
                if (VSplitter->IsSplitterPressed())
                {
                    VSplitter->OnDrag(FPoint(DeltaX, DeltaY));
                    bSplitterDragging = true;
                }
                if (HSplitter->IsSplitterPressed())
                {
                    HSplitter->OnDrag(FPoint(DeltaX, DeltaY));
                    bSplitterDragging = true;
                }

                if (bSplitterDragging)
                {
                    ResizeViewports();
                }
            }

            // 멀티 뷰포트일 때, 커서 변경 로직
            if (
                bMultiViewportMode
                && !InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)
                && !InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton)
                )
            {
                // TODO: 나중에 커서가 Viewport 위에 있을때만 ECursorType::Crosshair로 바꾸게끔 하기
                // ECursorType CursorType = ECursorType::Crosshair;
                ECursorType CursorType = ECursorType::Arrow;
                POINT Point;

                GetCursorPos(&Point);
                ScreenToClient(GEngineLoop.AppWnd, &Point);
                FVector2D ClientPos = FVector2D{ static_cast<float>(Point.x), static_cast<float>(Point.y) };
                const bool bIsVerticalHovered = VSplitter->IsSplitterHovered({ ClientPos.X, ClientPos.Y });
                const bool bIsHorizontalHovered = HSplitter->IsSplitterHovered({ ClientPos.X, ClientPos.Y });

                if (bIsHorizontalHovered && bIsVerticalHovered)
                {
                    CursorType = ECursorType::ResizeAll;
                }
                else if (bIsHorizontalHovered)
                {
                    CursorType = ECursorType::ResizeLeftRight;
                }
                else if (bIsVerticalHovered)
                {
                    CursorType = ECursorType::ResizeUpDown;
                }
                FWindowsCursor::SetMouseCursor(CursorType);
            }
        }));

    InputDelegatesHandles.Add(Handler->OnMouseUpDelegate.AddLambda([this](const FPointerEvent& InMouseEvent)
        {
            switch (InMouseEvent.GetEffectingButton())  // NOLINT(clang-diagnostic-switch-enum)
            {
            case EKeys::RightMouseButton:
            {
                FWindowsCursor::SetShowMouseCursor(true);
                FWindowsCursor::SetPosition(
                    static_cast<int32>(MousePinPosition.X),
                    static_cast<int32>(MousePinPosition.Y)
                );
                return;
            }

            // Viewport 선택 로직
            case EKeys::LeftMouseButton:
            {
                bPressedAlt = false;
                VSplitter->OnReleased();
                HSplitter->OnReleased();
                return;
            }

            default:
                return;
            }
        }));

    InputDelegatesHandles.Add(Handler->OnRawMouseInputDelegate.AddLambda([this](const FPointerEvent& InMouseEvent)
        {
            // Mouse Move 이벤트 일때만 실행
            if (
                InMouseEvent.GetInputEvent() == IE_Axis
                && InMouseEvent.GetEffectingButton() == EKeys::Invalid
                )
            {
                // 에디터 카메라 이동 로직
                if (
                    !InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)
                    && InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton)
                    )
                {
                    ActiveViewportClient->MouseMove(InMouseEvent);
                }

                else if (
                    !InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton)
                    && InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)
                    )
                {
                    // Gizmo control
                    if (UEditorEngine* EdEngine = Cast<UEditorEngine>(GEngine))
                    {
                        if (EdEngine->GetEditorPlayer()->GetControlMode() != CM_TRANSLATION)
                        {
                            return;
                        }
                        
                        const TArray<AActor*> SelectedActors = EdEngine->GetSelectedActors();
                        if (SelectedActors.IsEmpty())
                        {
                            return;
                        }
                        
                        const UGizmoBaseComponent* Gizmo = Cast<UGizmoBaseComponent>(ActiveViewportClient->GetPickedGizmoComponent());
                        if (!Gizmo)
                        {
                            return;
                        }

                        AActor* PrimaryActor = EdEngine->GetPrimarySelectedActor();
                        if (!PrimaryActor || !PrimaryActor->GetRootComponent())
                        {
                            return; // 주 선택 액터 또는 루트 컴포넌트 없으면 진행 불가
                        }
                        
                        USceneComponent* TargetComponent = EdEngine->GetSelectedComponent();

                        const FViewportCamera* ViewTransform = ActiveViewportClient->GetViewportType() == LVT_Perspective
                                                                ? &ActiveViewportClient->PerspectiveCamera
                                                                : &ActiveViewportClient->OrthogonalCamera;

                        FVector RayOrigin, RayDir;
                        ActiveViewportClient->DeprojectFVector2D(FWindowsCursor::GetClientPosition(), RayOrigin, RayDir);

                        const float TargetDist = FVector::Distance(ViewTransform->GetLocation(), ActorStartLocation);
                        const FVector TargetRayEnd = RayOrigin + RayDir * TargetDist;
                        
                        FVector GizmoTargetLocation = TargetRayEnd + TargetDiff;

                        if (ActiveViewportClient->bUseGridMove && !(GetAsyncKeyState(VK_SHIFT) & 0x8000))
                        {
                            float GridScale = ActiveViewportClient->GridMovementScale;
                            
                            FVector RelativeMovement = GizmoTargetLocation - ActorStartLocation;

                            RelativeMovement.X = (roundf(RelativeMovement.X / GridScale) * GridScale) + ActorStartLocation.X;
                            RelativeMovement.Y = (roundf(RelativeMovement.Y / GridScale) * GridScale) + ActorStartLocation.Y;
                            RelativeMovement.Z = (roundf(RelativeMovement.Z / GridScale) * GridScale) + ActorStartLocation.Z;

                            GizmoTargetLocation = RelativeMovement;
                        }

                        FVector MovementDelta = GizmoTargetLocation - ActorStartLocation; // Real movement delta

                        if (!bPressedAlt  && (GetAsyncKeyState(VK_MENU) & 0x8000))
                        {
                            bPressedAlt = true;
                            TArray<AActor*> NewActors;
                            for (AActor* OriginalActor : SelectedActors)
                            {
                                if(OriginalActor)
                                {
                                    AActor* NewActor = EdEngine->ActiveWorld->DuplicateActor(OriginalActor);
                                    NewActors.Add(NewActor);
                                }
                            }
                            EdEngine->ClearSelectedActors(); // 이전 선택 해제
                            InitialActorLocations.Empty();   // 이전 액터들의 초기 위치 정보도 클리어
                            for (AActor* NewActor : NewActors)
                            {
                                EdEngine->SelectActor(NewActor, true); // 복제된 액터들을 선택
                                if (NewActor && NewActor->GetRootComponent())
                                {
                                     InitialActorLocations.Add(NewActor->GetRootComponent()->GetComponentLocation()); // 새 액터들의 현재 위치를 초기 위치로 저장
                                }
                            }
                        }

                        for (int32 i = 0; i < SelectedActors.Num(); ++i)
                        {
                            AActor* CurrentActor = SelectedActors[i];
                            if (!CurrentActor || !CurrentActor->GetRootComponent() || i >= InitialActorLocations.Num() || InitialActorLocations.IsEmpty())
                            {
                                continue;
                            }

                            USceneComponent* NewSceneComponent = CurrentActor->GetRootComponent();
                            FVector InitialLocation = InitialActorLocations[i];

                            FVector NewLocation = NewSceneComponent->GetComponentLocation();
                            NewLocation = InitialLocation;

                            if (EdEngine->GetEditorPlayer()->GetCoordMode() == CDM_WORLD)
                            {
                                FVector AxisFilteredMovementDelta = FVector::ZeroVector;
                                if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowX)
                                {
                                    AxisFilteredMovementDelta.X = MovementDelta.X;
                                }
                                else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowY)
                                {
                                    AxisFilteredMovementDelta.Y = MovementDelta.Y;
                                }
                                else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowZ)
                                {
                                    AxisFilteredMovementDelta.Z = MovementDelta.Z;
                                }
                                else
                                {
                                    AxisFilteredMovementDelta = MovementDelta;
                                }
                                NewLocation += AxisFilteredMovementDelta;
                            }
                            else
                            {
                                const FTransform PrimaryActorTransform = TargetComponent->GetComponentTransform();
                                FVector LocalMovementDelta = FVector::ZeroVector;

                                if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowX)
                                {
                                    // MovementDelta 를 PrimaryActor의 X축에 투영
                                    float DotP = FVector::DotProduct(MovementDelta, PrimaryActorTransform.GetUnitAxis(EAxis::X));
                                    LocalMovementDelta = PrimaryActorTransform.GetUnitAxis(EAxis::X) * DotP;
                                }
                                else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowY)
                                {
                                    float DotP = FVector::DotProduct(MovementDelta, PrimaryActorTransform.GetUnitAxis(EAxis::Y));
                                    LocalMovementDelta = PrimaryActorTransform.GetUnitAxis(EAxis::Y) * DotP;
                                }
                                else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowZ)
                                {
                                    float DotP = FVector::DotProduct(MovementDelta, PrimaryActorTransform.GetUnitAxis(EAxis::Z));
                                    LocalMovementDelta = PrimaryActorTransform.GetUnitAxis(EAxis::Z) * DotP;
                                }
                                 else
                                 {
                                     LocalMovementDelta = MovementDelta; // 전체 이동
                                 }
                                 NewLocation += LocalMovementDelta;
                            }

                            NewSceneComponent->SetWorldLocation(NewLocation);
                        }
                    }
                }
            }

            // 마우스 휠 이벤트
            else if (InMouseEvent.GetEffectingButton() == EKeys::MouseWheelAxis)
            {
                // 카메라 속도 조절
                if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton) && ActiveViewportClient->IsPerspective())
                {
                    const float CurrentSpeed = ActiveViewportClient->GetCameraSpeedScalar();
                    const float Adjustment = FMath::Sign(InMouseEvent.GetWheelDelta()) * FMath::Loge(CurrentSpeed + 1.0f) * 0.5f;

                    ActiveViewportClient->SetCameraSpeed(CurrentSpeed + Adjustment);
                }
            }
        }));

    InputDelegatesHandles.Add(Handler->OnMouseWheelDelegate.AddLambda([this](const FPointerEvent& InMouseEvent)
        {
            if (ImGui::GetIO().WantCaptureMouse) return;

            // 뷰포트에서 앞뒤 방향으로 화면 이동
            if (ActiveViewportClient->IsPerspective())
            {
                if (!InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
                {
                    const FVector CameraLoc = ActiveViewportClient->PerspectiveCamera.GetLocation();
                    const FVector CameraForward = ActiveViewportClient->PerspectiveCamera.GetForwardVector();
                    ActiveViewportClient->PerspectiveCamera.SetLocation(
                        CameraLoc + CameraForward * InMouseEvent.GetWheelDelta() * 50.0f
                    );
                }
            }
            else
            {
                FEditorViewportClient::SetOthoSize(-InMouseEvent.GetWheelDelta());
            }
        }));

    InputDelegatesHandles.Add(Handler->OnKeyDownDelegate.AddLambda([this](const FKeyEvent& InKeyEvent)
        {
            ActiveViewportClient->InputKey(InKeyEvent);
        }));

    InputDelegatesHandles.Add(Handler->OnKeyUpDelegate.AddLambda([this](const FKeyEvent& InKeyEvent)
        {
            ActiveViewportClient->InputKey(InKeyEvent);
        }));
}

void SLevelEditor::RegisterPIEInputDelegates()
{
    FSlateAppMessageHandler* Handler = GEngineLoop.GetAppMessageHandler();

    // 이거 주석 꼭 풀어야 함.
    // Clear current delegate functions
    for (const FDelegateHandle& Handle : InputDelegatesHandles)
    {
        Handler->OnKeyCharDelegate.Remove(Handle);
        Handler->OnKeyDownDelegate.Remove(Handle);
        Handler->OnKeyUpDelegate.Remove(Handle);
        Handler->OnMouseDownDelegate.Remove(Handle);
        Handler->OnMouseUpDelegate.Remove(Handle);
        Handler->OnMouseDoubleClickDelegate.Remove(Handle);
        Handler->OnMouseWheelDelegate.Remove(Handle);
        Handler->OnMouseMoveDelegate.Remove(Handle);
        Handler->OnRawMouseInputDelegate.Remove(Handle);
        Handler->OnRawKeyboardInputDelegate.Remove(Handle);
    }
    // Add Delegate functions in PIE mode
}
