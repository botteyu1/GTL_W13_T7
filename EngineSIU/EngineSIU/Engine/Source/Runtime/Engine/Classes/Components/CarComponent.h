#pragma once

#include "StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Physics/PhysicsManager.h"

class UCameraComponent;

class UCarComponent : public UStaticMeshComponent
{
    DECLARE_CLASS(UCarComponent, UStaticMeshComponent)

public:
    UCarComponent();
    virtual ~UCarComponent() override;

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void TickComponent(float DeltaTime) override;
    virtual void EndPhysicsTickComponent(float DeltaTime) override;

    virtual void CreatePhysXGameObject() override;
    virtual void Spawn();

    virtual void GetProperties(TMap<FString, FString>& OutProperties) const override;
    virtual void SetProperties(const TMap<FString, FString>& InProperties) override;

    void MoveCar();

    void createWheelConvexData(float radius, float halfHeight, int segmentCount, const FVector& Scale,
        std::vector<PxVec3>& outPoints);

    PxShape* CreateWheelShape(PxPhysics* Physics, PxCooking* Cooking, const FVector& Scale, int segmentCount);

    void UpdateFromPhysics(GameObject* PhysicsActor, UStaticMeshComponent* ActualActor);

    void ApplyForceToActors(float Angle, float Magnitude);

    float GetVelocity() { return Velocity; }
    void SetVelocity(float InVelocity) { Velocity = InVelocity; }

    float GetFinalBoost() { return FinalBoost; }
    void SetFinalBoost(float InBoost) { FinalBoost = InBoost; }

    float GetSteerAngle() { return SteerAngle; }
    void SetSteerAngle(float InAngle) { SteerAngle = InAngle; }

    float GetSlopeAngle() {return FMath::DegreesToRadians(GetComponentRotation().Pitch); }
    void SetSlopeAngle(float InAngle) { SlopeAngle = InAngle; }

    bool IsBoosted() { return bBoosted; }
    void SetBoosted(bool Value) { bBoosted = Value; }
    void BoostCar();

    float GetCurSteerAngle();

    float GetCurSpeed();

    void Restart();

    int GetFireCount() { return FireCount; }

    bool IsDriving() { return bCarDriving; }

    void Release();

private:
    PxMaterial* DefaultMaterial = nullptr;
    GameObject* CarBody = nullptr;
    GameObject* Hub[2] = { nullptr }; //Front, Rear
    GameObject* Wheels[4] = { nullptr }; //FR, FL, RR, RL
    PxRevoluteJoint* WheelJoints[4] = { nullptr }; //FR, FL, RR, RL
    PxRevoluteJoint* SteeringJoint = nullptr;
    PxFixedJoint* RearHubJoints[2] = { nullptr };
    float MaxSteerAngle = PxPi / 6.f;
    float DeltaSteerAngle = PxPi / 6.f;
    float SteerAngle = 0.0f;
    float MaxDriveTorque = 50000.0f;
    float Velocity = 0.f;
    float MaxVelocity = 70.f;
    float FinalBoost = 0.f;
    float MaxBoost = 18000.f;
    bool bBoosted = false;
    bool bCarDriving = true;

    UStaticMeshComponent* WheelComp[4] = { nullptr };
    UParticleSystemComponent* BoostParticle = nullptr;
    UParticleSystemComponent* WheelDustParticle[2] =  { nullptr };

    UCameraComponent* Camera = nullptr;

    bool bHasBody = false;

    bool bSoundCarEngine = false;

    FVector CarBodyPos = FVector(0, 0, 0);
    FVector BodyExtent;
    float WheelRadius = 1.2f;
    float WheelWidth = 0.6f;
    float WheelHeight = 0.6f; //half height

    float SlopeAngle;

    int FireCount = 1;

    FVector HubSize = FVector(0.2f, 0.5f, 0.2f);

    const FVector WheelPos[4] =
    {
        {  0.7f,  0.8f, -0.2f}, //FR
        {  0.7f, -0.8f, -0.2f}, //FL
        {-0.95f,  0.9f, -0.1f}, //RR
        {-0.95f, -0.9f, -0.1f}  //RL
    };

    PxTransform InitialBodyT;
    PxTransform InitialWheelT[4];
    PxTransform InitialHubT[2];
};
