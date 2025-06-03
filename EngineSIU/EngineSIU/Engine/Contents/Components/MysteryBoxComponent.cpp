#include "MysteryBoxComponent.h"

#include "Engine/AssetManager.h"
#include "GameFramework/Actor.h"

UMysteryBoxComponent::UMysteryBoxComponent()
{
    bSimulate = true;
    bApplyGravity = true;
    RigidBodyType = ERigidBodyType::DYNAMIC;

    FString MeshName = TEXT("Contents/MarioMysteryBlock/MarioMysteryBlock");

    UStaticMesh* StaticMesh = UAssetManager::Get().GetStaticMesh(MeshName);
    if (StaticMesh)
    {
        SetStaticMesh(StaticMesh);
    }
    SetRelativeScale3D(FVector(0.07f, 0.07f, 0.07f)); // 클라우드 크기 조정
}

void UMysteryBoxComponent::InitializeComponent()
{
    UStaticMeshComponent::InitializeComponent();
    GetOwner()->SetRootComponent(this);
}
