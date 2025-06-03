#include "InputComponent.h"

void UInputComponent::ProcessInput(float DeltaTime)
{
    if (PressedKeys.Contains(EKeys::W))
    {
        KeyBindDelegate[FString("W")].Broadcast(DeltaTime);
    }
    if (PressedKeys.Contains(EKeys::A))
    {
        KeyBindDelegate[FString("A")].Broadcast(DeltaTime);
    }
    if (PressedKeys.Contains(EKeys::S))
    {
        KeyBindDelegate[FString("S")].Broadcast(DeltaTime);
    }
    if (PressedKeys.Contains(EKeys::D))
    {
        KeyBindDelegate[FString("D")].Broadcast(DeltaTime);
    }
    if (PressedKeys.Contains(EKeys::R))
    {
        KeyBindDelegate[FString("R")].Broadcast(DeltaTime);
    }
    if (PressedKeys.Contains(EKeys::Z))
    {
        KeyBindDelegate[FString("Z")].Broadcast(DeltaTime);
    }
    if (PressedKeys.Contains(EKeys::X))
    {
        KeyBindDelegate[FString("X")].Broadcast(DeltaTime);
    }
    if (PressedKeys.Contains(EKeys::X))
    {
        KeyBindDelegate[FString("C")].Broadcast(DeltaTime);
    }
    if (PressedKeys.Contains(EKeys::Q))
    {
        KeyBindDelegate[FString("Q")].Broadcast(DeltaTime);
    }
    if (PressedKeys.Contains(EKeys::E))
    {
        KeyBindDelegate[FString("E")].Broadcast(DeltaTime);
    }
    if (PressedKeys.Contains(EKeys::Y))
    {
        KeyBindDelegate[FString("Y")].Broadcast(DeltaTime);
    }
    if (PressedKeys.Contains(EKeys::One))
    {
        KeyBindDelegate[FString("One")].Broadcast(DeltaTime);
    }
    if (PressedKeys.Contains(EKeys::Two))
    {
        KeyBindDelegate[FString("Two")].Broadcast(DeltaTime);
    }
    if (PressedKeys.Contains(EKeys::Three))
    {
        KeyBindDelegate[FString("Three")].Broadcast(DeltaTime);
    }
    if (PressedKeys.Contains(EKeys::Four))
    {
        KeyBindDelegate[FString("Four")].Broadcast(DeltaTime);
    }
    if (PressedKeys.Contains(EKeys::Five))
    {
        KeyBindDelegate[FString("Five")].Broadcast(DeltaTime);
    }
}

void UInputComponent::SetPossess()
{
    BindInputDelegate();
    
    //TODO: Possess일때 기존에 있던거 다시 넣어줘야할수도
}

void UInputComponent::BindInputDelegate()
{
    FSlateAppMessageHandler* Handler = GEngineLoop.GetAppMessageHandler();

    BindKeyDownDelegateHandles.Add(Handler->OnKeyDownDelegate.AddLambda([this](const FKeyEvent& InKeyEvent)
    {
        InputKey(InKeyEvent);
    }));

    BindKeyUpDelegateHandles.Add(Handler->OnKeyUpDelegate.AddLambda([this](const FKeyEvent& InKeyEvent)
    {
        InputKey(InKeyEvent);
    }));
    BindMouseDelegateHandles.Add(Handler->OnRawMouseInputDelegate.AddLambda([this](const FPointerEvent& InMouseEvent)
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
                MouseBindDelegate[FString("MouseRightMove")].Broadcast(InMouseEvent);
            }
            else if (
                !InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton)
                && InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)
                )
            {
                MouseBindDelegate[FString("MouseLeftMove")].Broadcast(InMouseEvent);
            }
        }
    }));
    
}

void UInputComponent::UnPossess()
{ 
    ClearBindDelegate();
}

