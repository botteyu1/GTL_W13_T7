#include "SkeletalMeshComponent.h"

#include "PhysicsManager.h"
#include "ReferenceSkeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include "Misc/FrameTime.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimTypes.h"
#include "Engine/Engine.h"
#include "Misc/Parse.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UObject/Casts.h"
#include "UObject/ObjectFactory.h"
#include "PhysicsEngine/ConstraintInstance.h"

bool USkeletalMeshComponent::bIsCPUSkinning = false;

USkeletalMeshComponent::USkeletalMeshComponent()
    : AnimationMode(EAnimationMode::AnimationSingleNode)
    , SkeletalMeshAsset(nullptr)
    , AnimClass(nullptr)
    , AnimScriptInstance(nullptr)
    , bPlayAnimation(true)
    ,BonePoseContext(nullptr)
{
    CPURenderData = std::make_unique<FSkeletalMeshRenderData>();
}

void USkeletalMeshComponent::InitializeComponent()
{
    Super::InitializeComponent();

    InitAnim();
}

UObject* USkeletalMeshComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewComponent->SetRelativeTransform(GetRelativeTransform());
    NewComponent->SetSkeletalMeshAsset(SkeletalMeshAsset);
    NewComponent->SetAnimationMode(AnimationMode);
    if (AnimationMode == EAnimationMode::AnimationBlueprint)
    {
        NewComponent->SetAnimClass(AnimClass);
        // TODO: 애님 인스턴스 세팅하기
        //UMyAnimInstance* AnimInstance = Cast<UMyAnimInstance>(NewComponent->GetAnimInstance());
        //AnimInstance->SetPlaying(Cast<UMyAnimInstance>(AnimScriptInstance)->IsPlaying());
    }
    else
    {
        NewComponent->SetAnimation(GetAnimation());
    }
    NewComponent->SetLooping(this->IsLooping());
    NewComponent->SetPlaying(this->IsPlaying());
    return NewComponent;
}

void USkeletalMeshComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);

    if (InProperties.Contains("SkeletalMeshKey"))
    {
        FName SkelMeshKey = FName(InProperties["SkeletalMeshKey"]);
        if (USkeletalMesh* SkelMesh = Cast<USkeletalMesh>(UAssetManager::Get().GetAsset(EAssetType::SkeletalMesh, SkelMeshKey)))
        {
            SetSkeletalMeshAsset(SkelMesh);
        }
    }
    
    if (InProperties.Contains("AnimationMode"))
    {
        const EAnimationMode Mode = static_cast<EAnimationMode>(FString::ToInt(InProperties["AnimationMode"]));
        SetAnimationMode(Mode);
    }

    if (InProperties.Contains("AnimClass") && AnimationMode == EAnimationMode::AnimationBlueprint)
    {
        UClass* InAnimClass = UClass::FindClass(InProperties["AnimClass"]);
        SetAnimClass(InAnimClass);
    }

    if (AnimationMode == EAnimationMode::AnimationSingleNode)
    {
        if (InProperties.Contains("AnimToPlay"))
        {
            FName AnimKey = FName(InProperties["AnimToPlay"]);
            if (UAnimationAsset* Anim = Cast<UAnimationAsset>(UAssetManager::Get().GetAsset(EAssetType::Animation, AnimKey)))
            {
                SetAnimation(Anim);
            }
        }

        if (InProperties.Contains("PlayRate"))
        {
            SetPlayRate(FString::ToFloat(InProperties["PlayRate"]));
        }
        if (InProperties.Contains("bLooping"))
        {
            SetLooping(InProperties["bLooping"] == "true");
        }
        if (InProperties.Contains("bPlaying"))
        {
            SetPlaying(InProperties["bPlaying"] == "true");
        }
        if (InProperties.Contains("bReverse"))
        {
            SetReverse(InProperties["bReverse"] == "true");
        }
        if (InProperties.Contains("LoopStartFrame"))
        {
            SetLoopStartFrame(FString::ToFloat(InProperties["LoopStartFrame"]));
        }
        if (InProperties.Contains("LoopEndFrame"))
        {
            SetLoopEndFrame(FString::ToFloat(InProperties["LoopEndFrame"]));
        }
    }
}

void USkeletalMeshComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);

    const FName SkelMeshKey = UAssetManager::Get().GetAssetKeyByObject(EAssetType::SkeletalMesh, GetSkeletalMeshAsset());
    OutProperties.Add(TEXT("SkeletalMeshKey"), SkelMeshKey.ToString());

    OutProperties.Add(TEXT("AnimationMode"), FString::FromInt(static_cast<uint8>(AnimationMode)));

    FString AnimClassStr = FName().ToString();
    if (AnimationMode == EAnimationMode::AnimationBlueprint)
    {
        if (AnimClass && AnimClass.Get())
        {
            AnimClassStr = AnimClass.Get()->GetName();
        }
    }
    OutProperties.Add(TEXT("AnimClass"), AnimClassStr);

    if (AnimationMode == EAnimationMode::AnimationSingleNode)
    {
        const FName AnimKey = UAssetManager::Get().GetAssetKeyByObject(EAssetType::Animation, GetAnimation());
        OutProperties.Add(TEXT("AnimToPlay"), AnimKey.ToString());

        OutProperties.Add(TEXT("PlayRate"), std::to_string(GetPlayRate()));
        OutProperties.Add(TEXT("bLooping"), IsLooping() ? "true" : "false");
        OutProperties.Add(TEXT("bPlaying"), IsPlaying() ? "true" : "false");
        OutProperties.Add(TEXT("bReverse"), IsReverse() ? "true" : "false");
        OutProperties.Add(TEXT("LoopStartFrame"), std::to_string(GetLoopStartFrame()));
        OutProperties.Add(TEXT("LoopEndFrame"), std::to_string(GetLoopEndFrame()));
    }
}

void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    //bool로 하면 맨 처음부터 애니메이션이 안돌아감
    if (bDisableAnimAfterHit<bDisableAnimAfterHitMax)
    {
        TickPose(DeltaTime);
    }
}

void USkeletalMeshComponent::EndPhysicsTickComponent(float DeltaTime)
{
    if (!bSimulate)
        return;

    const FReferenceSkeleton& RefSkeleton = SkeletalMeshAsset->GetSkeleton()->GetReferenceSkeleton();
    const int32 BoneNum = RefSkeleton.GetRawBoneNum();
    const FVector CompScale = GetComponentScale3D();
    TArray<FMatrix> BoneWorldMatrices;
    BoneWorldMatrices.SetNum(BoneNum);

    bool bPoseChanged = false;
    const bool bIsFirstPhysicsFrame = (PrevPhysicsBoneWorldMatrices.Num() != BoneNum);

    // 이전 프레임과 비교해 이 거리 이상 움직였을 경우 Pose 변경으로 간주
    constexpr float PoseChangeThresholdSqr = 0.001f;
    for (int32 i = 0; i < BoneNum; ++i)
    {
        bool bFoundBody = false;
        for (FBodyInstance* BI : Bodies)
        {
            if (BI->BoneIndex == i)
            {
                BI->BIGameObject->UpdateFromPhysics(GEngine->PhysicsManager->GetScene(GEngine->ActiveWorld));
                XMFLOAT4X4 dxMat;
                XMStoreFloat4x4(&dxMat, BI->BIGameObject->WorldMatrix);

                FMatrix WorldMatrix;
                for (int r = 0; r < 4; ++r)
                    for (int c = 0; c < 4; ++c)
                        WorldMatrix.M[r][c] = *(&dxMat._11 + r * 4 + c);

                FMatrix ScaledMatrix = FMatrix::CreateScaleMatrix(CompScale) * WorldMatrix;
                BoneWorldMatrices[i] = ScaledMatrix;

                // 비교
                if (!bIsFirstPhysicsFrame)
                {
                    FVector Prev = PrevPhysicsBoneWorldMatrices[i].GetOrigin();
                    FVector Curr = ScaledMatrix.GetOrigin();

                    if ((Prev - Curr).SizeSquared() > PoseChangeThresholdSqr)
                    {
                        bPoseChanged = true;
                    }
                }
                else
                {
                    // 첫 프레임이면 무조건 Pose가 변경된 것으로 처리
                    bPoseChanged = true;
                }
                /*
                else
                {
                    bPoseChanged = true;
                }*/

                bFoundBody = true;
                break;
            }
        }

        if (!bFoundBody)
        {
            int32 ParentIndex = RefSkeleton.GetParentIndex(i);
            FMatrix ParentMatrix = (ParentIndex == INDEX_NONE)
                ? GetComponentTransform().ToMatrixWithScale()
                : BoneWorldMatrices[ParentIndex];

            FMatrix Local = RefSkeleton.GetRawRefBonePose()[i].ToMatrixWithScale();
            BoneWorldMatrices[i] = Local * ParentMatrix;
        }
    }

    // 모든 Bone이 동일하면 return
    /*if (!bPoseChanged)
        return;*/
    // ✅ 한번이라도 물리 Pose가 변하면 이후 애니메이션 비활성화
    if (bPoseChanged)
    bDisableAnimAfterHit++;
    if (bDisableAnimAfterHit > bDisableAnimAfterHitMax)
    {
        if (!bPostAnimDisabledGravityApplied)
        {
            ApplyGravityToAllBodies();
            bPostAnimDisabledGravityApplied = true;
        }
    }
    //if (bDisableAnimAfterHit < bDisableAnimAfterHitMax)return;
    // 로컬 Pose 계산
    for (int32 i = 0; i < BoneNum; ++i)
    {
        int32 ParentIndex = RefSkeleton.GetParentIndex(i);
        FMatrix ParentMatrix = (ParentIndex == INDEX_NONE)
            ? GetComponentTransform().ToMatrixWithScale()
            : BoneWorldMatrices[ParentIndex];

        FMatrix Local = BoneWorldMatrices[i] * FMatrix::Inverse(ParentMatrix);
        if (bDisableAnimAfterHit > bDisableAnimAfterHitMax)
        BonePoseContext.Pose[i] = FTransform(Local);
    }

    PrevPhysicsBoneWorldMatrices = BoneWorldMatrices;

    CPUSkinning();
}

