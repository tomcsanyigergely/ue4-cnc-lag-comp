#pragma once

#include "SnapshotPacketBits.generated.h"

USTRUCT()
struct FSnapshotPacketBits
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	float TimeStamp;

	UPROPERTY(Transient)
	TArray<FVector> Positions;

	UPROPERTY(Transient)
	TArray<AActor*> Actors;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FSnapshotPacketBits> : public TStructOpsTypeTraitsBase2<FSnapshotPacketBits>
{
	enum 
	{
		WithNetSerializer = true,
	};
};