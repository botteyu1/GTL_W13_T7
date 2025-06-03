#include "PhysicsManager.h"

#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/ConstraintInstance.h"
#include "PhysicsEngine/PhysicsAsset.h"

#include "World/World.h"
#include <thread>

#include "SimulationEventCallback.h"


void GameObject::SetRigidBodyType(ERigidBodyType RigidBodyType) const
{
    switch (RigidBodyType)
    {
    case ERigidBodyType::STATIC:
    {
        // TODO: 재생성해야 함, BodySetup은 어디서 받아오지?
        break;
    }
        case ERigidBodyType::DYNAMIC:
    {
        DynamicRigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
        break;
    }
        case ERigidBodyType::KINEMATIC:
    {
        DynamicRigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        DynamicRigidBody->setLinearVelocity(PxVec3(0));
        DynamicRigidBody->setAngularVelocity(PxVec3(0));
        break;
    }
    }
}

FPhysicsManager::FPhysicsManager()
{
}

void FPhysicsManager::InitPhysX()
{
    Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, Allocator, ErrorCallback);

    // PVD 생성 및 연결
    Pvd = PxCreatePvd(*Foundation);
    if (Pvd) {
        // TCP 연결 (기본 포트 5425)
        Transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
        if (Transport) {
            Pvd->connect(*Transport, PxPvdInstrumentationFlag::eALL);
        }
    }
    
    Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *Foundation, PxTolerancesScale(), true, Pvd);
    
    Material = Physics->createMaterial(0.5f, 0.5f, 0.0f);

    PxInitExtensions(*Physics, Pvd);

    Cooking = PxCreateCooking(PX_PHYSICS_VERSION, *Foundation, PxCookingParams(Physics->getTolerancesScale()));
}

PxScene* FPhysicsManager::CreateScene(UWorld* World)
{
    if (SceneMap[World])
    {
        // PVD 클라이언트 생성 및 씬 연결
        if (Pvd && Pvd->isConnected()) {
            PxPvdSceneClient* pvdClient = SceneMap[World]->getScenePvdClient();
            if (pvdClient) {
                pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
                pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
                pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
            }
        }
        return SceneMap[World];
    }
    
    PxSceneDesc SceneDesc(Physics->getTolerancesScale());
    
    SceneDesc.gravity = PxVec3(0, 0, -20.f);
    
    unsigned int hc = std::thread::hardware_concurrency();
    Dispatcher = PxDefaultCpuDispatcherCreate(hc-2);
    SceneDesc.cpuDispatcher = Dispatcher;
    
    SceneDesc.filterShader = MySimulationFilterShader;
    
    // sceneDesc.simulationEventCallback = gMyCallback; // TODO: 이벤트 핸들러 등록(옵저버 or component 별 override)
    
    SceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    SceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
    SceneDesc.flags |= PxSceneFlag::eENABLE_PCM;

    if (!SimEventCallback) // SimEventCallback은 FPhysicsManager의 멤버 변수
    {
        SimEventCallback = new FSimulationEventCallback();
    }
    SceneDesc.simulationEventCallback = SimEventCallback; // 콜백 등록
    
    PxScene* NewScene = Physics->createScene(SceneDesc);
    SceneMap.Add(World, NewScene);

    // PVD 클라이언트 생성 및 씬 연결
    if (Pvd && Pvd->isConnected()) {
        PxPvdSceneClient* pvdClient = NewScene->getScenePvdClient();
        if (pvdClient) {
            pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
            pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
            pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        }
    }

    return NewScene;
}

bool FPhysicsManager::ConnectPVD()
{
    // PVD 생성
    Pvd = PxCreatePvd(*Foundation);
    if (!Pvd) {
        printf("PVD 생성 실패\n");
        return false;
    }

    // 네트워크 전송 생성 (TCP)
    Transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
    if (!Transport) {
        printf("PVD Transport 생성 실패\n");
        return false;
    }

    // PVD 연결 및 계측 플래그 설정
    bool connected = Pvd->connect(*Transport, 
        PxPvdInstrumentationFlag::eALL);  // 모든 데이터 전송
    // 또는 특정 플래그만:
    // PxPvdInstrumentationFlag::eDEBUG | 
    // PxPvdInstrumentationFlag::ePROFILE |
    // PxPvdInstrumentationFlag::eMEMORY

    if (connected) {
        printf("PVD 연결 성공\n");
    } else {
        printf("PVD 연결 실패\n");
    }

    return connected;
}


