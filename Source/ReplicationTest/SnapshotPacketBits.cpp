#include "SnapshotPacketBits.h"

bool FSnapshotPacketBits::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	if (!Ar.IsLoading())
	{
		Ar << TimeStamp;
		uint8 NumPlayerSnapshots = PlayerSnapshots.Num();
		Ar << NumPlayerSnapshots;

		for(int i = 0; i < NumPlayerSnapshots; i++)
		{
			Ar << PlayerSnapshots[i].PlayerId;
			PlayerSnapshots[i].Position.NetSerialize(Ar, Map, bOutSuccess);
			Ar << PlayerSnapshots[i].AnimPlaybackTime;
		}
	}
	else
	{
		PlayerSnapshots.Empty();
		
		Ar << TimeStamp;
		uint8 NumPlayerSnapshots;
		Ar << NumPlayerSnapshots;
		PlayerSnapshots.SetNum(NumPlayerSnapshots);

		for(int i = 0; i < NumPlayerSnapshots; i++)
		{
			Ar << PlayerSnapshots[i].PlayerId;
			PlayerSnapshots[i].Position.NetSerialize(Ar, Map, bOutSuccess);
			Ar << PlayerSnapshots[i].AnimPlaybackTime;
		}
	}
	
	return true;
}
