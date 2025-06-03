#pragma once
#include "GameFramework/Actor.h"


class AGravitySkeletalActor : public AActor
{
    DECLARE_CLASS(AGravitySkeletalActor, AActor)

public:
    AGravitySkeletalActor();
    virtual ~AGravitySkeletalActor() override = default;
    virtual void BeginPlay() override;
    
};