void USkeletalMeshComponent::ApplyGravityToAllBodies()
{
    for (FBodyInstance* BI : Bodies)
    {
        if (BI && BI->BIGameObject && BI->BIGameObject->DynamicRigidBody)
        {
            BI->BIGameObject->DynamicRigidBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, false);
        }
    }
}
void USkeletalMeshComponent::TickPose(float DeltaTime)
{
    if (!ShouldTickAnimation())
    {
        return;
    }

    TickAnimation(DeltaTime);
}

void USkeletalMeshComponent::TickAnimation(float DeltaTime)
{
    if (GetSkeletalMeshAsset())
    {
        TickAnimInstances(DeltaTime);
    }

    CPUSkinning();
}

void USkeletalMeshComponent::TickAnimInstances(float DeltaTime)
{
    if (AnimScriptInstance)
    {
        AnimScriptInstance->UpdateAnimation(DeltaTime, BonePoseContext);
    }
}

bool USkeletalMeshComponent::ShouldTickAnimation() const
{
    if (GEngine->GetWorldContextFromWorld(GetWorld())->WorldType == EWorldType::Editor)
    {
        return false;
    }
    return GetAnimInstance() && SkeletalMeshAsset && SkeletalMeshAsset->GetSkeleton();
}

bool USkeletalMeshComponent::InitializeAnimScriptInstance()
{
    USkeletalMesh* SkelMesh = GetSkeletalMeshAsset();
    
    if (NeedToSpawnAnimScriptInstance())
    {
        AnimScriptInstance = Cast<UAnimInstance>(FObjectFactory::ConstructObject(AnimClass, this));

        if (AnimScriptInstance)
        {
            AnimScriptInstance->InitializeAnimation();
        }
    }
    else
    {
        bool bShouldSpawnSingleNodeInstance = !AnimScriptInstance && SkelMesh && SkelMesh->GetSkeleton();
        if (bShouldSpawnSingleNodeInstance)
        {
            AnimScriptInstance = FObjectFactory::ConstructObject<UAnimSingleNodeInstance>(this);

            if (AnimScriptInstance)
            {
                AnimScriptInstance->InitializeAnimation();
            }
        }
    }

    return true;
}

void USkeletalMeshComponent::ClearAnimScriptInstance()
{
    if (AnimScriptInstance)
    {
        GUObjectArray.MarkRemoveObject(AnimScriptInstance);
    }
    AnimScriptInstance = nullptr;
}

