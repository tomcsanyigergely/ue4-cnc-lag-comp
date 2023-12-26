#include "SnapshotPacketBits.h"

bool FSnapshotPacketBits::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	if (!Ar.IsLoading())
	{
		Ar << TimeStamp;		
	}
	else
	{
		Ar << TimeStamp;
	}
	
	return true;
}
