// Fill out your copyright notice in the Description page of Project Settings.


#include "SnapshotReplicator.h"

#include "ReplicationTestPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

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
		FSnapshotPacketBits SnapshotPacketBits;
		SnapshotPacketBits.TimeStamp = GetWorld()->GetTimeSeconds();

		for (const APlayerState* PlayerState : GetWorld()->GetGameState()->PlayerArray)
		{
			APawn* ControlledPawn = PlayerState->GetPawn();
			const AReplicationTestPlayerState* RepTestPlayerState = Cast<AReplicationTestPlayerState>(PlayerState);
			if (IsValid(ControlledPawn) && IsValid(RepTestPlayerState) && RepTestPlayerState->RepTestPlayerId != 0)
			{
				FPlayerSnapshot PlayerSnapshot{RepTestPlayerState->RepTestPlayerId, ControlledPawn->GetActorLocation()};
				SnapshotPacketBits.PlayerSnapshots.Add(PlayerSnapshot);
			}
		}

		MulticastSnapshotRPC(SnapshotPacketBits);
		ForceNetUpdate();
	}
}

void ASnapshotReplicator::MulticastSnapshotRPC_Implementation(FSnapshotPacketBits SnapshotPacketBits)
{
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		AReplicationTestPlayerState* LocalPlayerState = GetWorld()->GetFirstPlayerController()->GetPlayerState<AReplicationTestPlayerState>();

		if (IsValid(LocalPlayerState))
		{
			for(const FPlayerSnapshot& PlayerSnapshot : SnapshotPacketBits.PlayerSnapshots)
			{		
				if (PlayerSnapshot.PlayerId != 0 && LocalPlayerState->RepTestPlayerId != PlayerSnapshot.PlayerId)
				{
					for(APlayerState* PlayerState : GetWorld()->GetGameState()->PlayerArray)
					{
						AReplicationTestPlayerState* RepTestPlayerState = Cast<AReplicationTestPlayerState>(PlayerState);
						if (IsValid(RepTestPlayerState) && RepTestPlayerState->RepTestPlayerId != 0 && RepTestPlayerState->RepTestPlayerId == PlayerSnapshot.PlayerId)
						{
							APawn* ControlledPawn = PlayerState->GetPawn();
							if (IsValid(ControlledPawn))
							{
								ControlledPawn->SetActorLocation(PlayerSnapshot.Position);
							}
							break;
						}
					}
				}
			}
		}
	}
}

