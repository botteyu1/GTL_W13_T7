#pragma once
#include "Components/StaticMeshComponent.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"


class UCloud : public UStaticMeshComponent
{
    DECLARE_CLASS(UCloud, UStaticMeshComponent)

public:
    UCloud();
    void InitializeComponent();
    virtual ~UCloud() override = default;
    
};
