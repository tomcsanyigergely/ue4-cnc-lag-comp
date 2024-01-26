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

	class FSavedMove_MyCharacter : public FSavedMove_Character
	{
		typedef FSavedMove_Character Super;

		uint8 Saved_bWantsToSprint:1;
		uint8 Saved_bWantsToShoot:1;

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
	};

	class FNetworkPredictionData_Client_MyCharacter : public FNetworkPredictionData_Client_Character
	{
	public:
		FNetworkPredictionData_Client_MyCharacter(const UCharacterMovementComponent& ClientMovement);

		typedef FNetworkPredictionData_Client_Character Super;
		
		virtual FSavedMovePtr AllocateNewMove() override;
	};

	bool Safe_bWantsToSprint;
	bool Safe_bWantsToShoot;
	
public:
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	
private:
	struct FTimestamp
	{
		float Timestamp;
		float ArrivalTime;
	};

	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
	float Sprint_MaxWalkSpeed = 2000.0f;

	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
	float Walk_MaxWalkSpeed = 1000.0f;

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

	void SprintPressed();
	void SprintReleased();

	void TryShoot();
	UFUNCTION(Server, Reliable) void ServerShootRPC();

protected:
	virtual void SimulatedTick(float DeltaSeconds) override;

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

private:
	double CalculateInterpolationMultiplier();
};

