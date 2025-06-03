#include "CarActor.h"
#include <Lua/LuaUtils/LuaTypeMacros.h>

ACarActor::ACarActor()
{
    UCarComponent* Car = AddComponent<UCarComponent>("Car");
}

UObject* ACarActor::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    return NewComponent;
}

void ACarActor::RegisterLuaType(sol::state& Lua)
{
    Super::RegisterLuaType(Lua);
    RootComponent->RegisterLua(Lua);
}
