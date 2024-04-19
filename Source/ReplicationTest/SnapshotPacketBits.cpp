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
			
			for(int j = 0; j < PlayerSnapshots[i].Anim.Num(); j++)
			{
				uint8 LastAnim = (j == PlayerSnapshots[i].Anim.Num()-1 && PlayerSnapshots[i].BlendAnim.Num() == 0);
				Ar.SerializeBits(&LastAnim, 1);

				Ar << PlayerSnapshots[i].Anim[j].Id;
				Ar << PlayerSnapshots[i].Anim[j].Time;

				if (!LastAnim)
				{
					Ar << PlayerSnapshots[i].Anim[j].Weight;
				}
			}

			for(int j = 0; j < PlayerSnapshots[i].BlendAnim.Num(); j++)
			{
				uint8 LastAnim = (j == PlayerSnapshots[i].BlendAnim.Num()-1);
				Ar.SerializeBits(&LastAnim, 1);

				Ar << PlayerSnapshots[i].BlendAnim[j].Id;
				Ar << PlayerSnapshots[i].BlendAnim[j].NormalizedTime;
				Ar << PlayerSnapshots[i].BlendAnim[j].NormalizedBlendX;

				if (!LastAnim)
				{
					Ar << PlayerSnapshots[i].BlendAnim[j].Weight;
				}
			}
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

			uint8 LastAnim;
			float SumWeights = 0;

			do
			{
				Ar.SerializeBits(&LastAnim, 1);

				

				uint8 Id;

				Ar << Id;

				if (Id == 1)
				{
					FBlendSpaceAnimSnapshot AnimSnapshot;
					
					AnimSnapshot.Id = Id;
					Ar << AnimSnapshot.NormalizedTime;
					Ar << AnimSnapshot.NormalizedBlendX;

					if (!LastAnim)
					{
						Ar << AnimSnapshot.Weight;
						SumWeights += AnimSnapshot.Weight;
					}
					else
					{
						AnimSnapshot.Weight = FMath::Clamp(1.0 - SumWeights, 0.0, 1.0);
					}

					PlayerSnapshots[i].BlendAnim.Add(AnimSnapshot);
				}
				else
				{
					FAnimSnapshot AnimSnapshot;

					AnimSnapshot.Id = Id;
					Ar << AnimSnapshot.Time;

					if (!LastAnim)
					{
						Ar << AnimSnapshot.Weight;
						SumWeights += AnimSnapshot.Weight;
					}
					else
					{
						AnimSnapshot.Weight = FMath::Clamp(1.0 - SumWeights, 0.0, 1.0);
					}
					
					PlayerSnapshots[i].Anim.Add(AnimSnapshot);
				}

				//UE_LOG(LogTemp, Warning, TEXT("Anim Snapshot: %d %f %f"), AnimSnapshot.Id, AnimSnapshot.Time, AnimSnapshot.Weight)

				
			}
			while(!LastAnim);
		}
	}
	
	return true;
}
