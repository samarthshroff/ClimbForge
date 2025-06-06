// Copyright (c) Samarth Shroff. All Rights Reserved.
// This work is protected under applicable copyright laws in perpetuity.
// Licensed under the CC BY-NC-ND 4.0 License. See LICENSE file for details.

#include "ClimbForgeCharacter.h"

#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ClimbForgeMovementComponent.h"
#include "DebugHelper.h"
#include "MotionWarpingComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AClimbForgeCharacter

AClimbForgeCharacter::AClimbForgeCharacter(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.SetDefaultSubobjectClass<UClimbForgeMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(35.f, 90.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	ClimbForgeMovementComponent = Cast<UClimbForgeMovementComponent>(GetCharacterMovement());

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AClimbForgeCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (ClimbForgeMovementComponent != nullptr)
	{
		ClimbForgeMovementComponent->OnEnterClimbingMode.BindUObject(this, &AClimbForgeCharacter::OnEnterClimbingMode);
		ClimbForgeMovementComponent->OnExitClimbingMode.BindUObject(this, &AClimbForgeCharacter::OnExitClimbingMode);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AClimbForgeCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	AddInputMappingContext(DefaultMappingContext, 0);
}

void AClimbForgeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AClimbForgeCharacter::HandleGroundMovement);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AClimbForgeCharacter::Look);

		// Climb Action
		EnhancedInputComponent->BindAction(ClimbAction, ETriggerEvent::Started, this, &AClimbForgeCharacter::ClimbStarted);
		EnhancedInputComponent->BindAction(ClimbMoveAction, ETriggerEvent::Triggered, this, &AClimbForgeCharacter::HandleClimbingMovement);
		EnhancedInputComponent->BindAction(ClimbHopAction, ETriggerEvent::Started, this, &AClimbForgeCharacter::ClimbHopStarted);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AClimbForgeCharacter::HandleGroundMovement(const FInputActionValue& Value)
{
	// input is a Vector2D
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}	
}

void AClimbForgeCharacter::HandleClimbingMovement(const FInputActionValue& Value)
{
	// input is a Vector2D
	const FVector2D MovementVector = Value.Get<FVector2D>();
	
	// Using the Left hand (instead of right hand) rule for cross product 
	// get forward vector
	const FVector ForwardDirection = FVector::CrossProduct(-1.0f*ClimbForgeMovementComponent->GetClimbableSurfaceNormal(), GetActorRightVector());
	
	// get right vector 
	const FVector RightDirection = FVector::CrossProduct(-1.0f*ClimbForgeMovementComponent->GetClimbableSurfaceNormal(), -1.0f*GetActorUpVector());

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void AClimbForgeCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AClimbForgeCharacter::ClimbStarted(const FInputActionValue& Value)
{
	if (ClimbForgeMovementComponent == nullptr) return;
	ClimbForgeMovementComponent->ToggleClimbing(!ClimbForgeMovementComponent->IsClimbing());
}

void AClimbForgeCharacter::ClimbHopStarted(const FInputActionValue& Value)
{
	if (ClimbForgeMovementComponent == nullptr) return;
	Debug::Print(TEXT("Climb Dash Requested"), FColor::Red, 1);
	ClimbForgeMovementComponent->RequestClimbDash();
}

void AClimbForgeCharacter::OnEnterClimbingMode()
{
	AddInputMappingContext(ClimbingMappingContext, 1);
}

void AClimbForgeCharacter::OnExitClimbingMode()
{
	RemoveInputMappingContext(ClimbingMappingContext);
}

void AClimbForgeCharacter::AddInputMappingContext(const UInputMappingContext* InContext, const int32 InPriority)
{
	check(InContext);
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InContext, InPriority);
		}
	}
}

void AClimbForgeCharacter::RemoveInputMappingContext(const UInputMappingContext* InContext)
{
	check(InContext);
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (Subsystem->HasMappingContext(InContext))
			{
				Subsystem->RemoveMappingContext(InContext);
			}			
		}
	}
}

