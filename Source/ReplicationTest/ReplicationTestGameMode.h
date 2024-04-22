// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ReplicationTestGameMode.generated.h"

UCLASS(minimalapi)
class AReplicationTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
	TSubclassOf<APawn> PlayerPawnClass;

	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
	TSubclassOf<APawn> AimbotPawnClass;

	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
	TSubclassOf<APawn> DefaultPlayerPawnClass;

	int lastPlayerIndex = 0;	

	UPROPERTY(EditDefaultsOnly)
	bool DisableLagCompensationEditor = false;

public:
	bool DisableLagCompensation = false;
	
	AReplicationTestGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
};



