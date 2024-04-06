#pragma once

#include "SnapshotPacketBits.generated.h"

struct FPlayerSnapshot
{
	uint8 PlayerId;
	FVector Position;
	float AnimPlaybackTime;
	float Timestamp = 0; // used locally only, not serialized
};

USTRUCT()
struct FSnapshotPacketBits
{
	GENERATED_BODY()

	float TimeStamp;

	TArray<FPlayerSnapshot> PlayerSnapshots;

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