GameObject FPhysicsManager::CreateBox(const PxVec3& Pos, const PxVec3& HalfExtents) const
{
    GameObject Obj;
    
    PxTransform Pose(Pos);
    Obj.DynamicRigidBody = Physics->createRigidDynamic(Pose);
    
    PxShape* Shape = Physics->createShape(PxBoxGeometry(HalfExtents), *Material);
    physx::PxFilterData defaultFilterData;
    defaultFilterData.word0 = 0; // 모든 그룹에 속함 (또는 특정 '일반 객체' 그룹)
    defaultFilterData.word1 = 0; // 모든 그룹과 충돌함
    defaultFilterData.word2 = 0; // 사용자 정의 필터링을 위한 비트
    defaultFilterData.word3 = 0; // 사용자 정의 필터링을 위한 비트
    Shape->setSimulationFilterData(defaultFilterData);
    Obj.DynamicRigidBody->attachShape(*Shape);
    
    PxRigidBodyExt::updateMassAndInertia(*Obj.DynamicRigidBody, 10.0f);
    CurrentScene->addActor(*Obj.DynamicRigidBody);
    
    Obj.UpdateFromPhysics(CurrentScene);
    
    return Obj;
}

GameObject* FPhysicsManager::CreateGameObject(const PxVec3& Pos, const PxQuat& Rot, FBodyInstance* BodyInstance, UBodySetup* BodySetup, ERigidBodyType RigidBodyType) const
{
    GameObject* Obj = new GameObject();
    
    // RigidBodyType에 따라 다른 타입의 Actor 생성
    switch (RigidBodyType)
    {
    case ERigidBodyType::STATIC:
    {
        Obj->StaticRigidBody = CreateStaticRigidBody(Pos, Rot, BodyInstance, BodySetup);
        ApplyBodyInstanceSettings(Obj->StaticRigidBody, BodyInstance);
        break;
    }
    case ERigidBodyType::DYNAMIC:
    {
        Obj->DynamicRigidBody = CreateDynamicRigidBody(Pos, Rot, BodyInstance, BodySetup);
        ApplyBodyInstanceSettings(Obj->DynamicRigidBody, BodyInstance);
        break;
    }
    case ERigidBodyType::KINEMATIC:
    {
        Obj->DynamicRigidBody = CreateDynamicRigidBody(Pos, Rot, BodyInstance, BodySetup);
        Obj->DynamicRigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        ApplyBodyInstanceSettings(Obj->DynamicRigidBody, BodyInstance);
        break;
    }
    }

    BodyInstance->BIGameObject = Obj;
    
    return Obj;
}

PxRigidDynamic* FPhysicsManager::CreateDynamicRigidBody(const PxVec3& Pos, const PxQuat& Rot, FBodyInstance* BodyInstance, UBodySetup* BodySetup) const
{
    const PxTransform Pose(Pos, Rot);
    PxRigidDynamic* DynamicRigidBody = Physics->createRigidDynamic(Pose);
    
    // Shape 추가
    AttachShapesToActor(DynamicRigidBody, BodySetup);
    
    // 질량과 관성 설정
    ApplyMassAndInertiaSettings(DynamicRigidBody, BodyInstance);
    
    // Scene에 추가
    CurrentScene->addActor(*DynamicRigidBody);
    DynamicRigidBody->userData = (void*)BodyInstance;

    return DynamicRigidBody;
}

PxRigidStatic* FPhysicsManager::CreateStaticRigidBody(const PxVec3& Pos, const PxQuat& Rot, FBodyInstance* BodyInstance, UBodySetup* BodySetup) const
{
    const PxTransform Pose(Pos, Rot);
    PxRigidStatic* StaticRigidBody = Physics->createRigidStatic(Pose);
    
    // Shape 추가
    AttachShapesToActor(StaticRigidBody, BodySetup);
    
    // Scene에 추가
    CurrentScene->addActor(*StaticRigidBody);
    StaticRigidBody->userData = (void*)BodyInstance;

    return StaticRigidBody;
}

