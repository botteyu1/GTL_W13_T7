#pragma once
#include "Components/StaticMeshComponent.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"


class UMysteryBoxComponent : public UStaticMeshComponent
{
    DECLARE_CLASS(UMysteryBoxComponent, UStaticMeshComponent)

public:
    UMysteryBoxComponent();
    virtual ~UMysteryBoxComponent() override = default;
    virtual void InitializeComponent() override;

    
};
