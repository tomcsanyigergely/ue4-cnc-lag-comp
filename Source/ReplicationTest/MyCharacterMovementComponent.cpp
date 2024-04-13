// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCharacterMovementComponent.h"

#include "ReplicationTestCharacter.h"
#include "ReplicationTestGameState.h"
#include "GameFramework/Character.h"

bool UMyCharacterMovementComponent::FSavedMove_MyCharacter::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	FSavedMove_MyCharacter* NewMyMove = static_cast<FSavedMove_MyCharacter*>(NewMove.Get());

	if (Saved_bWantsToSprint != NewMyMove->Saved_bWantsToSprint)
	{
		return false;
	}
	
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void UMyCharacterMovementComponent::FSavedMove_MyCharacter::Clear()
{
	Super::Clear();

	Saved_bWantsToSprint = 0;
}

uint8 UMyCharacterMovementComponent::FSavedMove_MyCharacter::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (Saved_bWantsToSprint) Result |= FLAG_Custom_0;

	return Result;
}

void UMyCharacterMovementComponent::FSavedMove_MyCharacter::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	UMyCharacterMovementComponent* CharacterMovement = Cast<UMyCharacterMovementComponent>(C->GetCharacterMovement());

	Saved_bWantsToSprint = CharacterMovement->Safe_bWantsToSprint;
	Saved_bWantsToShoot = CharacterMovement->Safe_bWantsToShoot;
}

void UMyCharacterMovementComponent::FSavedMove_MyCharacter::PrepMoveFor(ACharacter* C)
{
	FSavedMove_Character::PrepMoveFor(C);

	UMyCharacterMovementComponent* CharacterMovement = Cast<UMyCharacterMovementComponent>(C->GetCharacterMovement());

	CharacterMovement->Safe_bWantsToSprint = Saved_bWantsToSprint;
	CharacterMovement->Safe_bWantsToShoot = Saved_bWantsToShoot;
}

UMyCharacterMovementComponent::FNetworkPredictionData_Client_MyCharacter::FNetworkPredictionData_Client_MyCharacter(const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement)
{	
}

FSavedMovePtr UMyCharacterMovementComponent::FNetworkPredictionData_Client_MyCharacter::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_MyCharacter());
}

FNetworkPredictionData_Client* UMyCharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr)

	if (ClientPredictionData == nullptr)
	{
		UMyCharacterMovementComponent* MutableThis = const_cast<UMyCharacterMovementComponent*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_MyCharacter(*this);
		//MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		//MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.0f;
	}
	
	return ClientPredictionData;
}

void UMyCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	Safe_bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