void USkeletalMeshComponent::SetSkeletalMeshAsset(USkeletalMesh* InSkeletalMeshAsset)
{
    if (InSkeletalMeshAsset == GetSkeletalMeshAsset())
    {
        return;
    }
    
    SkeletalMeshAsset = InSkeletalMeshAsset;

    InitAnim();

    BonePoseContext.Pose.Empty();
    RefBonePoseTransforms.Empty();
    AABB = FBoundingBox(InSkeletalMeshAsset->GetRenderData()->BoundingBoxMin, SkeletalMeshAsset->GetRenderData()->BoundingBoxMax);
    
    const FReferenceSkeleton& RefSkeleton = SkeletalMeshAsset->GetSkeleton()->GetReferenceSkeleton();
    BonePoseContext.Pose.InitBones(RefSkeleton.RawRefBoneInfo.Num());
    for (int32 i = 0; i < RefSkeleton.RawRefBoneInfo.Num(); ++i)
    {
        BonePoseContext.Pose[i] = RefSkeleton.RawRefBonePose[i];
        RefBonePoseTransforms.Add(RefSkeleton.RawRefBonePose[i]);
    }
    
    CPURenderData->Vertices = InSkeletalMeshAsset->GetRenderData()->Vertices;
    CPURenderData->Indices = InSkeletalMeshAsset->GetRenderData()->Indices;
    CPURenderData->ObjectName = InSkeletalMeshAsset->GetRenderData()->ObjectName;
    CPURenderData->MaterialSubsets = InSkeletalMeshAsset->GetRenderData()->MaterialSubsets;
}

FTransform USkeletalMeshComponent::GetSocketTransform(FName SocketName) const
{
    FTransform Transform = FTransform::Identity;

    if (USkeleton* Skeleton = GetSkeletalMeshAsset()->GetSkeleton())
    {
        int32 BoneIndex = Skeleton->FindBoneIndex(SocketName);

        TArray<FMatrix> GlobalBoneMatrices;
        GetCurrentGlobalBoneMatrices(GlobalBoneMatrices);
        Transform = FTransform(GlobalBoneMatrices[BoneIndex]);
    }
    return Transform;
}

void USkeletalMeshComponent::GetCurrentGlobalBoneMatrices(TArray<FMatrix>& OutBoneMatrices) const
{
    const FReferenceSkeleton& RefSkeleton = SkeletalMeshAsset->GetSkeleton()->GetReferenceSkeleton();
    const int32 BoneNum = RefSkeleton.RawRefBoneInfo.Num();

    OutBoneMatrices.Empty();
    OutBoneMatrices.SetNum(BoneNum);

    for (int32 BoneIndex = 0; BoneIndex < BoneNum; ++BoneIndex)
    {
        // 현재 본의 로컬 변환
        FTransform CurrentLocalTransform = BonePoseContext.Pose[BoneIndex];
        FMatrix LocalMatrix = CurrentLocalTransform.ToMatrixWithScale(); // FTransform -> FMatrix
        
        // 부모 본의 영향을 적용하여 월드 변환 구성
        int32 ParentIndex = RefSkeleton.RawRefBoneInfo[BoneIndex].ParentIndex;
        if (ParentIndex != INDEX_NONE)
        {
            // 로컬 변환에 부모 월드 변환 적용
            LocalMatrix = LocalMatrix * OutBoneMatrices[ParentIndex];
        }
        
        // 결과 행렬 저장
        OutBoneMatrices[BoneIndex] = LocalMatrix;
    }
}

void USkeletalMeshComponent::DEBUG_SetAnimationEnabled(bool bEnable)
{
    bPlayAnimation = bEnable;
    
    if (!bPlayAnimation)
    {
        if (SkeletalMeshAsset && SkeletalMeshAsset->GetSkeleton())
        {
            const FReferenceSkeleton& RefSkeleton = SkeletalMeshAsset->GetSkeleton()->GetReferenceSkeleton();
            BonePoseContext.Pose.InitBones(RefSkeleton.RawRefBonePose.Num());
            for (int32 i = 0; i < RefSkeleton.RawRefBoneInfo.Num(); ++i)
            {
                BonePoseContext.Pose[i] = RefSkeleton.RawRefBonePose[i];
            }
        }
        SetElapsedTime(0.f); 
        CPURenderData->Vertices = SkeletalMeshAsset->GetRenderData()->Vertices;
        CPURenderData->Indices = SkeletalMeshAsset->GetRenderData()->Indices;
        CPURenderData->ObjectName = SkeletalMeshAsset->GetRenderData()->ObjectName;
        CPURenderData->MaterialSubsets = SkeletalMeshAsset->GetRenderData()->MaterialSubsets;
    }
}

void USkeletalMeshComponent::PlayAnimation(UAnimationAsset* NewAnimToPlay, bool bLooping)
{
    SetAnimation(NewAnimToPlay);
    Play(bLooping);
}

