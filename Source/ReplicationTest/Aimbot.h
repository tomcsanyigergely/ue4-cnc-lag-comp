// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Aimbot.generated.h"

UCLASS()
class REPLICATIONTEST_API AAimbot : public APawn
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess))
	USceneComponent* CameraComponent;

	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
	float ShootInterval = 1.0f;

	FTimerHandle ShootTimer;
	bool ShootSignal = false;

	class AReplicationTestCharacter* Target = nullptr;
	class ADefaultCharacter* DefaultTarget = nullptr;

public:
	// Sets default values for this pawn's properties
	AAimbot();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	
private:
	UFUNCTION()
	void Shoot();

	UFUNCTION(Server, Reliable)
	void ShootServerRPC(FVector Location, FVector Direction, float TargetRewindTime);

	UFUNCTION(Client, Reliable)
	void HitboxRPC(const TArray<FVector>& Locations, const TArray<FRotator>& Rotations);

	UFUNCTION(Client, Reliable)
	void HitboxDefaultTargetRPC(const TArray<FVector>& Locations, const TArray<FRotator>& Rotations);
};