void UMyCharacterMovementComponent::AddClientSideSnapshot(float Timestamp, FPlayerSnapshot PlayerSnapshot)
{
	if (Timestamp > SnapshotBuffer[static_cast<uint8>(EndIndex-1)].Timestamp)
	{
		SnapshotBuffer[EndIndex] = PlayerSnapshot;
		SnapshotBuffer[EndIndex].Timestamp = Timestamp;
		EndIndex++;
	}
	
	TimestampBuffer[snapshotsReceived % 256].Timestamp = Timestamp;
	TimestampBuffer[snapshotsReceived % 256].ArrivalTime = GetWorld()->GetTimeSeconds();
	snapshotsReceived++;
	
	if (snapshotsReceived >= TimestampBufferWindow)
	{
		double AvgTimestamp = 0;
		double AvgArrivalTime = 0;
		for(int i = 1; i <= TimestampBufferWindow; i++)
		{
			uint8 TimestampIndex = ((snapshotsReceived - i) % 256);
			AvgTimestamp += TimestampBuffer[TimestampIndex].Timestamp;
			AvgArrivalTime += TimestampBuffer[TimestampIndex].ArrivalTime;
		}

		AvgTimestamp /= TimestampBufferWindow;
		AvgArrivalTime /= TimestampBufferWindow;
		double Diff = AvgTimestamp - AvgArrivalTime;

		// TODO: slope-ot figyelembe venni!!!!
		double TargetInterpolationTime = GetWorld()->GetTimeSeconds() + Diff - 3.0 / 20.0 - 0.05;
		//GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Yellow, FString::Printf(TEXT("Diff: %f"), (TargetInterpolationTime - CurrentInterpolationTime) * 1000.0f));

		InterpolationMultiplier = CalculateInterpolationMultiplier();

		if (FMath::Abs(TargetInterpolationTime - CurrentInterpolationTime) >= 0.005f)
		{
			CurrentInterpolationTime = TargetInterpolationTime;
			if (IsValid(GetWorld()->GetGameState<AReplicationTestGameState>()))
			{
				GetWorld()->GetGameState<AReplicationTestGameState>()->PreviousInterpolationTime = GetWorld()->GetGameState<AReplicationTestGameState>()->CurrentInterpolationTime;
				GetWorld()->GetGameState<AReplicationTestGameState>()->CurrentInterpolationTime = CurrentInterpolationTime;
			}
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Blue, FString::Printf(TEXT("Snap")));
		}
			
		//CurrentInterpolationTime = TargetInterpolationTime;
		/*if (FMath::Abs(TargetInterpolationTime - CurrentInterpolationTime) >= 0.1f)
		{
			CurrentInterpolationTime = TargetInterpolationTime;
			InterpolationMultiplier = 1.0;
		}
		else
		{
			InterpolationMultiplier = FMath::Clamp((1.0 / 20.0 + (TargetInterpolationTime - CurrentInterpolationTime)) / (1.0 / 20.0), 0.9, 1.1);
		}*/
			
		BeginIndex = static_cast<uint8>(EndIndex-1);

		while(SnapshotBuffer[BeginIndex].Timestamp > TargetInterpolationTime && SnapshotBuffer[static_cast<uint8>(BeginIndex-1)].Timestamp < SnapshotBuffer[BeginIndex].Timestamp)
		{
			BeginIndex--;
		}
		/*double InterpolationTimeDifference = TargetInterpolationTime - CurrentInterpolationTime;
		InterpolationMultiplier = (InterpolationTimeDifference + 1.0f / 20.0f) / (1.0f / 20.0f);*/
			
	}
}

void UMyCharacterMovementComponent::AddServerSideSnapshot(float Timestamp, FPlayerSnapshot PlayerSnapshot)
{
	SnapshotBuffer[EndIndex] = PlayerSnapshot;
	SnapshotBuffer[EndIndex].Timestamp = Timestamp;
	EndIndex++;
}

bool UMyCharacterMovementComponent::ForcePositionUpdate(float DeltaTime)
{
	static int i = 0;

	if (GetWorld()->IsServer() && i % 10 == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("FORCE"))
	}

	i++;
	
	return Super::ForcePositionUpdate(DeltaTime);

	
}

void UMyCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                  FActorComponentTickFunction* ThisTickFunction)
{	
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Yellow, FString::Printf(TEXT("CMC::TickComponent(): %f"), GetWorld()->GetTimeSeconds()));
}

void UMyCharacterMovementComponent::RewindPose(float RewindTime)
{
	SavedPoseLocation = GetOwner()->GetActorLocation();

	uint8 SnapshotIndex = static_cast<uint8>(EndIndex-1);
	while (SnapshotBuffer[SnapshotIndex].Timestamp > RewindTime && SnapshotBuffer[static_cast<uint8>(SnapshotIndex-1)].Timestamp < SnapshotBuffer[SnapshotIndex].Timestamp)
	{
		SnapshotIndex--;
	}

	uint8 NumSnapshots = EndIndex - SnapshotIndex;

	if (NumSnapshots >= 2 && SnapshotBuffer[SnapshotIndex].Timestamp != 0)
	{
		float Interp = FMath::Clamp((RewindTime - SnapshotBuffer[SnapshotIndex].Timestamp) / (SnapshotBuffer[static_cast<uint8>(SnapshotIndex+1)].Timestamp - SnapshotBuffer[SnapshotIndex].Timestamp), 0.0f, 1.0f);
		FVector LerpPosition = FMath::Lerp(SnapshotBuffer[SnapshotIndex].Position, SnapshotBuffer[static_cast<uint8>(SnapshotIndex+1)].Position, Interp);
		GetOwner()->SetActorLocation(LerpPosition);

		AnimPlaybackTime = FMath::Lerp(SnapshotBuffer[SnapshotIndex].AnimPlaybackTime, SnapshotBuffer[static_cast<uint8>(SnapshotIndex+1)].AnimPlaybackTime, Interp);
		
		GetOwner<AReplicationTestCharacter>()->LagCompensatedSkeleton->GetAnimInstance()->UpdateAnimation(0.0, false);
		GetOwner<AReplicationTestCharacter>()->LagCompensatedSkeleton->RefreshBoneTransforms();
		
		LastRewindInterp = Interp;
	}
}