int USkeletalMeshComponent::CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const
{
    if (!AABB.Intersect(InRayOrigin, InRayDirection, OutHitDistance))
    {
        return 0;
    }
    if (SkeletalMeshAsset == nullptr)
    {
        return 0;
    }
    
    OutHitDistance = FLT_MAX;
    
    int IntersectionNum = 0;

    const FSkeletalMeshRenderData* RenderData = SkeletalMeshAsset->GetRenderData();

    const TArray<FSkeletalMeshVertex>& Vertices = RenderData->Vertices;
    const int32 VertexNum = Vertices.Num();
    if (VertexNum == 0)
    {
        return 0;
    }
    
    const TArray<UINT>& Indices = RenderData->Indices;
    const int32 IndexNum = Indices.Num();
    const bool bHasIndices = (IndexNum > 0);
    
    int32 TriangleNum = bHasIndices ? (IndexNum / 3) : (VertexNum / 3);
    for (int32 i = 0; i < TriangleNum; i++)
    {
        int32 Idx0 = i * 3;
        int32 Idx1 = i * 3 + 1;
        int32 Idx2 = i * 3 + 2;
        
        if (bHasIndices)
        {
            Idx0 = Indices[Idx0];
            Idx1 = Indices[Idx1];
            Idx2 = Indices[Idx2];
        }

        // 각 삼각형의 버텍스 위치를 FVector로 불러옵니다.
        FVector v0 = FVector(Vertices[Idx0].X, Vertices[Idx0].Y, Vertices[Idx0].Z);
        FVector v1 = FVector(Vertices[Idx1].X, Vertices[Idx1].Y, Vertices[Idx1].Z);
        FVector v2 = FVector(Vertices[Idx2].X, Vertices[Idx2].Y, Vertices[Idx2].Z);

        float HitDistance = FLT_MAX;
        if (IntersectRayTriangle(InRayOrigin, InRayDirection, v0, v1, v2, HitDistance))
        {
            OutHitDistance = FMath::Min(HitDistance, OutHitDistance);
            IntersectionNum++;
        }

    }
    return IntersectionNum;
}

const FSkeletalMeshRenderData* USkeletalMeshComponent::GetCPURenderData() const
{
    return CPURenderData.get();
}

void USkeletalMeshComponent::SetCPUSkinning(bool Flag)
{
    bIsCPUSkinning = Flag;
}

bool USkeletalMeshComponent::GetCPUSkinning()
{
    return bIsCPUSkinning;
}

void USkeletalMeshComponent::SetAnimationMode(EAnimationMode InAnimationMode)
{
    const bool bNeedsChange = AnimationMode != InAnimationMode;
    if (bNeedsChange)
    {
        AnimationMode = InAnimationMode;
        ClearAnimScriptInstance();
    }

    if (GetSkeletalMeshAsset() && (bNeedsChange || AnimationMode == EAnimationMode::AnimationBlueprint))
    {
        InitializeAnimScriptInstance();
    }
}

void USkeletalMeshComponent::InitAnim()
{
    if (GetSkeletalMeshAsset() == nullptr)
    {
        return;
    }

    bool bBlueprintMismatch = AnimClass && AnimScriptInstance && AnimScriptInstance->GetClass() != AnimClass;
    
    const USkeleton* AnimSkeleton = AnimScriptInstance ? AnimScriptInstance->GetCurrentSkeleton() : nullptr;
    
    const bool bClearAnimInstance = AnimScriptInstance && !AnimSkeleton;
    const bool bSkeletonMismatch = AnimSkeleton && (AnimScriptInstance->GetCurrentSkeleton() != GetSkeletalMeshAsset()->GetSkeleton());
    const bool bSkeletonsExist = AnimSkeleton && GetSkeletalMeshAsset()->GetSkeleton() && !bSkeletonMismatch;

    if (bBlueprintMismatch || bSkeletonMismatch || !bSkeletonsExist || bClearAnimInstance)
    {
        ClearAnimScriptInstance();
    }

    const bool bInitializedAnimInstance = InitializeAnimScriptInstance();

    if (bInitializedAnimInstance)
    {
        // TODO: 애니메이션 포즈 바로 반영하려면 여기에서 진행.
    }
}