// === Shape 추가 헬퍼 함수 ===
void FPhysicsManager::AttachShapesToActor(PxRigidActor* Actor, UBodySetup* BodySetup) const
{
    if (!BodySetup)return;
    // Sphere 추가
    for (const auto& Sphere : BodySetup->AggGeom.SphereElems)
    {
    
        Actor->attachShape(*Sphere);
    }

    // Box 추가
    for (const auto& Box : BodySetup->AggGeom.BoxElems)
    {
        Actor->attachShape(*Box);
    }

    // Capsule 추가
    for (const auto& Capsule : BodySetup->AggGeom.CapsuleElems)
    {
        Actor->attachShape(*Capsule);
    }
}

// === 질량과 관성 설정 ===
void FPhysicsManager::ApplyMassAndInertiaSettings(PxRigidDynamic* DynamicBody, const FBodyInstance* BodyInstance) const
{
    // 기본 질량 설정
    if (BodyInstance->MassInKg > 0.0f)
    {
        PxRigidBodyExt::setMassAndUpdateInertia(*DynamicBody, BodyInstance->MassInKg);
    }
    else
    {
        // 기본 밀도로 질량 계산
        PxRigidBodyExt::updateMassAndInertia(*DynamicBody, 1000.0f); // 기본 밀도
    }
    
    // // 질량 중심 오프셋 적용
    // if (!BodyInstance->COMNudge.IsZero())
    // {
    //     // PxVec3 COMOffset(BodyInstance->COMNudge.X, BodyInstance->COMNudge.Y, BodyInstance->COMNudge.Z);
    //     // PxRigidBodyExt::setMassAndUpdateInertia(*DynamicBody, 1.0f);
    //     
    //     float newMass = 1.0f;
    //     DynamicBody->setMass(newMass);
    //
    //     // 🔸 문제 2: MassSpaceInertiaTensor: 0.000, 0.000, 0.000 해결
    //     // 구체의 경우 (반지름 1.0f 가정)
    //     float radius = 1.0f;
    //     float I = (2.0f / 5.0f) * newMass * radius * radius; // = 0.4f
    //     PxVec3 inertia(I, I, I);
    //     DynamicBody->setMassSpaceInertiaTensor(inertia);
    // }
    //
    // 관성 텐서 스케일 적용
    if (BodyInstance->InertiaTensorScale != FVector::OneVector)
    {
        PxVec3 Inertia = DynamicBody->getMassSpaceInertiaTensor();
        Inertia.x *= BodyInstance->InertiaTensorScale.X;
        Inertia.y *= BodyInstance->InertiaTensorScale.Y;
        Inertia.z *= BodyInstance->InertiaTensorScale.Z;
        DynamicBody->setMassSpaceInertiaTensor(Inertia);
    }
}

// === 모든 BodyInstance 설정 적용 ===
void FPhysicsManager::ApplyBodyInstanceSettings(PxRigidActor* Actor, const FBodyInstance* BodyInstance) const
{
    PxRigidDynamic* DynamicBody = Actor->is<PxRigidDynamic>();

    if (DynamicBody)
    {
        // === 중력 설정 (Toe 포함 시에만 켬) ===
        bool bGravityAllowed = BodyInstance->bEnableGravity;
        FString BoneNameStr = BodyInstance->BodyInstanceName.ToString();
        if (bGravityAllowed && (!BoneNameStr.Contains(TEXT("mix"))||BoneNameStr.Contains(TEXT("Toe"))))
        {
            DynamicBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, false); // 중력 적용
        }
        else
        {
            DynamicBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);  // 중력 비활성화
        }
        //DynamicBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);  // 중력 비활성화

        // 기존 코드 유지
        if (!BodyInstance->bSimulatePhysics)
        {
            DynamicBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        }

        if (!BodyInstance->bStartAwake)
        {
            DynamicBody->putToSleep();
        }

        ApplyLockConstraints(DynamicBody, BodyInstance);
        DynamicBody->setLinearDamping(BodyInstance->LinearDamping);
        DynamicBody->setAngularDamping(BodyInstance->AngularDamping);
        DynamicBody->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, BodyInstance->bUseCCD);

        if (BodyInstance->MaxAngularVelocity > 0.0f)
        {
            DynamicBody->setMaxAngularVelocity(BodyInstance->MaxAngularVelocity);
        }

        DynamicBody->setSolverIterationCounts(
            BodyInstance->PositionSolverIterationCount,
            BodyInstance->VelocitySolverIterationCount
        );
    }

    ApplyCollisionSettings(Actor, BodyInstance);
}
void FPhysicsManager::EnableGravityForAllBodies(UWorld* World)
{
    PxScene* Scene = nullptr;
    if (PxScene** FoundScene = SceneMap.Find(World))
    {
        Scene = *FoundScene;
    }
    else
    {
        return; // 또는 nullptr 반환 대신 다른 처리
    }

    PxU32 ActorCount = Scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
    if (ActorCount == 0) return;

    TArray<PxActor*> Actors;
    Actors.SetNum(ActorCount); // SetNumUninitialized → SetNum 로 변경
    Scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC, Actors.GetData(), ActorCount);

    for (PxActor* Actor : Actors)
    {
        PxRigidDynamic* Dyn = Actor->is<PxRigidDynamic>();
        if (Dyn)
        {
            Dyn->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, false); // 중력 활성화
        }
    }
}


