// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SnapshotPacketBits.h"
#include "GameFramework/Actor.h"
#include "SnapshotReplicator.generated.h"

UCLASS()
class REPLICATIONTEST_API ASnapshotReplicator : public AActor
{
	GENERATED_BODY()

	FTimerHandle PingTimer;
	
public:	
	// Sets default values for this actor's properties
	ASnapshotReplicator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSnapshotRPC(FSnapshotPacketBits SnapshotPacketBits);

private:
	UFUNCTION()
	void ShowPing();
};
