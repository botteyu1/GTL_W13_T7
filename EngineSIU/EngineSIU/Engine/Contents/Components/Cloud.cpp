#include "Cloud.h"

#include "FloorSoil.h"
#include "Engine/AssetManager.h"
#include "GameFramework/Actor.h"

UCloud::UCloud()
{
    bSimulate = true;
    bApplyGravity = false;
    RigidBodyType = ERigidBodyType::DYNAMIC;

    FString MeshName = TEXT("Contents/Cloud/Cloud");

    UStaticMesh* StaticMesh = UAssetManager::Get().GetStaticMesh(MeshName);
    if (StaticMesh)
    {
        SetStaticMesh(StaticMesh);
    }

    SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f)); // 클라우드 크기 조정
}

void UCloud::InitializeComponent()
{
    UStaticMeshComponent::InitializeComponent();
    GetOwner()->SetRootComponent(this);
}