// === 움직임 제한 적용 ===
void FPhysicsManager::ApplyLockConstraints(PxRigidDynamic* DynamicBody, const FBodyInstance* BodyInstance) const
{
    PxRigidDynamicLockFlags LockFlags = PxRigidDynamicLockFlags(0);
    
    // 이동 제한
    if (BodyInstance->bLockXTranslation) LockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_X;
    if (BodyInstance->bLockYTranslation) LockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Y;
    if (BodyInstance->bLockZTranslation) LockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Z;
    
    // 회전 제한
    if (BodyInstance->bLockXRotation) LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_X;
    if (BodyInstance->bLockYRotation) LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y;
    if (BodyInstance->bLockZRotation) LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z;
    
    DynamicBody->setRigidDynamicLockFlags(LockFlags);
}

// === 충돌 설정 적용 ===
void FPhysicsManager::ApplyCollisionSettings(const PxRigidActor* Actor, const FBodyInstance* BodyInstance) const
{
    // 모든 Shape에 대해 충돌 설정 적용
    PxU32 ShapeCount = Actor->getNbShapes();
    TArray<PxShape*> Shapes;
    Shapes.SetNum(ShapeCount);
    Actor->getShapes(Shapes.GetData(), ShapeCount);
    
    for (PxShape* Shape : Shapes)
    {
        if (Shape)
        {
            ApplyShapeCollisionSettings(Shape, BodyInstance);
        }
    }
}

void FPhysicsManager::ApplyShapeCollisionSettings(PxShape* Shape, const FBodyInstance* BodyInstance) const
{
    PxShapeFlags ShapeFlags = Shape->getFlags();
    
    // 기존 플래그 초기화
    ShapeFlags &= ~(PxShapeFlag::eSIMULATION_SHAPE | PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eTRIGGER_SHAPE);
    
    // CollisionEnabled에 따른 플래그 설정
    switch (BodyInstance->CollisionEnabled)
    {
    case ECollisionEnabled::NoCollision:
        // 모든 충돌 비활성화
        break;
        
    case ECollisionEnabled::QueryOnly:
        // 쿼리만 활성화 (트레이스, 오버랩)
        ShapeFlags |= PxShapeFlag::eSCENE_QUERY_SHAPE;
        break;
        
    case ECollisionEnabled::PhysicsOnly:
        // 물리 시뮬레이션만 활성화
        ShapeFlags |= PxShapeFlag::eSIMULATION_SHAPE;
        break;
        
    case ECollisionEnabled::QueryAndPhysics:
        // 둘 다 활성화
        ShapeFlags |= (PxShapeFlag::eSIMULATION_SHAPE | PxShapeFlag::eSCENE_QUERY_SHAPE);
        break;
    }
    
    // 복잡한 충돌을 단순 충돌로 사용 설정
    if (BodyInstance->bUseComplexAsSimpleCollision)
    {
        // 복잡한 메시를 단순 충돌로 사용하는 로직
        // (구체적인 구현은 메시 충돌 시스템에 따라 다름)
    }

    //일단 모든 충돌 활성화
    physx::PxFilterData defaultFilterData;
    defaultFilterData.word0 = 0; // 모든 그룹에 속함 (또는 특정 '일반 객체' 그룹)
    defaultFilterData.word1 = 0; // 모든 그룹과 충돌함
    defaultFilterData.word2 = 0; // 사용자 정의 필터링을 위한 비트
    defaultFilterData.word3 = 0; // 사용자 정의 필터링을 위한 비트
    Shape->setSimulationFilterData(defaultFilterData);
    
    Shape->setFlags(ShapeFlags);
}

