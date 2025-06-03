#pragma once

#include "GameFramework/Actor.h"
#include "UObject/Casts.h"
#include "Components/CarComponent.h"

class UCameraComponent;

class ACarActor : public AActor
{
    DECLARE_CLASS(ACarActor, AActor)

public:
    ACarActor();

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void BeginPlay() override;

private:
    UCameraComponent* Camera = nullptr;
};
