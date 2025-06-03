#include "CarComponent.h"

#include "Engine/AssetManager.h"
#include "Engine/FObjLoader.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "Lua/LuaScriptComponent.h"
#include "Lua/LuaScriptManager.h"
#include "Lua/LuaUtils/LuaTypeMacros.h"
#include <Camera/CameraComponent.h>
#include <World/World.h>
#include <Engine/Contents/Actors/FreeCameraActor.h>
#include <GameFramework/SpringArmComponent.h>

#include "SoundManager.h"
#include "Particles/ParticleSystem.h"

UCarComponent::UCarComponent()
{
    bSimulate = true;
}

UCarComponent::~UCarComponent()
{
    if (CarBody)
    {
        Release();
    }
}

UObject* UCarComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    return NewComponent;
}

void UCarComponent::TickComponent(float DeltaTime)
{
    MoveCar();
    if (Camera)
    {
        FVector Pos = GetComponentLocation() + FVector(1, 0, 0) * -15.f + FVector(0, 0, 3.5f);
        Camera->SetWorldLocation(Pos);
        FVector Target = GetComponentLocation() + FVector(1, 0, 0) * 10.f;
        Camera->SetLookTarget(Target);
        Camera->SetWorldRotation(FQuat::Identity);
    }
    if (bBoosted)
        bCarDriving = false;
    else
        bCarDriving = true;
    BoostParticle->bEnabled = bBoosted;

    bool bShouldPlayEngineSound = (Velocity > 0.2f);

    // 2. 현재 재생 중인 사운드 상태와 원하는 상태를 비교하여 필요할 때만 사운드 변경
    if (bShouldPlayEngineSound && !bSoundCarEngine)
    {
        // 현재 상태: 아이들 사운드 재생 중 (또는 아무것도 재생 안함)
        // 원하는 상태: 엔진 사운드 재생
        // 변경: 아이들 -> 엔진
        FSoundManager::GetInstance().StopSound("Car_Idle");
        FSoundManager::GetInstance().PlaySound("Car_Engine"); // "Car_Engine"이 루핑 사운드라면, 멱등성으로 인해 이미 재생 중이면 새 인스턴스를 만들지 않음
        bSoundCarEngine = true; // 현재 엔진 사운드가 재생 중임을 표시
    }
    else if (!bShouldPlayEngineSound && bSoundCarEngine)
    {
        // 현재 상태: 엔진 사운드 재생 중
        // 원하는 상태: 아이들 사운드 재생
        // 변경: 엔진 -> 아이들
        FSoundManager::GetInstance().StopSound("Car_Engine");
        FSoundManager::GetInstance().PlaySound("Car_Idle"); // "Car_Idle"이 루핑 사운드라면, 멱등성으로 인해 이미 재생 중이면 새 인스턴스를 만들지 않음
        bSoundCarEngine = false; // 현재 아이들 사운드가 재생 중임을 표시
    }
    //if (GetAsyncKeyState('R') & 0x8000)
    //{
    //    Restart();
    //}
}

void UCarComponent::EndPhysicsTickComponent(float DeltaTime)
{
    //물리 시뮬
    if (!CarBody)
    {
        return;
    }
    UpdateFromPhysics(CarBody, this);
    for (int i = 0; i < 4; ++i)
    {
        if (!Wheels[i])
        {
            continue;
        }
        UpdateFromPhysics(Wheels[i], WheelComp[i]);
    }
}

