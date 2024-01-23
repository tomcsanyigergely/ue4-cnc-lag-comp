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

	struct FTimestamp
	{
		float Timestamp;
		float ArrivalTime;
	};

	FPlayerSnapshot SnapshotBuffer[256];
	FTimestamp TimestampBuffer[256];

	uint8_t TimestampBufferWindow = 20;

	uint8 BeginIndex = 0;
	uint8 EndIndex = 0;

	double CurrentInterpolationTime = 0;
	double InterpolationMultiplier = 1.0;	
	
	int snapshotsReceived = 0;

	FVector SavedPoseLocation;

public:
	float LastInterp;
	float LastRewindInterp;
	
	void AddClientSideSnapshot(float Timestamp, FPlayerSnapshot PlayerSnapshot);
	void AddServerSideSnapshot(float Timestamp, FPlayerSnapshot PlayerSnapshot);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void RewindPose(float RewindTime);
	void ResetPose();

	FORCEINLINE double GetInterpolationTime() const { return CurrentInterpolationTime; }

protected:
	virtual void SimulatedTick(float DeltaSeconds) override;

private:
	double CalculateInterpolationMultiplier();
};
