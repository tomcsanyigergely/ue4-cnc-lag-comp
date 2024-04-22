// Copyright Epic Games, Inc. All Rights Reserved.

#include "ReplicationTestGameMode.h"
#include "UObject/ConstructorHelpers.h"

AReplicationTestGameMode::AReplicationTestGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ReplicationTestCharacter_BP"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AReplicationTestGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	bStartPlayersAsSpectators = true;
	
	Super::InitGame(MapName, Options, ErrorMessage);

#if WITH_EDITOR
	DisableLagCompensation = DisableLagCompensationEditor;
#else
	DisableLagCompensation = FParse::Param(FCommandLine::Get(), TEXT("DisableLagCompensation"));
#endif

	UE_LOG(LogTemp, Warning, TEXT("DisableLagCompensation: %d"), DisableLagCompensation);
}

void AReplicationTestGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	lastPlayerIndex++;

	APawn* NewPawn = nullptr;
	FActorSpawnParameters ActorSpawnParameters;
	ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (lastPlayerIndex == 1)
	{
		if (!DisableLagCompensation)
		{
			NewPawn = GetWorld()->SpawnActor<APawn>(PlayerPawnClass, FVector{0.0, 0.0, 200.0}, FRotator::ZeroRotator, ActorSpawnParameters);
			UE_LOG(LogTemp, Warning, TEXT("Character spawned."));
		}
		else
		{
			NewPawn = GetWorld()->SpawnActor<APawn>(DefaultPlayerPawnClass, FVector{0.0, 0.0, 200.0}, FRotator::ZeroRotator, ActorSpawnParameters);
			UE_LOG(LogTemp, Warning, TEXT("Default Character spawned."));
		}
	}
	else if (lastPlayerIndex == 2)
	{
		NewPawn = GetWorld()->SpawnActor<APawn>(AimbotPawnClass, FVector{-1000.0, 0.0, 200.0}, FRotator::ZeroRotator, ActorSpawnParameters);
		UE_LOG(LogTemp, Warning, TEXT("Aimbot spawned."));
	}

	if (IsValid(NewPawn))
	{
		NewPlayer->Possess(NewPawn);
	}
	
}