void USkeletalMeshComponent::CreatePhysXGameObject()
{
    if (RigidBodyType == ERigidBodyType::STATIC)
    {
        RigidBodyType = ERigidBodyType::KINEMATIC;
    }

    const auto& Skeleton = SkeletalMeshAsset->GetSkeleton()->GetReferenceSkeleton();
    TArray<UBodySetup*> BodySetups = SkeletalMeshAsset->GetPhysicsAsset()->BodySetups;
    const FVector CompScale = GetComponentScale3D();

    for (int i = 0; i < BodySetups.Num(); i++)
    {
        FBodyInstance* NewBody = new FBodyInstance(this);
        NewBody->BodyInstanceName = BodySetups[i]->BoneName;
        NewBody->BoneIndex = Skeleton.FindBoneIndex(BodySetups[i]->BoneName);

        for (const auto& GeomAttribute : BodySetups[i]->GeomAttributes)
        {
            FVector ScaledOffset = GeomAttribute.Offset * CompScale;
            PxVec3 Offset = PxVec3(ScaledOffset.X, ScaledOffset.Y, ScaledOffset.Z);
            FQuat GeomQuat = GeomAttribute.Rotation.Quaternion();
            PxQuat GeomPQuat = PxQuat(GeomQuat.X, GeomQuat.Y, GeomQuat.Z, GeomQuat.W);
            FVector ScaledExtent = GeomAttribute.Extent * CompScale;
            PxVec3 Extent = PxVec3(ScaledExtent.X, ScaledExtent.Y, ScaledExtent.Z);

            PxShape* Shape = nullptr;

            switch (GeomAttribute.GeomType)
            {
            case EGeomType::ESphere:
                Shape = GEngine->PhysicsManager->CreateSphereShape(Offset, GeomPQuat, Extent.x);
                break;
            case EGeomType::EBox:
                Shape = GEngine->PhysicsManager->CreateBoxShape(Offset, GeomPQuat, Extent);
                break;
            case EGeomType::ECapsule:
                Shape = GEngine->PhysicsManager->CreateCapsuleShape(Offset, GeomPQuat, Extent.x, Extent.z);
                break;
            }

            if (Shape)
            {
                NewBody->Shapes.Add(Shape);
            }
        }

        // 본 위치 계산
        TArray<FMatrix> CurrentGlobalBoneMatrices;
        GetCurrentGlobalBoneMatrices(CurrentGlobalBoneMatrices);
        FMatrix CompToWorld = GetComponentTransform().ToMatrixWithScale();
        for (FMatrix& Mat : CurrentGlobalBoneMatrices)
        {
            Mat = Mat * CompToWorld;
        }

        FVector Location = CurrentGlobalBoneMatrices[NewBody->BoneIndex].GetTranslationVector();
        FQuat Rotation = FTransform(CurrentGlobalBoneMatrices[NewBody->BoneIndex]).GetRotation();

        PxVec3 Pos(Location.X, Location.Y, Location.Z);
        PxQuat Quat(Rotation.X, Rotation.Y, Rotation.Z, Rotation.W);

        GameObject* Obj = GEngine->PhysicsManager->CreateGameObject(Pos, Quat, NewBody, nullptr, RigidBodyType);

        // Custom Attach
        for (PxShape* Shape : NewBody->Shapes)
        {
            if (Obj->DynamicRigidBody)
                Obj->DynamicRigidBody->attachShape(*Shape);
            else if (Obj->StaticRigidBody)
                Obj->StaticRigidBody->attachShape(*Shape);
        }

        /*if (RigidBodyType != ERigidBodyType::STATIC && Obj->DynamicRigidBody)
        {
            Obj->DynamicRigidBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !bApplyGravity);
        }*/

        NewBody->SetGameObject(Obj);
        Bodies.Add(NewBody);
    }

    // Constraint 생성
    const auto& ConstraintSetups = SkeletalMeshAsset->GetPhysicsAsset()->ConstraintSetups;
    for (FConstraintSetup* Setup : ConstraintSetups)
    {
        FConstraintInstance* NewConstraintInstance = new FConstraintInstance;
        FBodyInstance* Body1 = nullptr;
        FBodyInstance* Body2 = nullptr;

        for (FBodyInstance* Body : Bodies)
        {
            if (Body->BodyInstanceName.ToString() == Setup->ConstraintBone1)
                Body1 = Body;
            if (Body->BodyInstanceName.ToString() == Setup->ConstraintBone2)
                Body2 = Body;
        }

        if (Body1 && Body2)
        {
            GEngine->PhysicsManager->CreateJoint(Body1->BIGameObject, Body2->BIGameObject, NewConstraintInstance, Setup);
            Constraints.Add(NewConstraintInstance);
        }
    }
}

