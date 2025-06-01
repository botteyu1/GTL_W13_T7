#include "CameraActor.h"

void ACameraActor::PostSpawnInitialize()
{
    AActor::PostSpawnInitialize();
}

UObject* ACameraActor::Duplicate(UObject* InOuter)
{
    return AActor::Duplicate(InOuter);
}

void ACameraActor::BeginPlay()
{
    AActor::BeginPlay();
}

void ACameraActor::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);
}

void ACameraActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    AActor::EndPlay(EndPlayReason);
}