void UCarComponent::CreatePhysXGameObject()
{
    TArray<USceneComponent*> DestroyComps;
    for (auto& Comp : GetAttachChildren())
    {
        bool bIsWheel = false;
        for (int i = 0; i < 4; ++i)
        {
            if (Comp == WheelComp[i])
            {
                UE_LOG(ELogLevel::Display, "Found Wheel!");
                bIsWheel = true;
                break;
            }
        }
        if (!bIsWheel)
            DestroyComps.Add(Comp);
    }
    for (auto& Comp : DestroyComps)
    {
        if (UCameraComponent* DuplicateCamera = Cast<UCameraComponent>(Comp))
        {
            Camera = DuplicateCamera;
            Camera->SetRelativeLocation(FVector(1, 0, 0) * -15.f + FVector(0, 0, 2.f));
            continue;
        }
        Comp->DestroyComponent();
    }
    Spawn();

    PxPhysics* Physics = GEngine->PhysicsManager->GetPhysics();
    PxCooking* Cooking = GEngine->PhysicsManager->GetCooking();
    PxScene* Scene = GEngine->PhysicsManager->GetScene(GEngine->ActiveWorld);
    if (!DefaultMaterial)
        DefaultMaterial = Physics->createMaterial(1.f, 0.7f, 0.f);

    PxQuat CarRotation = GetOwner()->GetActorRotation().Quaternion().ToPxQuat();

    //몸통    
    CarBody = new GameObject();

    PxQuat BodyQuat = GetComponentRotation().Quaternion().ToPxQuat();

    BodyExtent = (AABB.MaxLocation - AABB.MinLocation) * GetComponentScale3D() * 0.5f;
    BodyExtent.Y *= 0.65f;
    BodyExtent.Z *= 0.3f;
    BodyExtent.X *= 0.45f;
    PxBoxGeometry CarBodyGeom(BodyExtent.ToPxVec3());
    CarBody->DynamicRigidBody = Physics->createRigidDynamic(
        PxTransform((GetComponentLocation() + GetUpVector() * BodyExtent.Z - GetForwardVector() * BodyExtent.X * 0.3f).ToPxVec3(), BodyQuat));
    //CarBody->DynamicRigidBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
    {
        PxShape* BodyShape = Physics->createShape(CarBodyGeom, *DefaultMaterial);
        BodyShape->setSimulationFilterData(PxFilterData(ECollisionChannel::ECC_CarBody, 0xFFFF, 0, 0));
        CarBody->DynamicRigidBody->attachShape(*BodyShape);
        BodyShape->release();
        PxReal volume = 8 * BodyExtent.X * BodyExtent.Y * BodyExtent.Z;
        PxReal density = 3000.f / volume;
        PxRigidBodyExt::updateMassAndInertia(*CarBody->DynamicRigidBody, density);

        //PxVec3 inertia = CarBody->DynamicRigidBody->getMassSpaceInertiaTensor();
        //inertia.z *= 0.5f;
        //CarBody->DynamicRigidBody->setMassSpaceInertiaTensor(inertia);

        CarBody->DynamicRigidBody->setLinearDamping(0.1f);
        CarBody->DynamicRigidBody->setAngularDamping(0.1f);
        CarBody->DynamicRigidBody->setSolverIterationCounts(
            /*minPositionIters=*/12,
            /*minVelocityIters=*/6
        );
        CarBody->DynamicRigidBody->setRigidDynamicLockFlags(
            PxRigidDynamicLockFlag::eLOCK_ANGULAR_X
        );
        BodyInstance = new FBodyInstance(this);
        CarBody->DynamicRigidBody->userData = BodyInstance;
        BodyInstance->BIGameObject = CarBody;
        Scene->addActor(*CarBody->DynamicRigidBody);
    }
    if (!WheelComp[0])
        return;
    //바퀴
    for (int i = 0; i < 4; ++i)
    {
        FVector WheelPosition = WheelComp[i]->GetComponentLocation();
        //FQuat WheelQuat = WheelComp[i]->GetComponentRotation().Quaternion();
        FVector WheelScale = WheelComp[i]->GetComponentScale3D();
        Wheels[i] = new GameObject();
        Wheels[i]->DynamicRigidBody = Physics->createRigidDynamic(PxTransform(WheelPosition.ToPxVec3()));
        //Wheels[i]->DynamicRigidBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
        FVector WheelSize = (WheelComp[i]->AABB.MaxLocation - WheelComp[i]->AABB.MinLocation) * GetComponentScale3D() * 0.5f;
        WheelRadius = WheelSize.X;
        WheelHeight = WheelSize.Y;
        PxShape* WheelShape = CreateWheelShape(Physics, Cooking, WheelScale, 4096);
        WheelShape->setSimulationFilterData(PxFilterData(ECollisionChannel::ECC_Wheel, 0xFFFF, 0, 0));
        Wheels[i]->DynamicRigidBody->attachShape(*WheelShape);
        WheelShape->release();
        PxReal WheelMass = 100.f;
        PxReal WheelVolume = WheelRadius * WheelRadius * PxPi * 2 * WheelHeight;
        PxRigidBodyExt::updateMassAndInertia(*Wheels[i]->DynamicRigidBody, WheelMass / WheelVolume);
        Wheels[i]->DynamicRigidBody->setLinearDamping(0.01f);
        Wheels[i]->DynamicRigidBody->setAngularDamping(0.01f);
        Wheels[i]->DynamicRigidBody->setSolverIterationCounts(
            /*minPositionIters=*/8,
            /*minVelocityIters=*/4
        );
        
        Scene->addActor(*Wheels[i]->DynamicRigidBody);
    }
    //허브
    PxTransform WheelT[4];
    for (int i = 0; i < 4; ++i)
        WheelT[i] = Wheels[i]->DynamicRigidBody->getGlobalPose();
    PxVec3 HubPositions[2] =
    {
        {(WheelT[0].p + WheelT[1].p) * 0.5f},
        {(WheelT[2].p + WheelT[3].p) * 0.5f}
    };
    PxVec3 HubSize[2] =
    {
        PxVec3(0.05f, (WheelT[0].p.y - WheelT[1].p.y) * 0.5f, 0.05f),
        PxVec3(0.05f, (WheelT[2].p.y - WheelT[3].p.y) * 0.5f, 0.05f)
    };
    for (int i = 0; i < 2; ++i)
    {
        PxTransform HubT(HubPositions[i], CarRotation);
        Hub[i] = new GameObject();
        Hub[i]->DynamicRigidBody = Physics->createRigidDynamic(HubT);
        //Hub[i]->DynamicRigidBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
        PxShape* HubShape = Physics->createShape(PxBoxGeometry(HubSize[i]), *DefaultMaterial);
        HubShape->setSimulationFilterData(PxFilterData(ECollisionChannel::ECC_Hub, 0xFFFF, 0, 0));
        Hub[i]->DynamicRigidBody->attachShape(*HubShape);
        PxReal HubVolume = 8 * HubSize[i].x * HubSize[i].y * HubSize[i].z;
        PxReal HubMass = 2500.f;
        if (i == 1)
            HubMass = 200.f;
        PxRigidBodyExt::updateMassAndInertia(*Hub[i]->DynamicRigidBody, HubMass / HubVolume);
        Hub[i]->DynamicRigidBody->setLinearDamping(0.01f);
        Hub[i]->DynamicRigidBody->setAngularDamping(0.01f);
        Hub[i]->DynamicRigidBody->setSolverIterationCounts(
            /*minPositionIters=*/12,
            /*minVelocityIters=*/6
        );
        if (i == 1)
        {
            Hub[i]->DynamicRigidBody->setRigidDynamicLockFlags(
                PxRigidDynamicLockFlag::eLOCK_ANGULAR_X |
                PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z
            );
        }
        Scene->addActor(*Hub[i]->DynamicRigidBody);
    }

    //조인트 설정
    PxQuat XtoY = PxQuat(PxHalfPi, PxVec3(0, 0, 1));
    PxQuat XtoZ = PxQuat(PxHalfPi, PxVec3(0, 1, 0));
    //Wheel-Hub (Front)
    for (int i = 0; i < 2; ++i)
    {
        PxTransform WheelT = Wheels[i]->DynamicRigidBody->getGlobalPose();
        PxVec3 JointPos = WheelT.p;
        PxTransform JointT(JointPos, XtoY);
        PxTransform HubLocal = Hub[0]->DynamicRigidBody->getGlobalPose().getInverse() * JointT;
        PxTransform WheelLocal = WheelT.getInverse() * JointT;

        WheelJoints[i] = PxRevoluteJointCreate(
            *Physics,
            Hub[0]->DynamicRigidBody, HubLocal,
            Wheels[i]->DynamicRigidBody, WheelLocal
        );
    }
    //Wheel-Hub (Rear)
    for (int i = 2; i < 4; ++i)
    {
        PxTransform WheelT = Wheels[i]->DynamicRigidBody->getGlobalPose();
        PxVec3 JointPos = WheelT.p;
        PxTransform JointT(JointPos);
        PxTransform HubLocal = Hub[1]->DynamicRigidBody->getGlobalPose().getInverse() * JointT;
        PxTransform WheelLocal = WheelT.getInverse() * JointT;

       RearHubJoints[i - 2] = PxFixedJointCreate(
            *Physics,
            Hub[1]->DynamicRigidBody, HubLocal,
            Wheels[i]->DynamicRigidBody, WheelLocal
        );
    }
    //Wheel-Body (Rear)
    for (int i = 2; i < 4; ++i)
    {
        PxTransform WheelT = Wheels[i]->DynamicRigidBody->getGlobalPose();
        PxVec3 JointPos = WheelT.p;
        PxTransform JointT(JointPos, XtoY);
        PxTransform BodyLocal = CarBody->DynamicRigidBody->getGlobalPose().getInverse() * JointT;
        PxTransform WheelLocal = WheelT.getInverse() * JointT;

        WheelJoints[i] = PxRevoluteJointCreate(
            *Physics,
            CarBody->DynamicRigidBody, BodyLocal,
            Wheels[i]->DynamicRigidBody, WheelLocal
        );
    }
    for (int i = 0; i < 4; ++i)
    {
        WheelJoints[i]->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, true);
        WheelJoints[i]->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, false);
        WheelJoints[i]->setConstraintFlag(physx::PxConstraintFlag::eVISUALIZATION, true);
        WheelJoints[i]->setDriveVelocity(0.f);
        WheelJoints[i]->setDriveForceLimit(MaxDriveTorque);
    }

    PxTransform CarBodyT = CarBody->DynamicRigidBody->getGlobalPose();
    //Body-Hub (Front, SteeringJoint)
    PxTransform FrontHubT = Hub[0]->DynamicRigidBody->getGlobalPose();
    PxTransform FrontJointT(FrontHubT.p, XtoZ);
    PxTransform BodyLocalF = CarBodyT.getInverse() * FrontJointT;
    PxTransform FrontHubLocal = FrontHubT.getInverse() * FrontJointT;

    SteeringJoint = PxRevoluteJointCreate(
        *Physics,
        CarBody->DynamicRigidBody, BodyLocalF,
        Hub[0]->DynamicRigidBody, FrontHubLocal
    );
    SteeringJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, true);
    SteeringJoint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, false);
    SteeringJoint->setLimit(PxJointAngularLimitPair(-MaxSteerAngle, +MaxSteerAngle));
    SteeringJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
    SteeringJoint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
    SteeringJoint->setConstraintFlag(physx::PxConstraintFlag::eVISUALIZATION, true);

    //초기위치 저장
    InitialBodyT = CarBodyT;
    InitialHubT[0] = Hub[0]->DynamicRigidBody->getGlobalPose();
    InitialHubT[1] = Hub[1]->DynamicRigidBody->getGlobalPose();
    for (int i = 0; i < 4; ++i)
    {
        InitialWheelT[i] = Wheels[i]->DynamicRigidBody->getGlobalPose();
    }
}