void USkeletalMeshComponent::AddBodyInstance(FBodyInstance* BodyInstance)
{
    Bodies.Add(BodyInstance);
}

void USkeletalMeshComponent::AddConstraintInstance(FConstraintInstance* ConstraintInstance)
{
    Constraints.Add(ConstraintInstance);
}

void USkeletalMeshComponent::RemoveBodyInstance(FBodyInstance* BodyInstance)
{
    if (BodyInstance)
    {
        Bodies.Remove(BodyInstance);
    }
}

void USkeletalMeshComponent::RemoveConstraintInstance(FConstraintInstance* ConstraintInstance)
{
    if (ConstraintInstance)
    {
        Constraints.Remove(ConstraintInstance);
    }
}

bool USkeletalMeshComponent::NeedToSpawnAnimScriptInstance() const
{
    USkeletalMesh* MeshAsset = GetSkeletalMeshAsset();
    USkeleton* AnimSkeleton = MeshAsset ? MeshAsset->GetSkeleton() : nullptr;
    if (AnimationMode == EAnimationMode::AnimationBlueprint && AnimClass && AnimSkeleton)
    {
        if (AnimScriptInstance == nullptr || AnimScriptInstance->GetClass() != AnimClass || AnimScriptInstance->GetOuter() != this)
        {
            return true;
        }
    }
    return false;
}

void USkeletalMeshComponent::CPUSkinning(bool bForceUpdate)
{
    if (bIsCPUSkinning || bForceUpdate)
    {
         QUICK_SCOPE_CYCLE_COUNTER(SkinningPass_CPU)
         const FReferenceSkeleton& RefSkeleton = SkeletalMeshAsset->GetSkeleton()->GetReferenceSkeleton();
         TArray<FMatrix> CurrentGlobalBoneMatrices;
         GetCurrentGlobalBoneMatrices(CurrentGlobalBoneMatrices);
         const int32 BoneNum = RefSkeleton.RawRefBoneInfo.Num();
         
         // 최종 스키닝 행렬 계산
         TArray<FMatrix> FinalBoneMatrices;
         FinalBoneMatrices.SetNum(BoneNum);
    
         for (int32 BoneIndex = 0; BoneIndex < BoneNum; ++BoneIndex)
         {
             FinalBoneMatrices[BoneIndex] = RefSkeleton.InverseBindPoseMatrices[BoneIndex] * CurrentGlobalBoneMatrices[BoneIndex];
         }
         
         const FSkeletalMeshRenderData* RenderData = SkeletalMeshAsset->GetRenderData();
         
         for (int i = 0; i < RenderData->Vertices.Num(); i++)
         {
             FSkeletalMeshVertex Vertex = RenderData->Vertices[i];
             // 가중치 합산
             float TotalWeight = 0.0f;
    
             FVector SkinnedPosition = FVector(0.0f, 0.0f, 0.0f);
             FVector SkinnedNormal = FVector(0.0f, 0.0f, 0.0f);
             
             for (int j = 0; j < 4; ++j)
             {
                 float Weight = Vertex.BoneWeights[j];
                 TotalWeight += Weight;
     
                 if (Weight > 0.0f)
                 {
                     uint32 BoneIdx = Vertex.BoneIndices[j];
                     
                     // 본 행렬 적용 (BoneMatrices는 이미 최종 스키닝 행렬)
                     // FBX SDK에서 가져온 역바인드 포즈 행렬이 이미 포함됨
                     FVector Pos = FinalBoneMatrices[BoneIdx].TransformPosition(FVector(Vertex.X, Vertex.Y, Vertex.Z));
                     FVector4 Norm4 = FinalBoneMatrices[BoneIdx].TransformFVector4(FVector4(Vertex.NormalX, Vertex.NormalY, Vertex.NormalZ, 0.0f));
                     FVector Norm(Norm4.X, Norm4.Y, Norm4.Z);
                     
                     SkinnedPosition += Pos * Weight;
                     SkinnedNormal += Norm * Weight;
                 }
             }
    
             // 가중치 예외 처리
             if (TotalWeight < 0.001f)
             {
                 SkinnedPosition = FVector(Vertex.X, Vertex.Y, Vertex.Z);
                 SkinnedNormal = FVector(Vertex.NormalX, Vertex.NormalY, Vertex.NormalZ);
             }
             else if (FMath::Abs(TotalWeight - 1.0f) > 0.001f && TotalWeight > 0.001f)
             {
                 // 가중치 합이 1이 아닌 경우 정규화
                 SkinnedPosition /= TotalWeight;
                 SkinnedNormal /= TotalWeight;
             }
    
             CPURenderData->Vertices[i].X = SkinnedPosition.X;
             CPURenderData->Vertices[i].Y = SkinnedPosition.Y;
             CPURenderData->Vertices[i].Z = SkinnedPosition.Z;
             CPURenderData->Vertices[i].NormalX = SkinnedNormal.X;
             CPURenderData->Vertices[i].NormalY = SkinnedNormal.Y;
             CPURenderData->Vertices[i].NormalZ = SkinnedNormal.Z;
           }
     }
}

