#include "Whomp.h"

#include "PillarSoil.h"
#include "Engine/AssetManager.h"
#include "GameFramework/Actor.h"

UWhomp::UWhomp()
{
    bSimulate = true;
    bApplyGravity = false;
    RigidBodyType = ERigidBodyType::STATIC;

    FString MeshName = TEXT("Contents/Floor/Whomp");

    UStaticMesh* StaticMesh = UAssetManager::Get().GetStaticMesh(MeshName);
    if (StaticMesh)
    {
        SetStaticMesh(StaticMesh);
    }
    
    SetRelativeScale3D(FVector(.2f, .5f, 0.5f)); // 필라 소일 크기 조정
}

void UWhomp::InitializeComponent()
{
    UStaticMeshComponent::InitializeComponent();
    GetOwner()->SetRootComponent(this);
}
