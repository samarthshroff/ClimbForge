// Copyright (c) Samarth Shroff. All Rights Reserved.
// This work is protected under applicable copyright laws in perpetuity.
// Licensed under the CC-BY-4.0 License. See LICENSE file for details.


#include "CharacterAnimInstance.h"

#include "ClimbForgeCharacter.h"
#include "ClimbForgeMovementComponent.h"

void UCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwnerActor = Cast<AClimbForgeCharacter>(GetOwningActor());
	if (OwnerActor != nullptr)
	{
		OwnerMovementComponent = Cast<UClimbForgeMovementComponent>(OwnerActor->GetClimbForgeMovementComponent());	
	}	
}

void UCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (OwnerActor == nullptr || OwnerMovementComponent == nullptr) return;

	Velocity = OwnerActor->GetVelocity();
	
	// Get the length of X and Y of velocity, so moving up and down does no affect.
	GroundSpeed = Velocity.Size2D();
	AirSpeed = Velocity.Z;	
	ClimbVelocity = OwnerMovementComponent->GetUnrotatedClimbingVelocity();
	
	bShouldMove = OwnerMovementComponent->GetCurrentAcceleration() != FVector::ZeroVector && GroundSpeed > 3.0f;
	bIsFalling = OwnerMovementComponent->IsFalling();
	bIsClimbing = OwnerMovementComponent->IsClimbing();
}
