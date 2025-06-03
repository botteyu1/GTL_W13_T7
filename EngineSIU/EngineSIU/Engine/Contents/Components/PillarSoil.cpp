#include "PillarSoil.h"

#include "Engine/AssetManager.h"
#include "GameFramework/Actor.h"

UPillarSoil::UPillarSoil()
{
    bSimulate = true;
    bApplyGravity = true;
    RigidBodyType = ERigidBodyType::DYNAMIC;

    FString MeshName = TEXT("Contents/Pillar/PillarSoil");

    UStaticMesh* StaticMesh = UAssetManager::Get().GetStaticMesh(MeshName);
    if (StaticMesh)
    {
        SetStaticMesh(StaticMesh);
    }
    
    SetRelativeScale3D(FVector(.5f, .5f, 0.5f)); // 필라 소일 크기 조정
}

void UPillarSoil::InitializeComponent()
{
    UStaticMeshComponent::InitializeComponent();
    GetOwner()->SetRootComponent(this);
}
