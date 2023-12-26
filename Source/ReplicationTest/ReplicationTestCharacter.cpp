// Copyright Epic Games, Inc. All Rights Reserved.

#include "ReplicationTestCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "MyCharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"

//////////////////////////////////////////////////////////////////////////
// AReplicationTestCharacter

AReplicationTestCharacter::AReplicationTestCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMyCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

void AReplicationTestCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	{
		static const FName ReplicatedMovementPropertyName(TEXT("ReplicatedMovement"));
		static FProperty* ReplicatedMovementProperty = GetReplicatedProperty(StaticClass(), ACharacter::StaticClass(), ReplicatedMovementPropertyName);
		ChangedPropertyTracker.SetCustomIsActiveOverride(this, ReplicatedMovementProperty->RepIndex, false);
	}

	// Workaround: Disable replication of ReplicatedMovementMode since we have a custom movement mode replication that includes jump state info.
	// Cannot use DOREPLIFETIME_ACTIVE_OVERRIDE(ACharacter, ReplicatedMovementMode, false) here because GET_MEMBER_NAME_CHECKED does not support 
	// protected/private members.
	{
		static const FName ReplicatedMovementModePropertyName(TEXT("ReplicatedMovementMode"));
		static FProperty* ReplicatedMovementModeProperty = GetReplicatedProperty(StaticClass(), ACharacter::StaticClass(), ReplicatedMovementModePropertyName);
		ChangedPropertyTracker.SetCustomIsActiveOverride(this, ReplicatedMovementModeProperty->RepIndex, false);
	}

	// Workaround: RemoteViewPitch is taken from ReplicatedLook.Rotation.Pitch
	DOREPLIFETIME_ACTIVE_OVERRIDE(ACharacter, RemoteViewPitch, false);

	// Workaround: Jump state is replicated with movement mode and use of IsJumpForceApplied() is not even reliable as it only covers half of the jump.
	// The server must dictate if a fall is a jump or not in any stage of the fall.
	DOREPLIFETIME_ACTIVE_OVERRIDE(ACharacter, bProxyIsJumpForceApplied, false);

	UCharacterMovementComponent* MyCharacterMovement = GetCharacterMovement();
	check(MyCharacterMovement);
	if (MyCharacterMovement->CurrentRootMotion.HasActiveRootMotionSources() || IsPlayingNetworkedRootMotionMontage())
	{
		const FAnimMontageInstance* RootMotionMontageInstance = GetRootMotionAnimMontageInstance();

		RepRootMotion.bIsActive = true;
		// Is position stored in local space?
		RepRootMotion.bRelativePosition = BasedMovement.HasRelativeLocation();
		RepRootMotion.bRelativeRotation = BasedMovement.HasRelativeRotation();
		RepRootMotion.Location = RepRootMotion.bRelativePosition ? BasedMovement.Location : FRepMovement::RebaseOntoZeroOrigin(GetActorLocation(), GetWorld()->OriginLocation);
		RepRootMotion.Rotation = RepRootMotion.bRelativeRotation ? BasedMovement.Rotation : GetActorRotation();
		RepRootMotion.MovementBase = BasedMovement.MovementBase;
		RepRootMotion.MovementBaseBoneName = BasedMovement.BoneName;
		if (RootMotionMontageInstance)
		{
			RepRootMotion.AnimMontage = RootMotionMontageInstance->Montage;
			RepRootMotion.Position = RootMotionMontageInstance->GetPosition();
		}
		else
		{
			RepRootMotion.AnimMontage = nullptr;
		}

		RepRootMotion.AuthoritativeRootMotion = MyCharacterMovement->CurrentRootMotion;
		RepRootMotion.Acceleration = MyCharacterMovement->GetCurrentAcceleration();
		RepRootMotion.LinearVelocity = MyCharacterMovement->Velocity;

		DOREPLIFETIME_ACTIVE_OVERRIDE(ACharacter, RepRootMotion, true);
	}
	else
	{
		RepRootMotion.Clear();

		DOREPLIFETIME_ACTIVE_OVERRIDE(ACharacter, RepRootMotion, false);
	}

	// Workaround: Jump state is replicated with movement mode and use of IsJumpForceApplied() is not even reliable as it only covers half of the jump.
	// The server must dictate if a fall is a jump or not in any stage of the fall.
	// bProxyIsJumpForceApplied = (JumpForceTimeRemaining > 0.0f);
	ReplicatedServerLastTransformUpdateTimeStamp = MyCharacterMovement->GetServerLastTransformUpdateTimeStamp();

	ReplicatedBasedMovement = BasedMovement;

	// Optimization: only update and replicate these values if they are actually going to be used.
	if (BasedMovement.HasRelativeLocation())
	{
		// When velocity becomes zero, force replication so the position is updated to match the server (it may have moved due to simulation on the client).
		ReplicatedBasedMovement.bServerHasVelocity = !MyCharacterMovement->Velocity.IsZero();

		// Make sure absolute rotations are updated in case rotation occurred after the base info was saved.
		if (!BasedMovement.HasRelativeRotation())
		{
			ReplicatedBasedMovement.Rotation = GetActorRotation();
		}
	}

	// Save bandwidth by not replicating this value unless it is necessary, since it changes every update.
	if ((MyCharacterMovement->NetworkSmoothingMode == ENetworkSmoothingMode::Linear) || MyCharacterMovement->bNetworkAlwaysReplicateTransformUpdateTimestamp)
	{
		ReplicatedServerLastTransformUpdateTimeStamp = MyCharacterMovement->GetServerLastTransformUpdateTimeStamp();
	}
	else
	{
		ReplicatedServerLastTransformUpdateTimeStamp = 0.f;
	}
}

void AReplicationTestCharacter::OnRep_ReplicatedMovement() {}

void AReplicationTestCharacter::OnRep_ReplicatedBasedMovement() {}

//////////////////////////////////////////////////////////////////////////
// Input

void AReplicationTestCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AReplicationTestCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AReplicationTestCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AReplicationTestCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AReplicationTestCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AReplicationTestCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AReplicationTestCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AReplicationTestCharacter::OnResetVR);
}

void AReplicationTestCharacter::OnResetVR()
{
	// If ReplicationTest is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in ReplicationTest.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AReplicationTestCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AReplicationTestCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AReplicationTestCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AReplicationTestCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AReplicationTestCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AReplicationTestCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
