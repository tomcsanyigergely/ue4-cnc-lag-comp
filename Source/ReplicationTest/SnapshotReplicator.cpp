// Fill out your copyright notice in the Description page of Project Settings.


#include "SnapshotReplicator.h"

// Sets default values
ASnapshotReplicator::ASnapshotReplicator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bAlwaysRelevant = true;
}

// Called when the game starts or when spawned
void ASnapshotReplicator::BeginPlay()
{
	Super::BeginPlay();

	//AddTickPrerequisiteComponent(...);
}

// Called every frame
void ASnapshotReplicator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() == ROLE_Authority)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, TEXT("Tick"));

		FSnapshotPacketBits SnapshotPacketBits;
		SnapshotPacketBits.TimeStamp = GetWorld()->GetTimeSeconds();

		for(FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PlayerController = Iterator->Get();
			APawn* ControlledPawn = PlayerController->GetPawn();

			if (IsValid(ControlledPawn))
			{
				
			}
		}

		MulticastSnapshotRPC(SnapshotPacketBits);
		ForceNetUpdate();
	}
}

void ASnapshotReplicator::MulticastSnapshotRPC_Implementation(FSnapshotPacketBits SnapshotPacketBits)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Blue, FString::Printf(TEXT("Received snapshot: %f"), SnapshotPacketBits.TimeStamp));
}

