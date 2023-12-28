// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SnapshotPacketBits.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MyCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class REPLICATIONTEST_API UMyCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	FPlayerSnapshot SnapshotBuffer[256];

	uint8 BeginIndex = 0;
	uint8 EndIndex = 0;

	float TargetTime = 0;

public:
	void AddSnapshot(float Timestamp, FPlayerSnapshot PlayerSnapshot);

protected:
	virtual void SimulatedTick(float DeltaSeconds) override;
};
