#include "CarComponent.h"
#include "Engine/FObjLoader.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"

UCarComponent::UCarComponent()
{
    bSimulate = true;
}

UObject* UCarComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    return NewComponent;
}

void UCarComponent::TickComponent(float DeltaTime)
{
    MoveCar();
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
        Comp->DestroyComponent();
    }
    Spawn();

    PxPhysics* Physics = GEngine->PhysicsManager->GetPhysics();
    PxCooking* Cooking = GEngine->PhysicsManager->GetCooking();
    PxScene* Scene = GEngine->PhysicsManager->GetScene(GEngine->ActiveWorld);
    if (!DefaultMaterial)
        DefaultMaterial = Physics->createMaterial(1.f, 0.9f, 0.f);

    PxQuat CarRotation = GetOwner()->GetActorRotation().Quaternion().ToPxQuat();

    //몸통    
    CarBody = new GameObject();

    PxQuat BodyQuat = GetComponentRotation().Quaternion().ToPxQuat();

    BodyExtent = (AABB.MaxLocation - AABB.MinLocation) * GetComponentScale3D() * 0.5f;
    BodyExtent.Z *= 0.5f;
    PxBoxGeometry CarBodyGeom(BodyExtent.ToPxVec3());
    CarBody->DynamicRigidBody = Physics->createRigidDynamic(PxTransform(GetComponentLocation().ToPxVec3(), BodyQuat));
    //CarBody->DynamicRigidBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
    {
        PxShape* BodyShape = Physics->createShape(CarBodyGeom, *DefaultMaterial);
        BodyShape->setSimulationFilterData(PxFilterData(ECollisionChannel::ECC_CarBody, 0xFFFF, 0, 0));
        CarBody->DynamicRigidBody->attachShape(*BodyShape);
        BodyShape->release();
        PxReal volume = 8 * BodyExtent.X * BodyExtent.Y * BodyExtent.Z;
        PxReal density = 600.0f / volume;
        PxRigidBodyExt::updateMassAndInertia(*CarBody->DynamicRigidBody, density);

        //PxVec3 inertia = CarBody->DynamicRigidBody->getMassSpaceInertiaTensor();
        //// 예시로 yaw축 관성만 50%로 줄이려면
        //inertia.z *= 0.5f;
        //CarBody->DynamicRigidBody->setMassSpaceInertiaTensor(inertia);

        CarBody->DynamicRigidBody->setLinearDamping(0.1f);
        CarBody->DynamicRigidBody->setAngularDamping(0.1f);
        CarBody->DynamicRigidBody->setSolverIterationCounts(  
            /*minPositionIters=*/12,
            /*minVelocityIters=*/6
        );
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
        PxShape* WheelShape = CreateWheelShape(Physics, Cooking, WheelScale, 32);
        WheelShape->setSimulationFilterData(PxFilterData(ECollisionChannel::ECC_Wheel, 0xFFFF, 0, 0));
        Wheels[i]->DynamicRigidBody->attachShape(*WheelShape);
        WheelShape->release();
        PxRigidBodyExt::updateMassAndInertia(*Wheels[i]->DynamicRigidBody, 10.f);
        Wheels[i]->DynamicRigidBody->setLinearDamping(0.05f);
        Wheels[i]->DynamicRigidBody->setAngularDamping(0.05f);
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
        PxVec3(0.2f, (WheelT[0].p.y - WheelT[1].p.y) * 0.5f, 0.2f),
        PxVec3(0.2f, (WheelT[2].p.y - WheelT[3].p.y) * 0.5f, 0.2f)
    };
    for (int i = 0; i < 1; ++i)
    {
        PxTransform HubT(HubPositions[i], CarRotation);
        Hub[i] = new GameObject();
        Hub[i]->DynamicRigidBody = Physics->createRigidDynamic(HubT);
        //Hub[i]->DynamicRigidBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
        PxShape* HubShape = Physics->createShape(PxBoxGeometry(HubSize[i]), *DefaultMaterial);
        HubShape->setSimulationFilterData(PxFilterData(ECollisionChannel::ECC_Hub, 0xFFFF, 0, 0));
        Hub[i]->DynamicRigidBody->attachShape(*HubShape);
        PxReal HubVolume = 8 * HubSize[0].x * HubSize[0].y * HubSize[0].z;
        PxReal HubMass = 50.f;
        PxRigidBodyExt::updateMassAndInertia(*Hub[i]->DynamicRigidBody, HubMass/HubVolume);
        Hub[i]->DynamicRigidBody->setLinearDamping(0.05f);
        Hub[i]->DynamicRigidBody->setAngularDamping(0.05f);
        Hub[i]->DynamicRigidBody->setSolverIterationCounts(
            /*minPositionIters=*/12,
            /*minVelocityIters=*/6
        );
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

    ////Body-Hub (Rear);
    //PxTransform RearHubT = Hub[1]->DynamicRigidBody->getGlobalPose();
    //PxTransform RearJointT(RearHubT.p);
    //PxTransform BodyLocalR = CarBodyT.getInverse() * RearJointT;
    //PxTransform RearHubLocal = RearHubT.getInverse() * RearJointT;
    //PxFixedJoint* FixedJoint = PxFixedJointCreate(
    //    *Physics,
    //    CarBody->DynamicRigidBody, BodyLocalR,
    //    Hub[1]->DynamicRigidBody, RearHubLocal
    //);
    //FixedJoint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, false);
    //FixedJoint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);
}