void UMyCharacterMovementComponent::ResetPose()
{
	GetOwner()->SetActorLocation(SavedPoseLocation);
}

void UMyCharacterMovementComponent::SprintPressed()
{
	Safe_bWantsToSprint = true;
}

void UMyCharacterMovementComponent::SprintReleased()
{
	Safe_bWantsToSprint = false;
}

void UMyCharacterMovementComponent::TryShoot()
{
	Safe_bWantsToShoot = true;
}

void UMyCharacterMovementComponent::ServerShootRPC_Implementation()
{
	Safe_bWantsToShoot = true;
	UE_LOG(LogTemp, Warning, TEXT("SERVER_SHOOT_RPC"))
}

void UMyCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	if (MovementMode == MOVE_Walking)
	{
		if (Safe_bWantsToSprint)
		{
			MaxWalkSpeed = Sprint_MaxWalkSpeed;
		}
		else
		{
			MaxWalkSpeed = Walk_MaxWalkSpeed;
		}
	}
}

void UMyCharacterMovementComponent::SimulatedTick(float DeltaSeconds) // on the same Tick, executes earlier than AddSnapshot()
{
	//GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Yellow, FString::Printf(TEXT("CMC::SimulatedTick(): %f"), GetWorld()->GetTimeSeconds()));
	if (CurrentInterpolationTime != 0)
	{
		while (SnapshotBuffer[static_cast<uint8>(BeginIndex+1)].Timestamp < CurrentInterpolationTime && static_cast<uint8>(BeginIndex+1) != EndIndex)
		{
			BeginIndex++;
		}

		uint8 NumSnapshots = EndIndex - BeginIndex;

		if (NumSnapshots == 1)
		{
			GetOwner()->SetActorLocation(SnapshotBuffer[BeginIndex].Position);
			AnimPlaybackTime = SnapshotBuffer[BeginIndex].AnimPlaybackTime;

			// ANIM SECTION BEGIN

			IdleTime = 0;
			IdleWeight = 0;

			JumpStartTime = 0;
			JumpStartWeight = 0;

			JumpLoopTime = 0;
			JumpLoopWeight = 0;

			JumpEndTime = 0;
			JumpEndWeight = 0;

			for(auto AnimSnapshot : SnapshotBuffer[BeginIndex].Anim)
			{
				switch (AnimSnapshot.Id)
				{
				case 1:
					IdleTime = AnimSnapshot.Time;
					IdleWeight = AnimSnapshot.Weight;
					break;
				case 2:
					JumpStartTime = AnimSnapshot.Time;
					JumpStartWeight = AnimSnapshot.Weight;
					break;
				case 3:
					JumpLoopTime = AnimSnapshot.Time;
					JumpLoopWeight = AnimSnapshot.Weight;
					break;
				case 4:
					JumpEndTime = AnimSnapshot.Time;
					JumpEndWeight = AnimSnapshot.Weight;
					break;
				default:
					break;
				}
			}

			// ANIM SECTION END
			
			GetOwner<AReplicationTestCharacter>()->LagCompensatedSkeleton->GetAnimInstance()->UpdateAnimation(0.0, false);
			GetOwner<AReplicationTestCharacter>()->LagCompensatedSkeleton->RefreshBoneTransforms();
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, FString::Printf(TEXT("UH OH")));
		}
		else
		{
			// ANIM SECTION BEGIN
			
			IdleTime = 0;
			IdleWeight = 0;

			JumpStartTime = 0;
			JumpStartWeight = 0;

			JumpLoopTime = 0;
			JumpLoopWeight = 0;

			JumpEndTime = 0;
			JumpEndWeight = 0;

			for(auto AnimSnapshot : SnapshotBuffer[static_cast<uint8>(BeginIndex+1)].Anim)
			{
				switch (AnimSnapshot.Id)
				{
				case 1:
					IdleTime = AnimSnapshot.Time;
					IdleWeight = AnimSnapshot.Weight;
					break;
				case 2:
					JumpStartTime = AnimSnapshot.Time;
					JumpStartWeight = AnimSnapshot.Weight;
					break;
				case 3:
					JumpLoopTime = AnimSnapshot.Time;
					JumpLoopWeight = AnimSnapshot.Weight;
					break;
				case 4:
					JumpEndTime = AnimSnapshot.Time;
					JumpEndWeight = AnimSnapshot.Weight;
					break;
				default:
					break;
				}
			}

			// ANIM SECTION END
			
			if (SnapshotBuffer[BeginIndex].Timestamp != 0)
			{
				float Interp = FMath::Clamp((CurrentInterpolationTime - SnapshotBuffer[BeginIndex].Timestamp) / (SnapshotBuffer[static_cast<uint8>(BeginIndex+1)].Timestamp - SnapshotBuffer[BeginIndex].Timestamp), 0.0, 1.0);
				FVector LerpPosition = FMath::Lerp(SnapshotBuffer[BeginIndex].Position, SnapshotBuffer[static_cast<uint8>(BeginIndex+1)].Position, Interp);
				float LerpAnimPlaybackTime = FMath::Lerp(SnapshotBuffer[BeginIndex].AnimPlaybackTime, SnapshotBuffer[static_cast<uint8>(BeginIndex+1)].AnimPlaybackTime, Interp);
				GetOwner()->SetActorLocation(LerpPosition);
				AnimPlaybackTime = LerpAnimPlaybackTime;
				GetOwner<AReplicationTestCharacter>()->LagCompensatedSkeleton->GetAnimInstance()->UpdateAnimation(0.0, false);
				GetOwner<AReplicationTestCharacter>()->LagCompensatedSkeleton->RefreshBoneTransforms();
				LastInterp = Interp;
			}
			else
			{
				GetOwner()->SetActorLocation(SnapshotBuffer[static_cast<uint8>(BeginIndex+1)].Position);
				AnimPlaybackTime = SnapshotBuffer[static_cast<uint8>(BeginIndex+1)].AnimPlaybackTime;
				GetOwner<AReplicationTestCharacter>()->LagCompensatedSkeleton->GetAnimInstance()->UpdateAnimation(0.0, false);
				GetOwner<AReplicationTestCharacter>()->LagCompensatedSkeleton->RefreshBoneTransforms();
			}
		}

		if (IsValid(GetWorld()->GetGameState<AReplicationTestGameState>()))
		{
			GetWorld()->GetGameState<AReplicationTestGameState>()->PreviousInterpolationTime = GetWorld()->GetGameState<AReplicationTestGameState>()->CurrentInterpolationTime;
			GetWorld()->GetGameState<AReplicationTestGameState>()->CurrentInterpolationTime = CurrentInterpolationTime;
		}
		CurrentInterpolationTime += DeltaSeconds * InterpolationMultiplier;
	}
}

void UMyCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	if (Safe_bWantsToShoot)
	{
		if (!CharacterOwner->HasAuthority()) { ServerShootRPC(); }
		
		UE_LOG(LogTemp, Warning, TEXT("Server-side pos: [%f %f %f]"), GetOwner()->GetActorLocation().X, GetOwner()->GetActorLocation().Y, GetOwner()->GetActorLocation().Z)
		GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Red, FString::Printf(TEXT("Client-side pos: [%f %f %f]"), GetOwner()->GetActorLocation().X, GetOwner()->GetActorLocation().Y, GetOwner()->GetActorLocation().Z));
		
		Safe_bWantsToShoot = false;
	}
}

void UMyCharacterMovementComponent::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);

	
}

double UMyCharacterMovementComponent::CalculateInterpolationMultiplier()
{
	double MeanX = 0;
	double MeanY = 0;

	for(uint8 i = 1; i <= TimestampBufferWindow; i++)
	{
		uint8 TimestampIndex = static_cast<uint8>(snapshotsReceived-i);
		MeanX += TimestampBuffer[TimestampIndex].ArrivalTime;
		MeanY += TimestampBuffer[TimestampIndex].Timestamp;
	}

	MeanX /= TimestampBufferWindow;
	MeanY /= TimestampBufferWindow;

	double numerator = 0;
	double denominator = 0;

	for(uint8 i = 1; i <= TimestampBufferWindow; i++)
	{
		uint8 TimestampIndex = static_cast<uint8>(snapshotsReceived-i);
		double diffX = TimestampBuffer[TimestampIndex].ArrivalTime - MeanX;
		double diffY = TimestampBuffer[TimestampIndex].Timestamp - MeanY;
		numerator += (diffX) * (diffY);
		denominator += (diffX) * (diffX);
	}

	return numerator / denominator;
}