UAnimSingleNodeInstance* USkeletalMeshComponent::GetSingleNodeInstance() const
{
    return Cast<UAnimSingleNodeInstance>(AnimScriptInstance);
}

void USkeletalMeshComponent::SetAnimClass(UClass* NewClass)
{
    SetAnimInstanceClass(NewClass);
}

UClass* USkeletalMeshComponent::GetAnimClass() const
{
    return AnimClass;
}

void USkeletalMeshComponent::SetAnimInstanceClass(class UClass* NewClass)
{
    if (NewClass != nullptr)
    {
        // set the animation mode
        const bool bWasUsingBlueprintMode = AnimationMode == EAnimationMode::AnimationBlueprint;
        AnimationMode = EAnimationMode::AnimationBlueprint;

        if (NewClass != AnimClass || !bWasUsingBlueprintMode)
        {
            // Only need to initialize if it hasn't already been set or we weren't previously using a blueprint instance
            AnimClass = NewClass;
            ClearAnimScriptInstance();
            InitAnim();
        }
    }
    else
    {
        // Need to clear the instance as well as the blueprint.
        // @todo is this it?
        AnimClass = nullptr;
        ClearAnimScriptInstance();
    }
}

void USkeletalMeshComponent::SetAnimation(UAnimationAsset* NewAnimToPlay)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetAnimationAsset(NewAnimToPlay, false);
        SingleNodeInstance->SetPlaying(false);

        // TODO: Force Update Pose and CPU Skinning
    }
}

UAnimationAsset* USkeletalMeshComponent::GetAnimation() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetAnimationAsset();
    }
    return nullptr;
}

void USkeletalMeshComponent::Play(bool bLooping)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetPlaying(true);
        SingleNodeInstance->SetLooping(bLooping);
    }
}

void USkeletalMeshComponent::Stop()
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetPlaying(false);
    }
}

void USkeletalMeshComponent::SetPlaying(bool bPlaying)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetPlaying(bPlaying);
    }
}

bool USkeletalMeshComponent::IsPlaying() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->IsPlaying();
    }

    return false;
}

void USkeletalMeshComponent::SetReverse(bool bIsReverse)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetReverse(bIsReverse);
    }
}

bool USkeletalMeshComponent::IsReverse() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->IsReverse();
    }
}

void USkeletalMeshComponent::SetPlayRate(float Rate)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetPlayRate(Rate);
    }
}

float USkeletalMeshComponent::GetPlayRate() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetPlayRate();
    }

    return 0.f;
}

void USkeletalMeshComponent::SetLooping(bool bIsLooping)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetLooping(bIsLooping);
    }
}

bool USkeletalMeshComponent::IsLooping() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->IsLooping();
    }
    return false;
}

int USkeletalMeshComponent::GetCurrentKey() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetCurrentKey();
    }
    return 0;
}

void USkeletalMeshComponent::SetCurrentKey(int InKey)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetCurrentKey(InKey);
    }
}

void USkeletalMeshComponent::SetElapsedTime(float InElapsedTime)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetElapsedTime(InElapsedTime);
    }
}

float USkeletalMeshComponent::GetElapsedTime() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetElapsedTime();
    }
    return 0.f;
}

int32 USkeletalMeshComponent::GetLoopStartFrame() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetLoopStartFrame();
    }
    return 0;
}

void USkeletalMeshComponent::SetLoopStartFrame(int32 InLoopStartFrame)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetLoopStartFrame(InLoopStartFrame);
    }
}

int32 USkeletalMeshComponent::GetLoopEndFrame() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetLoopEndFrame();
    }
    return 0;
}

void USkeletalMeshComponent::SetLoopEndFrame(int32 InLoopEndFrame)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetLoopEndFrame(InLoopEndFrame);
    }
}
