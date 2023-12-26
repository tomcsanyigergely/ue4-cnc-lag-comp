// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "ReplicationTestGameState.generated.h"

/**
 * 
 */
UCLASS()
class REPLICATIONTEST_API AReplicationTestGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void AddPlayerState(APlayerState* PlayerState) override;

private:
	uint8 LastPlayerId = 0;
};
