#pragma once
#include "Components/StaticMeshComponent.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"


class UPillarSoil : public UStaticMeshComponent
{
    DECLARE_CLASS(UPillarSoil, UStaticMeshComponent)

public:
    UPillarSoil();
    void InitializeComponent() override;
    virtual ~UPillarSoil() override = default;
    
};
