#pragma once
#include "Components/StaticMeshComponent.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"


class UWhomp : public UStaticMeshComponent
{
    DECLARE_CLASS(UWhomp, UStaticMeshComponent)

public:
    UWhomp();
    void InitializeComponent();
    virtual ~UWhomp() override = default;
    
    
};