void UCarComponent::Spawn()
{
    SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/Car/Car_RemoveWheel.obj"));
    FMatrix CarMatrix = GetWorldMatrix();

    for (int i = 0; i < 4; ++i)
    {
        FMatrix WheelLocalMatrix = FTransform(WheelPos[i]).ToMatrixWithScale();
        //FMatrix WheelWorldMatrix = WheelLocalMatrix * CarMatrix;
        AActor* Owner = GetOwner();
        WheelComp[i] = GetOwner()->AddComponent<UStaticMeshComponent>();
        WheelComp[i]->SetupAttachment(GetOwner()->GetRootComponent());
        WheelComp[i]->SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/Car/tire.obj"));
        WheelComp[i]->SetRelativeTransform(FTransform(WheelPos[i] - FVector(0, 0, 1.5f)));
        //WheelComp[i]->SetRelativeRotation(WheelWorldMatrix.GetMatrixWithoutScale().ToQuat());
        //WheelComp[i]->SetWorldScale3D(WheelWorldMatrix.GetScaleVector());
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
    PxTransform CarT = CarBody->DynamicRigidBody->getGlobalPose();
    if (CarT.p.x > 75.f && !bBoosted)
    {
        float Angle = GetComponentRotation().Pitch;
        ApplyForceToActors(PxPi / 6.f, FinalBoost);
        bBoosted = true;
        return;
    }

    if (!Wheels[0] || bBoosted)
        return;
    if (GetAsyncKeyState('W') & 0x8000)
    {
        //if (Velocity < 0)
        //    Velocity = 0;
        //Velocity += 0.1f;
        FinalBoost += 2.5f;
        Velocity = 20.f;
    }
    //else if (GetAsyncKeyState('S') & 0x8000)
    //{
    //    //if (Velocity > 0)
    //    //    Velocity = 0;
    //    //Velocity -= 0.1f;
    //    FinalBoost -= 2.5f;
    //    Velocity = -20.f;
    //}
    else
    {
        if (Velocity > 0)
            Velocity -= 0.1f;
        else if (Velocity < 0)
            Velocity += 0.1f;
        if (FMath::Abs(Velocity) < 0.1f)
            Velocity = 0.f;
        FinalBoost -= 1.f;
    }
    if (FinalBoost > 0.f)
    {
        UE_LOG(ELogLevel::Display, "Boost Before Clamp: %f", FinalBoost);
        FinalBoost = FMath::Clamp(FinalBoost, 0.f, MaxBoost);
        UE_LOG(ELogLevel::Display, "Boost After Clamp: %f", FinalBoost);
    }

    for (int i = 0; i < 4; ++i)
    {
        WheelJoints[i]->setDriveVelocity(Velocity);
    }

    //PxTransform TargetPose;
    if (GetAsyncKeyState('A') & 0x8000)
    {
        SteeringJoint->setDriveVelocity(DeltaSteerAngle * 3.f);
    }
    else if (GetAsyncKeyState('D') & 0x8000)
    {
        SteeringJoint->setDriveVelocity(-DeltaSteerAngle * 3.f);
    }
    else
    {
        SteeringJoint->setDriveVelocity(0.f);
    }
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
}