// // === 런타임 설정 변경 함수들 === TODO : 필요하면 GameObject 안으로 옮기기
//
// void FPhysicsManager::SetSimulatePhysics(GameObject* Obj, bool bSimulate) const
// {
//     if (Obj && Obj->DynamicRigidBody)
//     {
//         Obj->DynamicRigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, !bSimulate);
//     }
// }
//
// void FPhysicsManager::SetEnableGravity(GameObject* Obj, bool bEnableGravity) const
// {
//     if (Obj && Obj->DynamicRigidBody)
//     {
//         Obj->DynamicRigidBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !bEnableGravity);
//     }
// }
//
// void FPhysicsManager::SetMass(GameObject* Obj, float NewMass) const
// {
//     if (Obj && Obj->DynamicRigidBody && NewMass > 0.0f)
//     {
//         PxRigidBodyExt::setMassAndUpdateInertia(*Obj->DynamicRigidBody, NewMass);
//     }
// }
//
// void FPhysicsManager::SetLinearDamping(GameObject* Obj, float Damping) const
// {
//     if (Obj && Obj->DynamicRigidBody)
//     {
//         Obj->DynamicRigidBody->setLinearDamping(Damping);
//     }
// }
//
// void FPhysicsManager::SetAngularDamping(GameObject* Obj, float Damping) const
// {
//     if (Obj && Obj->DynamicRigidBody)
//     {
//         Obj->DynamicRigidBody->setAngularDamping(Damping);
//     }
// }

