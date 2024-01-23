// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCharacterMovementComponent.h"

#include "ReplicationTestGameState.h"

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

	if (NumSnapshots == 2 && SnapshotBuffer[SnapshotIndex].Timestamp != 0)
	{
		float Interp = FMath::Clamp((RewindTime - SnapshotBuffer[SnapshotIndex].Timestamp) / (SnapshotBuffer[static_cast<uint8>(SnapshotIndex+1)].Timestamp - SnapshotBuffer[SnapshotIndex].Timestamp), 0.0f, 1.0f);
		FVector LerpPosition = FMath::Lerp(SnapshotBuffer[SnapshotIndex].Position, SnapshotBuffer[static_cast<uint8>(SnapshotIndex+1)].Position, Interp);
		GetOwner()->SetActorLocation(LerpPosition);
	}
}

void UMyCharacterMovementComponent::ResetPose()
{
	GetOwner()->SetActorLocation(SavedPoseLocation);
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
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, FString::Printf(TEXT("UH OH")));
		}
		else
		{
			if (SnapshotBuffer[BeginIndex].Timestamp != 0)
			{
				float Interp = FMath::Clamp((CurrentInterpolationTime - SnapshotBuffer[BeginIndex].Timestamp) / (SnapshotBuffer[static_cast<uint8>(BeginIndex+1)].Timestamp - SnapshotBuffer[BeginIndex].Timestamp), 0.0, 1.0);
				FVector LerpPosition = FMath::Lerp(SnapshotBuffer[BeginIndex].Position, SnapshotBuffer[static_cast<uint8>(BeginIndex+1)].Position, Interp);
				GetOwner()->SetActorLocation(LerpPosition);
			}
			else
			{
				GetOwner()->SetActorLocation(SnapshotBuffer[static_cast<uint8>(BeginIndex+1)].Position);
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
