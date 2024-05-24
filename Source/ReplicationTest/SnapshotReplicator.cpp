// Fill out your copyright notice in the Description page of Project Settings.


#include "SnapshotReplicator.h"

#include "MyCharacterMovementComponent.h"
#include "ReplicationTestCharacter.h"
#include "ReplicationTestGameState.h"
#include "ReplicationTestPlayerState.h"
#include "Animation/AnimNode_StateMachine.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

// Sets default values
ASnapshotReplicator::ASnapshotReplicator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bAlwaysRelevant = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
}

// Called when the game starts or when spawned
void ASnapshotReplicator::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		GetWorld()->GetTimerManager().SetTimer(PingTimer, this, &ASnapshotReplicator::ShowPing, 1.0f, true);
	}

	//AddTickPrerequisiteComponent(...);
}

// Called every frame
void ASnapshotReplicator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() == ROLE_Authority)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Yellow, FString::Printf(TEXT("SnapshotReplicator::Tick(): %f"), GetWorld()->GetTimeSeconds()));

		//UE_LOG(LogTemp, Warning, TEXT("SnapshotReplicator::Tick(): %f"), GetWorld()->GetTimeSeconds())
		
		static int tickCount = 0;

		if (tickCount == 3)
		{
			tickCount = 0;
			float ServerTime = GetWorld()->GetTimeSeconds();
			FSnapshotPacketBits SnapshotPacketBits;
			SnapshotPacketBits.TimeStamp = ServerTime;

			for (const APlayerState* PlayerState : GetWorld()->GetGameState()->PlayerArray)
			{
				AReplicationTestCharacter* Character = PlayerState->GetPawn<AReplicationTestCharacter>();
				const AReplicationTestPlayerState* RepTestPlayerState = Cast<AReplicationTestPlayerState>(PlayerState);
				if (IsValid(Character) && IsValid(RepTestPlayerState) && RepTestPlayerState->RepTestPlayerId != 0)
				{
					FPlayerSnapshot PlayerSnapshot;
					PlayerSnapshot.PlayerId = RepTestPlayerState->RepTestPlayerId;
					PlayerSnapshot.Yaw = Character->GetActorRotation().Yaw;
					PlayerSnapshot.Position = Character->GetActorLocation();
					PlayerSnapshot.AnimPlaybackTime = 2.0 * ServerTime;

					UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();				

					int32 MachineIndex;
					const FBakedAnimationStateMachine* BakedAnimationStateMachine;
					AnimInstance->GetStateMachineIndexAndDescription("Default", MachineIndex, &BakedAnimationStateMachine);

					auto States = BakedAnimationStateMachine->States;

					for(auto State : States)
					{
						int32 StateIndex = BakedAnimationStateMachine->FindStateIndex(State.StateName);
						float StateWeight = AnimInstance->GetInstanceStateWeight(MachineIndex, StateIndex);

						if (StateWeight > 0)
						{
							if (State.StateName == "Idle/Run")
							{
								FBlendSpaceAnimSnapshot AnimSnapshot;
								AnimSnapshot.Id = 1;
								AnimSnapshot.NormalizedTime = AnimInstance->GetInstanceAssetPlayerTimeFraction(State.PlayerNodeIndices[0]);
								AnimSnapshot.Weight = StateWeight;
								AnimSnapshot.NormalizedBlendX = Character->GetMyCharacterMovementComponent()->CopyOfIdleBlendSpaceX;
								PlayerSnapshot.BlendAnim.Add(AnimSnapshot);
							}
							else if (State.StateName == "JumpStart")
							{
								FAnimSnapshot AnimSnapshot;
								AnimSnapshot.Id = 2;
								AnimSnapshot.Time = AnimInstance->GetInstanceAssetPlayerTimeFraction(State.PlayerNodeIndices[0]);
								AnimSnapshot.Weight = StateWeight;
								PlayerSnapshot.Anim.Add(AnimSnapshot);
							}
							else if (State.StateName == "JumpLoop")
							{
								FAnimSnapshot AnimSnapshot;
								AnimSnapshot.Id = 3;
								AnimSnapshot.Time = AnimInstance->GetInstanceAssetPlayerTimeFraction(State.PlayerNodeIndices[0]);
								AnimSnapshot.Weight = StateWeight;
								PlayerSnapshot.Anim.Add(AnimSnapshot);
							}
							else if (State.StateName == "JumpEnd")
							{
								FAnimSnapshot AnimSnapshot;
								AnimSnapshot.Id = 4;
								AnimSnapshot.Time = AnimInstance->GetInstanceAssetPlayerTimeFraction(State.PlayerNodeIndices[0]);
								AnimSnapshot.Weight = StateWeight;
								PlayerSnapshot.Anim.Add(AnimSnapshot);
							}
						}
					}
					
					SnapshotPacketBits.PlayerSnapshots.Add(PlayerSnapshot);

					Character->GetMyCharacterMovementComponent()->AddServerSideSnapshot(ServerTime, PlayerSnapshot);

					//UE_LOG(LogTemp, Warning, TEXT("AnimTimes: %f %f %f %f"), PlayerSnapshot.BlendAnim[0].NormalizedTime, PlayerSnapshot.Anim[0].Time, PlayerSnapshot.Anim[1].Time, PlayerSnapshot.Anim[2].Time);
				}
			}

			MulticastSnapshotRPC(SnapshotPacketBits);
			ForceNetUpdate();
		}
		
		tickCount++;
	}
}

void ASnapshotReplicator::ShowPing()
{
	GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Yellow, FString::Printf(TEXT("Ping: %d"), GetWorld()->GetFirstPlayerController()->GetPlayerState<APlayerState>()->GetPing() * 4));;
}

void ASnapshotReplicator::MulticastSnapshotRPC_Implementation(FSnapshotPacketBits SnapshotPacketBits)
{
	static int messageCount = 0;
	static float lastMessageTime = 0;
	
	if (GetLocalRole() == ROLE_SimulatedProxy && IsValid(GetWorld()->GetGameState<AReplicationTestGameState>()))
	{
		AReplicationTestPlayerState* LocalPlayerState = GetWorld()->GetFirstPlayerController()->GetPlayerState<AReplicationTestPlayerState>();

		if (IsValid(LocalPlayerState))
		{
			for(const FPlayerSnapshot& PlayerSnapshot : SnapshotPacketBits.PlayerSnapshots)
			{
				if (PlayerSnapshot.PlayerId != 0 && LocalPlayerState->RepTestPlayerId != PlayerSnapshot.PlayerId)
				{
					for(APlayerState* PlayerState : GetWorld()->GetGameState()->PlayerArray)
					{						
						AReplicationTestPlayerState* RepTestPlayerState = Cast<AReplicationTestPlayerState>(PlayerState);
						if (IsValid(RepTestPlayerState) && RepTestPlayerState->RepTestPlayerId != 0 && RepTestPlayerState->RepTestPlayerId == PlayerSnapshot.PlayerId)
						{							
							AReplicationTestCharacter* Character = PlayerState->GetPawn<AReplicationTestCharacter>();
							if (IsValid(Character))
							{
								Character->GetMyCharacterMovementComponent()->AddClientSideSnapshot(SnapshotPacketBits.TimeStamp, PlayerSnapshot);
							}
							break;
						}
					}
				}
			}

			messageCount++;
			float Now = GetWorld()->GetTimeSeconds();
			//GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Red, FString::Printf(TEXT("Message count: %d %f"), messageCount, (Now-lastMessageTime) * 1000.0f));
			lastMessageTime = Now;
		}
	}
}