void UInputComponent::ClearBindDelegate()
{
    FSlateAppMessageHandler* Handler = GEngineLoop.GetAppMessageHandler();

    for (FDelegateHandle DelegateHandle : BindKeyDownDelegateHandles)
    {
        Handler->OnKeyDownDelegate.Remove(DelegateHandle);
    }
     
    for (FDelegateHandle DelegateHandle : BindKeyUpDelegateHandles)
    {
        Handler->OnKeyUpDelegate.Remove(DelegateHandle);
    }
    for (FDelegateHandle Handle : BindMouseDelegateHandles)
    {
        Handler->OnMouseDownDelegate.Remove(Handle);
        Handler->OnMouseUpDelegate.Remove(Handle);
        Handler->OnMouseDoubleClickDelegate.Remove(Handle);
        Handler->OnMouseWheelDelegate.Remove(Handle);
        Handler->OnMouseMoveDelegate.Remove(Handle);
        Handler->OnRawMouseInputDelegate.Remove(Handle);
    }
    
    KeyBindDelegate.Empty();
    MouseBindDelegate.Empty();
    BindKeyDownDelegateHandles.Empty();
    BindKeyUpDelegateHandles.Empty();
    BindMouseDelegateHandles.Empty();
}

void UInputComponent::InputKey(const FKeyEvent& InKeyEvent)
{
    // 일반적인 단일 키 이벤트
    switch (InKeyEvent.GetCharacter())
    {
    case 'W':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::W);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::W);
            }
            break;
        }
    case 'A':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::A);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::A);
            }
            break;
        }
    case 'S':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::S);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::S);
            }
            break;
        }
    case 'D':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::D);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::D);
            }
            break;
        }
    case 'R':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::R);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::R);
            }
            break;
        }
    case 'Z':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::Z);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::Z);
            }
            break;
        }
    case 'X':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::X);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::X);
            }
            break;
        }
    case 'Q':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::Q);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::Q);
            }
            break;
        }
    case 'C':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::C);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::C);
            }
            break;
        }
    case 'Y':
    {
        if (InKeyEvent.GetInputEvent() == IE_Pressed)
        {
            PressedKeys.Add(EKeys::Y);
        }
        else if (InKeyEvent.GetInputEvent() == IE_Released)
        {
            PressedKeys.Remove(EKeys::Y);
        }
        break;
    }
    case 'E':
    {
        if (InKeyEvent.GetInputEvent() == IE_Pressed)
        {
            PressedKeys.Add(EKeys::E);
        }
        else if (InKeyEvent.GetInputEvent() == IE_Released)
        {
            PressedKeys.Remove(EKeys::E);
        }
        break;
    }
    case '1':
    {
        if (InKeyEvent.GetInputEvent() == IE_Pressed)
        {
            PressedKeys.Add(EKeys::One);
        }
        else if (InKeyEvent.GetInputEvent() == IE_Released)
        {
            PressedKeys.Remove(EKeys::One);
        }
        break;
    }
    case '2':
    {
        if (InKeyEvent.GetInputEvent() == IE_Pressed)
        {
            PressedKeys.Add(EKeys::Two);
        }
        else if (InKeyEvent.GetInputEvent() == IE_Released)
        {
            PressedKeys.Remove(EKeys::Two);
        }
        break;
    }
    case '3':
        if (InKeyEvent.GetInputEvent() == IE_Pressed)
        {
            PressedKeys.Add(EKeys::Three);
        }
        else if (InKeyEvent.GetInputEvent() == IE_Released)
        {
            PressedKeys.Remove(EKeys::Three);
        }
        break;
    case '4':
        if (InKeyEvent.GetInputEvent() == IE_Pressed)
        {
            PressedKeys.Add(EKeys::Four);
        }
        else if (InKeyEvent.GetInputEvent() == IE_Released)
        {
            PressedKeys.Remove(EKeys::Four);
        }
        break;
    case '5':
        if (InKeyEvent.GetInputEvent() == IE_Pressed)
        {
            PressedKeys.Add(EKeys::Five);
        }
        else if (InKeyEvent.GetInputEvent() == IE_Released)
        {
            PressedKeys.Remove(EKeys::Five);
        }
        break;
    default:
        break;
    }
}


void UInputComponent::BindAction(const FString& Key, const std::function<void(float)>& Callback)
{
    if (Callback == nullptr)
    {
        return;
    }
    
    KeyBindDelegate[Key].AddLambda([this, Callback](float DeltaTime)
    {
        Callback(DeltaTime);
    });
}

void UInputComponent::BindMouseAction(const FString& Key, const std::function<void(const FPointerEvent&)>& Callback)
{
    if (Callback == nullptr)
    {
        return;
    }
    
    MouseBindDelegate[Key].AddLambda([this, Callback](const FPointerEvent& InMouseEvent)
    {
        Callback(InMouseEvent);
    });
}
