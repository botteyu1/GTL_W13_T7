#include "FloorSoil.h"

#include "Engine/AssetManager.h"
#include "GameFramework/Actor.h"

UFloorSoil::UFloorSoil()
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
    
    SetRelativeScale3D(FVector(2.5f, 0.5f, 0.8f)); // 필라 소일 크기 조정
}
void UFloorSoil::InitializeComponent()
{
    UStaticMeshComponent::InitializeComponent();
    GetOwner()->SetRootComponent(this);
}
