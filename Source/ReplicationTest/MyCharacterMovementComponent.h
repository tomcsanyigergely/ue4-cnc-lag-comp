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

	double CurrentInterpolationTime = 0;
	double InterpolationMultiplier = 1.0f;
	double slope = 0;
	double intercept = 0;
	int snapshotsReceived = 0;

public:
	void AddSnapshot(float Timestamp, FPlayerSnapshot PlayerSnapshot);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void SimulatedTick(float DeltaSeconds) override;

private:
	void CalculateLineOfBestFit();
};