void FPhysicsManager::CreateJoint(const GameObject* Obj1, const GameObject* Obj2, FConstraintInstance* ConstraintInstance, const FConstraintSetup* ConstraintSetup) const
{
    PxTransform GlobalPose1 = Obj1->DynamicRigidBody->getGlobalPose();
    PxTransform GlobalPose2 = Obj2->DynamicRigidBody->getGlobalPose();
        
    PxTransform LocalFrameParent = GlobalPose1.getInverse() * GlobalPose2;
    PxTransform LocalFrameChild = PxTransform(PxVec3(0));

    // PhysX D6 Joint 생성
    PxD6Joint* Joint = PxD6JointCreate(*Physics,
        Obj1->DynamicRigidBody, LocalFrameParent,
        Obj2->DynamicRigidBody, LocalFrameChild);

    if (Joint && ConstraintSetup)
    {
        // === Linear Constraint 설정 ===
        
        // X축 선형 제약
        switch (ConstraintSetup->LinearLimit.XMotion)
        {
        case ELinearConstraintMotion::LCM_Free:
            Joint->setMotion(PxD6Axis::eX, PxD6Motion::eFREE);
            break;
        case ELinearConstraintMotion::LCM_Limited:
            Joint->setMotion(PxD6Axis::eX, PxD6Motion::eLIMITED);
            break;
        case ELinearConstraintMotion::LCM_Locked:
            Joint->setMotion(PxD6Axis::eX, PxD6Motion::eLOCKED);
            break;
        }

        // Y축 선형 제약
        switch (ConstraintSetup->LinearLimit.YMotion)
        {
        case ELinearConstraintMotion::LCM_Free:
            Joint->setMotion(PxD6Axis::eY, PxD6Motion::eFREE);
            break;
        case ELinearConstraintMotion::LCM_Limited:
            Joint->setMotion(PxD6Axis::eY, PxD6Motion::eLIMITED);
            break;
        case ELinearConstraintMotion::LCM_Locked:
            Joint->setMotion(PxD6Axis::eY, PxD6Motion::eLOCKED);
            break;
        }

        // Z축 선형 제약
        switch (ConstraintSetup->LinearLimit.ZMotion)
        {
        case ELinearConstraintMotion::LCM_Free:
            Joint->setMotion(PxD6Axis::eZ, PxD6Motion::eFREE);
            break;
        case ELinearConstraintMotion::LCM_Limited:
            Joint->setMotion(PxD6Axis::eZ, PxD6Motion::eLIMITED);
            break;
        case ELinearConstraintMotion::LCM_Locked:
            Joint->setMotion(PxD6Axis::eZ, PxD6Motion::eLOCKED);
            break;
        }

        // 선형 제한 값 설정 (Limited 모션이 하나라도 있을 때)
        if (ConstraintSetup->LinearLimit.XMotion == ELinearConstraintMotion::LCM_Limited ||
            ConstraintSetup->LinearLimit.YMotion == ELinearConstraintMotion::LCM_Limited ||
            ConstraintSetup->LinearLimit.ZMotion == ELinearConstraintMotion::LCM_Limited)
        {
            float LinearLimitValue = ConstraintSetup->LinearLimit.Limit;
            Joint->setLinearLimit(PxJointLinearLimit(LinearLimitValue, PxSpring(0, 0)));
        }

        // === Angular Constraint 설정 ===

        // Twist 제약 (X축 회전)
        switch (ConstraintSetup->TwistLimit.TwistMotion)
        {
        case EAngularConstraintMotion::EAC_Free:
            Joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
            break;
        case EAngularConstraintMotion::EAC_Limited:
            Joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);
            break;
        case EAngularConstraintMotion::EAC_Locked:
            Joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLOCKED);
            break;
        }

        // Swing1 제약 (Y축 회전)
        switch (ConstraintSetup->ConeLimit.Swing1Motion)
        {
        case EAngularConstraintMotion::EAC_Free:
            Joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
            break;
        case EAngularConstraintMotion::EAC_Limited:
            Joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
            break;
        case EAngularConstraintMotion::EAC_Locked:
            Joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLOCKED);
            break;
        }

        // Swing2 제약 (Z축 회전)
        switch (ConstraintSetup->ConeLimit.Swing2Motion)
        {
        case EAngularConstraintMotion::EAC_Free:
            Joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);
            break;
        case EAngularConstraintMotion::EAC_Limited:
            Joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);
            break;
        case EAngularConstraintMotion::EAC_Locked:
            Joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLOCKED);
            break;
        }

        // === Angular Limit 값 설정 ===

        // Twist 제한 값 설정 (도를 라디안으로 변환)
        if (ConstraintSetup->TwistLimit.TwistMotion == EAngularConstraintMotion::EAC_Limited)
        {
            float TwistLimitRad = FMath::DegreesToRadians(ConstraintSetup->TwistLimit.TwistLimitDegrees);
            Joint->setTwistLimit(PxJointAngularLimitPair(-TwistLimitRad, TwistLimitRad, PxSpring(0, 0)));
        }

        // Swing 제한 값 설정 (원뿔 제한)
        if (ConstraintSetup->ConeLimit.Swing1Motion == EAngularConstraintMotion::EAC_Limited ||
            ConstraintSetup->ConeLimit.Swing2Motion == EAngularConstraintMotion::EAC_Limited)
        {
            float Swing1LimitRad = FMath::DegreesToRadians(ConstraintSetup->ConeLimit.Swing1LimitDegrees);
            float Swing2LimitRad = FMath::DegreesToRadians(ConstraintSetup->ConeLimit.Swing2LimitDegrees);
            Joint->setSwingLimit(PxJointLimitCone(Swing1LimitRad, Swing2LimitRad, PxSpring(0, 0)));
        }

        // === Drive/Motor 설정 ===

        // Linear Position Drive
        if (ConstraintSetup->bLinearPositionDrive)
        {
            PxD6JointDrive LinearDrive;
            LinearDrive.stiffness = 1000.0f;     // 강성 (조정 가능)
            LinearDrive.damping = 100.0f;        // 댐핑 (조정 가능)
            LinearDrive.forceLimit = PX_MAX_F32;  // 힘 제한
            LinearDrive.flags = PxD6JointDriveFlag::eACCELERATION;

            Joint->setDrive(PxD6Drive::eX, LinearDrive);
            Joint->setDrive(PxD6Drive::eY, LinearDrive);
            Joint->setDrive(PxD6Drive::eZ, LinearDrive);
        }

        // Angular Position Drive (Slerp)
        if (ConstraintSetup->bAngularOrientationDrive)
        {
            PxD6JointDrive AngularDrive;
            AngularDrive.stiffness = 1000.0f;
            AngularDrive.damping = 100.0f;
            AngularDrive.forceLimit = PX_MAX_F32;
            AngularDrive.flags = PxD6JointDriveFlag::eACCELERATION;

            Joint->setDrive(PxD6Drive::eSLERP, AngularDrive);
        }

        // Angular Velocity Drive
        if (ConstraintSetup->bAngularVelocityDrive)
        {
            PxD6JointDrive AngularVelDrive;
            AngularVelDrive.stiffness = 0.0f;     // 속도 제어시 강성은 0
            AngularVelDrive.damping = 100.0f;
            AngularVelDrive.forceLimit = PX_MAX_F32;
            AngularVelDrive.flags = PxD6JointDriveFlag::eACCELERATION;

            Joint->setDrive(PxD6Drive::eTWIST, AngularVelDrive);
            Joint->setDrive(PxD6Drive::eSWING, AngularVelDrive);
        }

        // === 기본 조인트 설정 ===
        
        // 조인트 이름 설정 (디버깅용)
        if (!ConstraintSetup->JointName.IsEmpty())
        {
            Joint->setName(GetData(ConstraintSetup->JointName));
        }

        // 조인트 활성화
        Joint->setConstraintFlags(PxConstraintFlag::eVISUALIZATION);
        
        // 파괴 임계값 설정 (선택사항)
        Joint->setBreakForce(PX_MAX_F32, PX_MAX_F32);  // 무한대로 설정하여 파괴되지 않음
    }

    ConstraintInstance->ConstraintData = Joint;
}

