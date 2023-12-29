// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCharacterMovementComponent.h"

void UMyCharacterMovementComponent::AddSnapshot(float Timestamp, FPlayerSnapshot PlayerSnapshot)
{	
	if (Timestamp > SnapshotBuffer[static_cast<uint8>(EndIndex-1)].Timestamp)
	{
		SnapshotBuffer[EndIndex] = PlayerSnapshot;
		SnapshotBuffer[EndIndex].Timestamp = Timestamp;
		SnapshotBuffer[EndIndex].ArrivalTime = GetWorld()->GetTimeSeconds();
		EndIndex++;

		snapshotsReceived++;

		if (snapshotsReceived >= 20)
		{
			CalculateLineOfBestFit();

			double TargetInterpolationTime = slope * GetWorld()->GetTimeSeconds() + intercept - 3.0 / 20.0 - 0.05;
			CurrentInterpolationTime = TargetInterpolationTime;
			BeginIndex = static_cast<uint8>(EndIndex-1);

			while(SnapshotBuffer[BeginIndex].Timestamp > TargetInterpolationTime && SnapshotBuffer[static_cast<uint8>(BeginIndex-1)].Timestamp < SnapshotBuffer[BeginIndex].Timestamp)
			{
				BeginIndex--;
			}
			/*double InterpolationTimeDifference = TargetInterpolationTime - CurrentInterpolationTime;
			InterpolationMultiplier = (InterpolationTimeDifference + 1.0f / 20.0f) / (1.0f / 20.0f);*/
			
		}
	}
}

void UMyCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Yellow, FString::Printf(TEXT("CMC::TickComponent(): %f"), GetWorld()->GetTimeSeconds()));
}

void UMyCharacterMovementComponent::SimulatedTick(float DeltaSeconds) // on the same Tick, executes earlier than AddSnapshot()
{	
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
		
		CurrentInterpolationTime += DeltaSeconds;
	}
}

void UMyCharacterMovementComponent::CalculateLineOfBestFit()
{
	double MeanX = 0;
	double MeanY = 0;

	for(uint8 i = 1; i <= 20; i++)
	{
		uint8 SnapshotIndex = static_cast<uint8>(EndIndex-i);
		MeanX += SnapshotBuffer[SnapshotIndex].ArrivalTime;
		MeanY += SnapshotBuffer[SnapshotIndex].Timestamp;
	}

	MeanX /= 20;
	MeanY /= 20;

	double numerator = 0;
	double denominator = 0;

	for(uint8 i = 1; i <= 20; i++)
	{
		uint8 SnapshotIndex = static_cast<uint8>(EndIndex-i);
		double diffX = SnapshotBuffer[SnapshotIndex].ArrivalTime - MeanX;
		double diffY = SnapshotBuffer[SnapshotIndex].Timestamp - MeanY;
		numerator += (diffX) * (diffY);
		denominator += (diffX) * (diffX);
	}

	slope = numerator / denominator;
	intercept = MeanY - slope * MeanX;
}