void UCarComponent::Spawn()
{
    //SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/Car/Car_RemoveWheel.obj"));
    SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/MarioKart/MarioKartBody.obj"));
    FMatrix CarMatrix = GetWorldMatrix();
    SlopeAngle = FMath::DegreesToRadians(GetComponentRotation().Pitch);
    for (int i = 0; i < 4; ++i)
    {
        FMatrix WheelLocalMatrix = FTransform(WheelPos[i]).ToMatrixWithScale();
        //FMatrix WheelWorldMatrix = WheelLocalMatrix * CarMatrix;
        AActor* Owner = GetOwner();
        WheelComp[i] = GetOwner()->AddComponent<UStaticMeshComponent>();
        WheelComp[i]->SetupAttachment(GetOwner()->GetRootComponent());
        if (i < 2)
            WheelComp[i]->SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/MarioKart/MarioKartFrontW.obj"));
        else
            WheelComp[i]->SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/MarioKart/MarioKartRearW.obj"));
        WheelComp[i]->SetRelativeTransform(FTransform(WheelPos[i]));
        //WheelComp[i]->SetRelativeRotation(WheelWorldMatrix.GetMatrixWithoutScale().ToQuat());
        //WheelComp[i]->SetWorldScale3D(WheelWorldMatrix.GetScaleVector());
    }

    BoostParticle = GetOwner()->AddComponent<UParticleSystemComponent>();
    BoostParticle->SetupAttachment(GetOwner()->GetRootComponent());

    UParticleSystem* BoostParticleSystem = Cast<UParticleSystem>(UAssetManager::Get().GetAsset(EAssetType::ParticleSystem, TEXT("Contents/ParticleSystem/Boost2")));
    if (BoostParticleSystem)
    {
        BoostParticle->SetParticleSystem(BoostParticleSystem);
        BoostParticle->SetRelativeTransform(FTransform(FRotator(0, 0, 0), FVector(-1.8f, -0.1f, 0.5f)));
        BoostParticle->bEnabled = false;
    }

    UParticleSystem* WheelDustParticleSystem = Cast<UParticleSystem>(UAssetManager::Get().GetAsset(EAssetType::ParticleSystem, TEXT("Contents/ParticleSystem/WheelDust")));
    for (int i = 0; i < 2; ++i)
    {
        WheelDustParticle[i] = GetOwner()->AddComponent<UParticleSystemComponent>();
        WheelDustParticle[i]->SetupAttachment(GetOwner()->GetRootComponent());

        if (WheelDustParticleSystem)
        {
            float y = i == 0 ? 0.9f : -0.9f;
            WheelDustParticle[i]->SetParticleSystem(WheelDustParticleSystem);
            WheelDustParticle[i]->SetRelativeTransform(FTransform(FRotator(0, 0, 0), FVector(-0.95f, y, -0.6f), FVector(0.4f, 0.4f, 0.5f)));
        }
    }
}

void UCarComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
}

void UCarComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
}

void UCarComponent::MoveCar()
{
    float CarLocX = GetComponentLocation().X;
    float EndLoc = 50 * FMath::Cos(SlopeAngle) + 130.f;

    if (!Wheels[0])
        return;

    if (GetAsyncKeyState('Q') & 0x8000 && !bBoosted)
    {
        ApplyForceToActors(SlopeAngle, MaxBoost);
        UE_LOG(ELogLevel::Display, "Boosted!: %f", FinalBoost);
        bBoosted = true;
        return;
    }

    //  FVector Up = GetUpVector();
    //if (Up.Dot(FVector(0, 0, 1)) < 0)
    //{
    //    bBoosted = true;
    //}

    if (!bBoosted)
    {
        if (GetAsyncKeyState('W') & 0x8000)
        {
            if (Velocity < 0)
                Velocity = 0;
            Velocity += 0.2f;
        }
        else
        {
            if (Velocity > 0)
                Velocity -= 0.1f;
            else if (Velocity < 0)
                Velocity += 0.1f;
            if (FMath::Abs(Velocity) < 0.1f)
                Velocity = 0.f;
        }

        if (GetAsyncKeyState('A') & 0x8000)
        {
            SteerAngle = DeltaSteerAngle;
        }
        else if (GetAsyncKeyState('D') & 0x8000)
        {
            SteerAngle = -DeltaSteerAngle;
        }
        else
        {
            float CurSteerAngle = GetCurSteerAngle();
            if (CurSteerAngle > 0.1f)
                SteerAngle = -DeltaSteerAngle;
            else if (CurSteerAngle < -0.1f)
                SteerAngle = DeltaSteerAngle;
            if (FMath::Abs(CurSteerAngle) < 0.01)
                SteerAngle = 0;
        }
    }
    else
    {
        Velocity = 0.f;

        float CurSteerAngle = GetCurSteerAngle();
        if (CurSteerAngle > 0.1f)
            SteerAngle = -DeltaSteerAngle * 3.f;
        else if (CurSteerAngle < -0.1f)
            SteerAngle = DeltaSteerAngle * 3.f;
        if (FMath::Abs(CurSteerAngle) < 0.01)
            SteerAngle = 0;

        //if (GetCurSpeed() < 5.f)
        {
            if (GetAsyncKeyState('R') & 0x8000)
                Restart();
        }
    }
    FinalBoost = FMath::Clamp(FinalBoost, 0.f, MaxBoost);

    Velocity = FMath::Clamp(Velocity, 0.f, MaxVelocity);

    for (int i = 0; i < 4; ++i)
    {
        WheelJoints[i]->setDriveVelocity(Velocity);
    }
    SteeringJoint->setDriveVelocity(SteerAngle);
}