void FPhysicsManager::MarkGameObjectForKill(GameObject* InGameObject)
{
    if (InGameObject)
    {
        PendingKillGameObjects.FindOrAdd(CurrentScene).AddUnique(InGameObject); // 중복 추가 방지
    }
}


void FPhysicsManager::DestroyGameObject(GameObject* GameObject) const
{
    // TODO: StaticRigidBody 분기 처리 필요
    if (GameObject && GameObject->DynamicRigidBody)
    {
        CurrentScene->removeActor(*GameObject->DynamicRigidBody);
        GameObject->DynamicRigidBody->release();
        GameObject->DynamicRigidBody = nullptr;
    }
    delete GameObject;
}

void FPhysicsManager::ProcessPendingKills()
{
    for (GameObject* go : PendingKillGameObjects.FindOrAdd(CurrentScene))
    {
        DestroyGameObject(go); // 실제 제거
    }
    PendingKillGameObjects.Empty();
}

PxShape* FPhysicsManager::CreateBoxShape(const PxVec3& Pos, const PxQuat& Quat, const PxVec3& HalfExtents) const
{
    // Box 모양 생성
    PxShape* Result = Physics->createShape(PxBoxGeometry(HalfExtents), *Material);
    
    // 위치와 회전을 모두 적용한 Transform 생성
    PxTransform LocalTransform(Pos, Quat);
    Result->setLocalPose(LocalTransform);
    
    return Result;
}

PxShape* FPhysicsManager::CreateSphereShape(const PxVec3& Pos, const PxQuat& Quat, float Radius) const
{
    // Sphere 모양 생성 (구는 회전에 영향받지 않지만 일관성을 위해 적용)
    PxShape* Result = Physics->createShape(PxSphereGeometry(Radius), *Material);
    
    // 위치와 회전을 모두 적용한 Transform 생성
    PxTransform LocalTransform(Pos, Quat);
    Result->setLocalPose(LocalTransform);
    
    return Result;
}

PxShape* FPhysicsManager::CreateCapsuleShape(const PxVec3& Pos, const PxQuat& UnrealQuatZ, float Radius, float HalfHeight) const
{
    // 1. Unreal Z축 정렬 → PhysX Y축 정렬로 회전
    PxQuat ToPhysXRot = PxQuat(PxPi / 2.0f, PxVec3(0,1, 0)); // -90도 y 회전
    PxQuat PhysXQuat = ToPhysXRot * UnrealQuatZ;

    // 2. Capsule 생성 (PhysX는 Y축 기준)
    PxShape* Result = Physics->createShape(PxCapsuleGeometry(Radius, HalfHeight), *Material);

    // 3. 위치 및 방향 적용
    PxTransform LocalTransform(Pos, PhysXQuat);
    Result->setLocalPose(LocalTransform);

    return Result;
}

void FPhysicsManager::Simulate(float DeltaTime)
{
    if (CurrentScene)
    {
        QUICK_SCOPE_CYCLE_COUNTER(SimulatePass_CPU)
        CurrentScene->simulate(DeltaTime);
        CurrentScene->fetchResults(true);
        ProcessPendingKills();
    }
}

