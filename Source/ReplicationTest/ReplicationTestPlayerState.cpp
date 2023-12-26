// Fill out your copyright notice in the Description page of Project Settings.


#include "ReplicationTestPlayerState.h"

#include "Net/UnrealNetwork.h"

void AReplicationTestPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AReplicationTestPlayerState, RepTestPlayerId)
}
