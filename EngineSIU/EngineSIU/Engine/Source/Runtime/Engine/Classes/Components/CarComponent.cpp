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

    FVector CarScale = GetOwner()->GetActorScale();
    PxQuat CarRotation = GetOwner()->GetActorRotation().Quaternion().ToPxQuat();

    //몸통
    CarBody = new GameObject();

    PxQuat BodyQuat = GetComponentRotation().Quaternion().ToPxQuat();

    BodyExtent = (AABB.MaxLocation - AABB.MinLocation) * CarScale * 0.5f;
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
        PxReal density = 1200.0f / volume;
        PxRigidBodyExt::updateMassAndInertia(*CarBody->DynamicRigidBody, density);
        Scene->addActor(*CarBody->DynamicRigidBody);
    }
    if (!WheelComp[0])
        return;
    //바퀴
    for (int i = 0; i < 4; ++i)
    {
        FVector WheelPosition = WheelComp[i]->GetComponentLocation();
        FQuat WheelQuat = WheelComp[i]->GetComponentRotation().Quaternion();
        Wheels[i] = new GameObject();
        Wheels[i]->DynamicRigidBody = Physics->createRigidDynamic(PxTransform(WheelPosition.ToPxVec3(), WheelQuat.ToPxQuat()));
        //Wheels[i]->DynamicRigidBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
        PxShape* WheelShape = CreateWheelShape(Physics, Cooking, CarScale, 32);
        WheelShape->setSimulationFilterData(PxFilterData(ECollisionChannel::ECC_Wheel, 0xFFFF, 0, 0));
        Wheels[i]->DynamicRigidBody->attachShape(*WheelShape);
        WheelShape->release();
        PxRigidBodyExt::updateMassAndInertia(*Wheels[i]->DynamicRigidBody, 10.f);
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
    for (int i = 0; i < 2; ++i)
    {
        PxTransform HubT(HubPositions[i], CarRotation);
        Hub[i] = new GameObject();
        Hub[i]->DynamicRigidBody = Physics->createRigidDynamic(HubT);
        //Hub[i]->DynamicRigidBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
        PxShape* HubShape = Physics->createShape(PxBoxGeometry(HubSize[i]), *DefaultMaterial);
        HubShape->setSimulationFilterData(PxFilterData(ECollisionChannel::ECC_Hub, 0xFFFF, 0, 0));
        Hub[i]->DynamicRigidBody->attachShape(*HubShape);
        PxRigidBodyExt::updateMassAndInertia(*Hub[i]->DynamicRigidBody, 0.01f);
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
    //Wheel-Hub (Rear
    for (int i = 2; i < 4; ++i)
    {
        PxTransform WheelT = Wheels[i]->DynamicRigidBody->getGlobalPose();
        PxVec3 JointPos = WheelT.p;
        PxTransform JointT(JointPos, XtoY);
        PxTransform HubLocal = Hub[1]->DynamicRigidBody->getGlobalPose().getInverse() * JointT;
        PxTransform WheelLocal = WheelT.getInverse() * JointT;

        WheelJoints[i] = PxRevoluteJointCreate(
            *Physics,
            Hub[1]->DynamicRigidBody, HubLocal,
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
    SteeringJoint->setConstraintFlag(physx::PxConstraintFlag::eVISUALIZATION, true);

    //Body-Hub (Rear);
    PxTransform RearHubT = Hub[1]->DynamicRigidBody->getGlobalPose();
    PxTransform RearJointT(RearHubT.p);
    PxTransform BodyLocalR = CarBodyT.getInverse() * RearJointT;
    PxTransform RearHubLocal = RearHubT.getInverse() * RearJointT;
    PxFixedJoint* FixedJoint = PxFixedJointCreate(
        *Physics,
        CarBody->DynamicRigidBody, BodyLocalR,
        Hub[1]->DynamicRigidBody, RearHubLocal
    );
    FixedJoint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, false);
    FixedJoint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);
}

void UCarComponent::Spawn()
{
    FVector CarPos = GetOwner()->GetActorLocation();
    FVector CarScale = GetOwner()->GetActorScale();
    SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/Car/Car_RemoveWheel.obj"));
    SetWorldLocation(CarBodyPos + CarPos);
    SetWorldScale3D(CarScale);
    for (int i = 0; i < 4; ++i)
    {
        AActor* Owner = GetOwner();
        WheelComp[i] = GetOwner()->AddComponent<UStaticMeshComponent>();
        WheelComp[i]->SetupAttachment(GetOwner()->GetRootComponent());
        WheelComp[i]->SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/Car/tire.obj"));
        WheelComp[i]->SetWorldLocation(WheelPos[i] + CarPos);
        WheelComp[i]->SetWorldScale3D(CarScale);
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

    XMVECTOR scale, rotation, translation;
    XMMatrixDecompose(&scale, &rotation, &translation, Matrix);

    //위치 추출
    XMFLOAT3 pos;
    XMStoreFloat3(&pos, translation);

    // 쿼터니언에서 오일러 각도로 변환
    XMFLOAT4 quat;
    XMStoreFloat4(&quat, rotation);

    FQuat MyQuat = FQuat(quat.x, quat.y, quat.z, quat.w);
    FRotator Rotator = MyQuat.Rotator();

    // Engine에 적용 (라디안 → 도 변환)
    ActualActor->SetWorldLocation(FVector(pos.x, -pos.y, pos.z));
    ActualActor->SetWorldRotation(Rotator);
}