void FPhysicsManager::ShutdownPhysX()
{
    if(CurrentScene)
    {
        CurrentScene->release();
        CurrentScene = nullptr;
    }
    if(Dispatcher)
    {
        Dispatcher->release();
        Dispatcher = nullptr;
    }
    if(Material)
    {
        Material->release();
        Material = nullptr;
    }
    if(Physics)
    {
        Physics->release();
        Physics = nullptr;
    }
    if(Foundation)
    {
        Foundation->release();
        Foundation = nullptr;
    }
    if(SimEventCallback)
    {
        delete SimEventCallback;
        SimEventCallback = nullptr;
    }
}

void FPhysicsManager::CleanupPVD() {
    if (Pvd) {
        if (Pvd->isConnected()) {
            Pvd->disconnect();
        }
        if (Transport) {
            Transport->release();
            Transport = nullptr;
        }
        Pvd->release();
        Pvd = nullptr;
    }
}

void FPhysicsManager::CleanupScene()
{
    if (CurrentScene)
    {
        CurrentScene->release();
        CurrentScene = nullptr;
    }
}

PxFilterFlags MySimulationFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    // 1. 자동차 부품 간의 충돌은 항상 억제 (가장 먼저 처리)
    // PxFilterData의 word0에 충돌 채널(타입) 정보가 있다고 가정합니다.
    bool bIsCarBodyWheel = ((filterData0.word0 == ECC_CarBody && filterData1.word0 == ECC_Wheel) ||
                            (filterData0.word0 == ECC_Wheel && filterData1.word0 == ECC_CarBody));
    bool bIsCarHub = ((filterData0.word0 == ECC_CarBody && filterData1.word0 == ECC_Hub) ||
                      (filterData0.word0 == ECC_Hub && filterData1.word0 == ECC_CarBody));
    bool bIsWheelHub = ((filterData0.word0 == ECC_Wheel && filterData1.word0 == ECC_Hub) ||
                        (filterData0.word0 == ECC_Hub && filterData1.word0 == ECC_Wheel));

    if (bIsCarBodyWheel || bIsCarHub || bIsWheelHub)
    {
        pairFlags = PxPairFlags(); // 이 쌍에 대한 모든 상호작용 플래그를 끔
        return PxFilterFlag::eSUPPRESS; // 이 쌍의 충돌을 완전히 억제
    }

    // 2. 그 외의 경우에는 PxDefaultSimulationFilterShader를 호출하여 기본적인 필터링 결정
    //    (충돌 여부, 트리거 여부, 기본 pairFlags 설정 등)
    PxFilterFlags defaultFilterFlags = PxDefaultSimulationFilterShader(
        attributes0, filterData0,
        attributes1, filterData1,
        pairFlags, constantBlock, constantBlockSize); // 여기서 pairFlags가 기본 설정됨

    // 3. PxDefaultSimulationFilterShader가 충돌을 억제하지 않았고 (eSUPPRESS가 아니고),
    //    물리적 충돌을 해결하도록 설정된 경우 (eSOLVE_CONTACT),
    //    추가적인 알림 플래그를 설정합니다.
    if (defaultFilterFlags != PxFilterFlag::eSUPPRESS && (pairFlags & PxPairFlag::eSOLVE_CONTACT))
    {
        // PxDefaultSimulationFilterShader가 설정한 pairFlags에 필요한 알림 플래그들을 추가 (OR 연산)
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;    // 필요하다면 추가
        pairFlags |= PxPairFlag::eNOTIFY_CONTACT_POINTS; // OnComponentHit에서 impulse 값을 얻기 위해 중요
        // pairFlags |= PxPairFlag::eNOTIFY_TOUCH_PERSISTS; // 지속적인 접촉 알림 (필요에 따라)
                                                        // eNOTIFY_TOUCH_PERSISTS는 성능에 영향을 줄 수 있으므로 신중히 사용
    }
    // 만약 PxDefaultSimulationFilterShader가 트리거로 처리하도록 pairFlags를 설정했다면 (예: pairFlags = PxPairFlag::eTRIGGER_DEFAULT),
    // 위의 if 조건 ((pairFlags & PxPairFlag::eSOLVE_CONTACT)) 에 걸리지 않으므로,
    // 트리거에 대한 pairFlags는 그대로 유지됩니다. (eNOTIFY_CONTACT_POINTS 등은 추가되지 않음)

    return defaultFilterFlags; // PxDefaultSimulationFilterShader의 최종 결정(eDEFAULT, eSUPPRESS 등)을 반환
}