void UCarComponent::createWheelConvexData(float radius, float halfHeight, int segmentCount, const FVector& Scale, std::vector<PxVec3>& outPoints)
{
    outPoints.clear();
    outPoints.reserve(segmentCount * 2);

    const float twoPi = PxTwoPi;
    for (int i = 0; i < segmentCount; ++i)
    {
        float theta = twoPi * float(i) / float(segmentCount);
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);

        // 상판 원형
        PxVec3 topPoint(x, halfHeight, z);
        // 하판 원형
        PxVec3 bottomPoint(x, -halfHeight, z);

        // 스케일 적용 (Unreal FVector -> PxVec3)
        topPoint.x *= Scale.X;
        topPoint.y *= Scale.Y;
        topPoint.z *= Scale.Z;

        bottomPoint.x *= Scale.X;
        bottomPoint.y *= Scale.Y;
        bottomPoint.z *= Scale.Z;

        outPoints.emplace_back(topPoint);
        outPoints.emplace_back(bottomPoint);
    }
}

PxShape* UCarComponent::CreateWheelShape(PxPhysics* Physics, PxCooking* Cooking, const FVector& Scale, int segmentCount)
{
    std::vector<PxVec3> convexPoints;
    createWheelConvexData(WheelRadius, WheelHeight, 32, Scale, convexPoints);
    PxConvexMeshDesc meshDesc;
    meshDesc.points.count = (PxU32)convexPoints.size();
    meshDesc.points.stride = sizeof(PxVec3);
    meshDesc.points.data = convexPoints.data();
    meshDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
    PxDefaultMemoryOutputStream OutputStream;
    if (!Cooking->cookConvexMesh(meshDesc, OutputStream))
    {
        return nullptr;
    }
    PxDefaultMemoryInputData InputStream(OutputStream.getData(), OutputStream.getSize());

    PxConvexMesh* ConvexMesh = Physics->createConvexMesh(InputStream);
    PxConvexMeshGeometry ConvexGeom = PxConvexMeshGeometry(ConvexMesh);

    return Physics->createShape(ConvexGeom, *DefaultMaterial);
}

