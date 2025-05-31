#pragma once
#include "Actors/Player.h"
#include "GameFramework/Actor.h"


class APlayerCameraTest : public ASequencerPlayer
{
    DECLARE_CLASS(APlayerCameraTest, ASequencerPlayer)

public:
    APlayerCameraTest() = default;
    virtual ~APlayerCameraTest() override = default;

    virtual void BeginPlay() override;

    
};
