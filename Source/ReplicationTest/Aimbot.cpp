// Fill out your copyright notice in the Description page of Project Settings.


#include "Aimbot.h"

#include "Kismet/KismetSystemLibrary.h"
#include "CollisionChannels.h"
#include "ReplicationTestCharacter.h"
#include "ReplicationTestGameState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
AAimbot::AAimbot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
}

// Called when the game starts or when spawned
void AAimbot::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAimbot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled())
	{
		if (Target != nullptr)
		{
			FVector TargetLocation = Target->Head->GetComponentLocation();
			FVector Direction = (TargetLocation - GetActorLocation()).GetSafeNormal();
			CameraComponent->SetWorldRotation(UKismetMathLibrary::MakeRotFromX(Direction));

			if (ShootSignal)
			{
				GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Purple, FString::Printf(TEXT("InterpTime: %f, LastInterp: %f, Pos: [%f %f %f]"), GetWorld()->GetGameState<AReplicationTestGameState>()->CurrentInterpolationTime, Target->GetMyCharacterMovementComponent()->LastInterp, TargetLocation.X, TargetLocation.Y, TargetLocation.Z));
				ShootServerRPC(GetActorLocation(), Direction, GetWorld()->GetGameState<AReplicationTestGameState>()->CurrentInterpolationTime);
				ShootSignal = false;
			}
		}
		else
		{
			TArray<AActor*> OutActors;
			UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(TEXT("Target")), OutActors);

			if (OutActors.Num() >= 1)
			{
				Target = Cast<AReplicationTestCharacter>(OutActors[0]);
			}

			GetWorldTimerManager().SetTimer(ShootTimer, FTimerDelegate::CreateUObject(this, &AAimbot::Shoot), ShootInterval, true);
		}
	}
}

// Called to bind functionality to input
void AAimbot::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AAimbot::Shoot()
{
	ShootSignal = true;
}

void AAimbot::ShootServerRPC_Implementation(FVector Location, FVector Direction, float InterpTime)
{
	FHitResult OutHit;

	for (const APlayerState* _PlayerState : GetWorld()->GetGameState()->PlayerArray)
	{
		AReplicationTestCharacter* Character = _PlayerState->GetPawn<AReplicationTestCharacter>();
		if (IsValid(Character))
		{
			Character->GetMyCharacterMovementComponent()->RewindPose(InterpTime);
			FVector TargetLocation = Character->Head->GetComponentLocation();
			UE_LOG(LogTemp, Warning, TEXT("InterpTime: %f, LastRewindInterp: %f, Pos: [%f %f %f]"), InterpTime, Character->GetMyCharacterMovementComponent()->LastRewindInterp, TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
		}
	}

	UKismetSystemLibrary::LineTraceSingle(GetWorld(), Location, Location + Direction * 10000.0f, UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_GameTraceChannel_WeaponTrace), false, {}, EDrawDebugTrace::None, OutHit, false);

	if (OutHit.bBlockingHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("HIT !!!!!!!!!"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MISS"));
	}
	
	for (const APlayerState* _PlayerState : GetWorld()->GetGameState()->PlayerArray)
	{
		AReplicationTestCharacter* Character = _PlayerState->GetPawn<AReplicationTestCharacter>();
		if (IsValid(Character))
		{
			Character->GetMyCharacterMovementComponent()->ResetPose();
		}
	}
}

