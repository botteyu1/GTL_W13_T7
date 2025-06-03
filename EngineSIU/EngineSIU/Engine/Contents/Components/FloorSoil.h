#pragma once
#include "Components/StaticMeshComponent.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"


class UFloorSoil : public UStaticMeshComponent
{
    DECLARE_CLASS(UFloorSoil, UStaticMeshComponent)

public:
    UFloorSoil();
    void InitializeComponent();
    virtual ~UFloorSoil() override = default;
    
};
