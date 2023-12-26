// Copyright Epic Games, Inc. All Rights Reserved.

#include "ReplicationTestGameMode.h"
#include "ReplicationTestCharacter.h"
#include "UObject/ConstructorHelpers.h"

AReplicationTestGameMode::AReplicationTestGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
