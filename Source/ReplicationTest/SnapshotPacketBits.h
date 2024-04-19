#pragma once

#include "SnapshotPacketBits.generated.h"

struct FAnimSnapshot
{
	uint8 Id;
	float Time;
	float Weight;
};

struct FBlendSpaceAnimSnapshot
{
	uint8 Id;
	float NormalizedTime;
	float Weight;
	float NormalizedBlendX;
};

struct FPlayerSnapshot
{
	uint8 PlayerId;
	FVector Position;
	float AnimPlaybackTime;
	TArray<FAnimSnapshot> Anim;
	TArray<FBlendSpaceAnimSnapshot> BlendAnim;

	// used locally only, not serialized:
	float Timestamp = 0;
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