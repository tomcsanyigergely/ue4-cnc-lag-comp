// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCharacterMovementComponent.h"

void UMyCharacterMovementComponent::AddSnapshot(float Timestamp, FPlayerSnapshot PlayerSnapshot)
{
	uint8_t NumSnapshots = EndIndex - BeginIndex;
	if (NumSnapshots == 0 || Timestamp > SnapshotBuffer[static_cast<uint8>(EndIndex-1)].Timestamp)
	{
		SnapshotBuffer[EndIndex] = PlayerSnapshot;
		SnapshotBuffer[EndIndex].Timestamp = Timestamp;
		EndIndex++;

		float AvgSnapshotDeltaTime = 1.0f / 20.0f;

		if (!(Timestamp - 5.0 * AvgSnapshotDeltaTime <= TargetTime && TargetTime <= Timestamp - 1.0f * AvgSnapshotDeltaTime))
		{
			TargetTime = Timestamp - 3.0 * AvgSnapshotDeltaTime; // might be negative at the beginning!
			BeginIndex = EndIndex-1;
			while (SnapshotBuffer[BeginIndex].Timestamp > TargetTime && SnapshotBuffer[static_cast<uint8>(BeginIndex-1)].Timestamp < SnapshotBuffer[BeginIndex].Timestamp)
			{
				BeginIndex--;
			}
		}
	}
}

void UMyCharacterMovementComponent::SimulatedTick(float DeltaSeconds)
{
	if (BeginIndex != EndIndex)
	{
		while (SnapshotBuffer[static_cast<uint8>(BeginIndex+1)].Timestamp < TargetTime && static_cast<uint8>(BeginIndex+1) != EndIndex)
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
				float Interp = FMath::Clamp((TargetTime - SnapshotBuffer[BeginIndex].Timestamp) / (SnapshotBuffer[static_cast<uint8>(BeginIndex+1)].Timestamp - SnapshotBuffer[BeginIndex].Timestamp), 0.0f, 1.0f);
				FVector LerpPosition = FMath::Lerp(SnapshotBuffer[BeginIndex].Position, SnapshotBuffer[static_cast<uint8>(BeginIndex+1)].Position, Interp);
				GetOwner()->SetActorLocation(LerpPosition);
			}
			else
			{
				GetOwner()->SetActorLocation(SnapshotBuffer[static_cast<uint8>(BeginIndex+1)].Position);
			}
		}
		
		TargetTime += DeltaSeconds;
	}

	
}
