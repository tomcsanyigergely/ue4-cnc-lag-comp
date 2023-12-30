// Fill out your copyright notice in the Description page of Project Settings.


#include "ReplicationTestGameState.h"

#include "ReplicationTestPlayerState.h"

void AReplicationTestGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	if (GetLocalRole() == ROLE_Authority)
	{
		AReplicationTestPlayerState* RepTestPlayerState = Cast<AReplicationTestPlayerState>(PlayerState);
		if (IsValid(RepTestPlayerState))
		{
			RepTestPlayerState->RepTestPlayerId = LastPlayerId+1;
			LastPlayerId++;
		}
	}
}
