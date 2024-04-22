// Fill out your copyright notice in the Description page of Project Settings.


#include "Aimbot.h"

#include "Kismet/KismetSystemLibrary.h"
#include "CollisionChannels.h"
#include "DefaultCharacter.h"
#include "DrawDebugHelpers.h"
#include "ReplicationTestCharacter.h"
#include "ReplicationTestGameMode.h"
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
			FVector TargetLocation = Target->GetActorLocation();//->Head->GetComponentLocation();
			TargetLocation.Z = 0;

			
			FVector Direction = (TargetLocation - GetActorLocation()).GetSafeNormal();
			Direction.Z = 0;
			
			CameraComponent->SetWorldRotation(UKismetMathLibrary::MakeRotFromX(Direction));

			if (ShootSignal)
			{
				for(UBoxComponent* Box : Target->Hitbox)
				{
					DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), Box->GetComponentRotation().Quaternion(), FColor::Blue, false, 10.0f, 0, 1);;
				}

				ShootServerRPC(GetActorLocation(), Direction, GetWorld()->GetGameState<AReplicationTestGameState>()->CurrentInterpolationTime);
				ShootSignal = false;
			}
		}
		else if (DefaultTarget != nullptr)
		{
			FVector TargetLocation = DefaultTarget->GetActorLocation();
			TargetLocation.Z = 0;

			
			FVector Direction = (TargetLocation - GetActorLocation()).GetSafeNormal();
			Direction.Z = 0;
			
			CameraComponent->SetWorldRotation(UKismetMathLibrary::MakeRotFromX(Direction));

			if (ShootSignal)
			{
				for(UBoxComponent* Box : DefaultTarget->Hitbox)
				{
					DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), Box->GetComponentRotation().Quaternion(), FColor::Blue, false, 10.0f, 0, 1);;
				}

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
				DefaultTarget = Cast<ADefaultCharacter>(OutActors[0]);
			}
			
			GetWorldTimerManager().SetTimer(ShootTimer, FTimerDelegate::CreateUObject(this, &AAimbot::Shoot), ShootInterval, true);
		}
	}

	if (GetWorld()->IsServer() && Target == nullptr)
	{
		TArray<AActor*> OutActors;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(TEXT("Target")), OutActors);

		if (OutActors.Num() >= 1)
		{
			Target = Cast<AReplicationTestCharacter>(OutActors[0]);
			DefaultTarget = Cast<ADefaultCharacter>(OutActors[0]);
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

void AAimbot::HitboxRPC_Implementation(const TArray<FVector>& Locations, const TArray<FRotator>& Rotations)
{
	if (IsLocallyControlled())
	{
		for(int i = 0; i < Target->Hitbox.Num(); i++)
		{
			DrawDebugBox(GetWorld(), Locations[i], Target->Hitbox[i]->GetScaledBoxExtent(), Rotations[i].Quaternion(), FColor::Red, false, 10.0f, 0, 1);;
		}
	}
}

void AAimbot::HitboxDefaultTargetRPC_Implementation(const TArray<FVector>& Locations, const TArray<FRotator>& Rotations)
{
	if (IsLocallyControlled())
	{
		for(int i = 0; i < DefaultTarget->Hitbox.Num(); i++)
		{
			DrawDebugBox(GetWorld(), Locations[i], DefaultTarget->Hitbox[i]->GetScaledBoxExtent(), Rotations[i].Quaternion(), FColor::Red, false, 10.0f, 0, 1);;
		}
	}
}

void AAimbot::ShootServerRPC_Implementation(FVector Location, FVector Direction, float TargetRewindTime)
{
	if (!GetWorld()->GetAuthGameMode<AReplicationTestGameMode>()->DisableLagCompensation)
	{
		FHitResult OutHit;

		float AuthRewindTime = GetWorld()->GetTimeSeconds() - GetPlayerState()->ExactPingV2 * 0.001 - 0.20;
		float RewindTime = FMath::Clamp(TargetRewindTime, AuthRewindTime - 0.025f, AuthRewindTime + 0.025f);

		UE_LOG(LogTemp, Warning, TEXT("TargetRewindTime: %f, AuthRewindTime: %f, RewindTime: %f, Diff: %f"), TargetRewindTime, AuthRewindTime, RewindTime, RewindTime-TargetRewindTime);

		for (const APlayerState* _PlayerState : GetWorld()->GetGameState()->PlayerArray)
		{
			AReplicationTestCharacter* Character = _PlayerState->GetPawn<AReplicationTestCharacter>();
			if (IsValid(Character))
			{
				Character->GetMyCharacterMovementComponent()->RewindPose(RewindTime);
				FVector TargetLocation = Character->Head->GetComponentLocation();
			}
		}

		if (Target != nullptr)
		{
			TArray<FVector> HitboxLocations;
			TArray<FRotator> HitboxRotations;

			for(UBoxComponent* HitBox : Target->Hitbox)
			{
				HitboxLocations.Add(HitBox->GetComponentLocation());
				HitboxRotations.Add(HitBox->GetComponentRotation());
			}

			HitboxRPC(HitboxLocations, HitboxRotations);
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
	else
	{
		if (DefaultTarget != nullptr)
		{
			TArray<FVector> HitboxLocations;
			TArray<FRotator> HitboxRotations;

			for(UBoxComponent* HitBox : DefaultTarget->Hitbox)
			{
				HitboxLocations.Add(HitBox->GetComponentLocation());
				HitboxRotations.Add(HitBox->GetComponentRotation());
			}

			HitboxDefaultTargetRPC(HitboxLocations, HitboxRotations);
		}
	}
}