void UCarComponent::UpdateFromPhysics(GameObject* PhysicsActor, UStaticMeshComponent* ActualActor)
{
    PhysicsActor->UpdateFromPhysics(GEngine->PhysicsManager->GetScene(GEngine->ActiveWorld));
    XMMATRIX Matrix = PhysicsActor->WorldMatrix;
    XMFLOAT4X4 temp;
    XMStoreFloat4x4(&temp, Matrix); // XMMATRIX → XMFLOAT4X4로 복사

    FMatrix WorldMatrix;
    memcpy(&WorldMatrix, &temp, sizeof(FMatrix)); // 안전하게 float[4][4] 복사

    FVector Location = WorldMatrix.GetTranslationVector();
    FQuat Quat = WorldMatrix.GetMatrixWithoutScale().ToQuat();
    FVector Scale = WorldMatrix.GetScaleVector();

    if (PhysicsActor == CarBody)
    {
        Location.Z -= BodyExtent.Z;
        Location.X += BodyExtent.X * 0.3f;
    }

    ActualActor->SetWorldLocation(Location);
    ActualActor->SetWorldRotation(FRotator(Quat));
}

void UCarComponent::ApplyForceToActors(float Angle, float Magnitude)
{
    PxVec3 Direction(FMath::Cos(Angle), 0, FMath::Sin(Angle));
    CarBody->DynamicRigidBody->addForce(Direction * Magnitude, PxForceMode::eIMPULSE);
    for (int i = 0; i < 4; ++i)
        Wheels[i]->DynamicRigidBody->addForce(Direction * Magnitude, PxForceMode::eIMPULSE);
    Hub[0]->DynamicRigidBody->addForce(Direction * Magnitude, PxForceMode::eIMPULSE);

    for (auto It : TObjectRange<AFreeCameraActor>())
    {
        if (It->GetWorld() == GEngine->ActiveWorld)
        {
            APlayerController* PlayerController = GEngine->ActiveWorld->GetPlayerController();
            FViewTargetTransitionParams TransitionParams;
            TransitionParams.BlendTime = 0.f; // 0.5초 동안 부드럽게 전환
            PlayerController->SetViewTarget(It, TransitionParams);
        }
    }
}

void UCarComponent::BoostCar()
{
    ApplyForceToActors(SlopeAngle, FinalBoost);
    UE_LOG(ELogLevel::Display, "Boosted!: %f", FinalBoost);
    bBoosted = true;
    return;
}

