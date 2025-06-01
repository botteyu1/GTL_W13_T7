#pragma once

#include "Core/Container/Map.h"
#include "Delegates/DelegateCombination.h"
#include "Runtime/InputCore/InputCoreTypes.h"
#include "Components/ActorComponent.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOneFloatDelegate, const float&)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnePointerEventDelegate, const FPointerEvent&)

class UInputComponent : public UActorComponent
{
    DECLARE_CLASS(UInputComponent, UActorComponent)


public:
    UInputComponent() = default;
    virtual ~UInputComponent() override = default;
    void BindAction(const FString& Key, const std::function<void(float)>& Callback);

    void BindMouseAction(const FString& Key, const std::function<void(const FPointerEvent&)>& Callback);

    void ProcessInput(float DeltaTime);
    
    void SetPossess();
    void BindInputDelegate();
    void UnPossess();
    void ClearBindDelegate();
    // Possess가 풀렸다가 다시 왔을때 원래 바인딩 돼있던 애들 일괄적으로 다시 바인딩해줘야할수도 있음.
    void InputKey(const FKeyEvent& InKeyEvent);

private:
    TMap<FString, FOneFloatDelegate> KeyBindDelegate;
    TMap<FString,FOnePointerEventDelegate> MouseBindDelegate;
    
    TArray<FDelegateHandle> BindKeyDownDelegateHandles;
    TArray<FDelegateHandle> BindKeyUpDelegateHandles;
    TArray<FDelegateHandle> BindMouseDelegateHandles;


    TSet<EKeys::Type> PressedKeys;
};


