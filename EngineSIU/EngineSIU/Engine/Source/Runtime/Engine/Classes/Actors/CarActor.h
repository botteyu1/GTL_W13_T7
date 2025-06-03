#pragma once

#include "GameFramework/Actor.h"
#include "UObject/Casts.h"
#include "Components/CarComponent.h"

class ACarActor : public AActor
{
    DECLARE_CLASS(ACarActor, AActor)

public:
    ACarActor();

    virtual UObject* Duplicate(UObject* InOuter) override;
    virtual void RegisterLuaType(sol::state& Lua) override;
};