float UCarComponent::GetCurSteerAngle()
{
    PxQuat HubRot = Hub[0]->DynamicRigidBody->getGlobalPose().q;
    FQuat Quat(HubRot.x, HubRot.y, HubRot.z, HubRot.w);
    FRotator HubRotation = Quat.Rotator();
    PxQuat BodyRot = CarBody->DynamicRigidBody->getGlobalPose().q;
    Quat = FQuat(BodyRot.x, BodyRot.y, BodyRot.z, BodyRot.w);
    FRotator BodyRotation = Quat.Rotator();
    HubRotation = BodyRotation - HubRotation;
    return HubRotation.Yaw;
}

float UCarComponent::GetCurSpeed()
{
    PxVec3 CurSpeed = CarBody->DynamicRigidBody->getLinearVelocity();
    return CurSpeed.magnitude();
}

void UCarComponent::Restart()
{
    //CarBody->DynamicRigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    //Hub[0]->DynamicRigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    //Hub[1]->DynamicRigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    //for (int i = 0; i < 4; ++i)
    //{
    //    Wheels[i]->DynamicRigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    //}
    CarBody->DynamicRigidBody->setGlobalPose(InitialBodyT);
    Hub[0]->DynamicRigidBody->setGlobalPose(InitialHubT[0]);
    Hub[1]->DynamicRigidBody->setGlobalPose(InitialHubT[1]);
    for (int i = 0; i < 4; ++i)
    {
        Wheels[i]->DynamicRigidBody->setGlobalPose(InitialWheelT[i]);
    }
    Velocity = 0;
    bBoosted = false;
    FinalBoost = 0;

    CarBody->DynamicRigidBody->clearForce(PxForceMode::eIMPULSE);
    for (int i = 0; i < 4; ++i)
    {
        Wheels[i]->DynamicRigidBody->clearForce(PxForceMode::eIMPULSE);
    }
    Hub[0]->DynamicRigidBody->clearForce(PxForceMode::eIMPULSE);
    Hub[1]->DynamicRigidBody->clearForce(PxForceMode::eIMPULSE);

    CarBody->DynamicRigidBody->setLinearVelocity(PxVec3(0));
    CarBody->DynamicRigidBody->setAngularVelocity(PxVec3(0));
    for (int i = 0; i < 4; ++i)
    {
        Wheels[i]->DynamicRigidBody->setLinearVelocity(PxVec3(0));
        Wheels[i]->DynamicRigidBody->setAngularVelocity(PxVec3(0));
    }
    Hub[0]->DynamicRigidBody->setLinearVelocity(PxVec3(0));
    Hub[0]->DynamicRigidBody->setAngularVelocity(PxVec3(0));
    Hub[1]->DynamicRigidBody->setLinearVelocity(PxVec3(0));
    Hub[1]->DynamicRigidBody->setAngularVelocity(PxVec3(0));

    FireCount += 1;
}

void UCarComponent::Release()
{
    if (PxScene* Scene = GEngine->PhysicsManager->GetScene(GEngine->ActiveWorld))
    {
        Scene->removeActor(*CarBody->DynamicRigidBody);
        Scene->removeActor(*Hub[0]->DynamicRigidBody);
        Scene->removeActor(*Hub[1]->DynamicRigidBody);
        for (int i = 0; i < 4; ++i)
            Scene->removeActor(*Wheels[i]->DynamicRigidBody);
    }
    CarBody->DynamicRigidBody->release();
    CarBody->DynamicRigidBody = nullptr;
    delete CarBody;
    Hub[0]->DynamicRigidBody->release();
    Hub[0]->DynamicRigidBody = nullptr;
    delete Hub[0];
    Hub[1]->DynamicRigidBody->release();
    Hub[1]->DynamicRigidBody = nullptr;
    delete Hub[1];
    for (int i = 0; i < 4; ++i)
    {
        Wheels[i]->DynamicRigidBody->release();
        Wheels[i]->DynamicRigidBody = nullptr;
        delete Wheels[i];
    }

    DefaultMaterial->release();
    DefaultMaterial = nullptr;
    SteeringJoint->release();
    SteeringJoint = nullptr;
    for (int i = 0; i < 4; ++i)
    {
        WheelJoints[i]->release();
        WheelJoints[i] = nullptr;
    }
    RearHubJoints[0]->release();
    RearHubJoints[1]->release();
    RearHubJoints[0] = nullptr;
    RearHubJoints[1] = nullptr;
    BodyInstance->BIGameObject = nullptr; // 중요: 포인터 정리
}
