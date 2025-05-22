// Copyright (c) Samarth Shroff. All Rights Reserved.
// This work is protected under applicable copyright laws in perpetuity.
// Licensed under the CC-BY-4.0 License. See LICENSE file for details.

#include "ClimbForgeMovementComponent.h"

#include "CustomMovementMode.h"
#include "DebugHelper.h"
#include "KismetTraceUtils.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

void UClimbForgeMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	ClimbQueryParams.AddIgnoredActor(GetOwner());
}

void UClimbForgeMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// TraceClimbableSurfaces();
	// TraceFromEyeHeight(100.0f);
}

void UClimbForgeMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	if (IsClimbing())
	{
		bOrientRotationToMovement = false;
		// Half of 96.0f that is in AClimbForgeCharacter constructor.
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(48.0f);
	}

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == MOVE_Climbing)
	{
		bOrientRotationToMovement = true;		
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(96.0f);
		StopMovementImmediately();
	}
	
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

void UClimbForgeMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	if (IsClimbing())
	{
		PhysClimbing(DeltaTime, Iterations);
	}
	Super::PhysCustom(DeltaTime, Iterations);
}

#pragma region ClimbTraces
TArray<FHitResult> UClimbForgeMovementComponent::CapsuleSweepTraceByChannel(const FVector& Start, const FVector& End, const bool bShowDebugShape, const bool bShowPersistent)
{
	TArray<FHitResult> OutCapsuleTraceHitResult;

	const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(CollisionCapsuleRadius, CollisionCapsuleHalfHeight);	
	
	const bool bHit = GetWorld()->SweepMultiByChannel(OutCapsuleTraceHitResult, Start, End, FQuat::Identity, ClimbableSurfaceTraceChannel, CollisionShape, 
	ClimbQueryParams);

	EDrawDebugTrace::Type DebugTraceType = EDrawDebugTrace::None;
	if (bShowDebugShape)
	{
		DebugTraceType = EDrawDebugTrace::ForOneFrame;
		if (bShowPersistent)
		{
			DebugTraceType = EDrawDebugTrace::Persistent;
		}
		DrawDebugCapsuleTraceMulti(GetWorld(), Start, End, CollisionCapsuleRadius, CollisionCapsuleHalfHeight, DebugTraceType, bHit,
			OutCapsuleTraceHitResult, FLinearColor::Red, FLinearColor::Green, 5.0f);
	}	
	return OutCapsuleTraceHitResult;
}

FHitResult UClimbForgeMovementComponent::LineTraceByChannel(const FVector& Start, const FVector& End, const bool bShowDebugShape, const bool bShowPersistent)
{
	FHitResult OutLineTrace;

	const bool bHit = GetWorld()->LineTraceSingleByChannel(OutLineTrace, Start, End, ClimbableSurfaceTraceChannel, ClimbQueryParams);

	EDrawDebugTrace::Type DebugTraceType = EDrawDebugTrace::None;
	if (bShowDebugShape)
	{
		DebugTraceType = EDrawDebugTrace::ForOneFrame;
		if (bShowPersistent)
		{
			DebugTraceType = EDrawDebugTrace::Persistent;
		}
		DrawDebugLineTraceSingle(GetWorld(), Start, End, DebugTraceType, bHit, OutLineTrace, FLinearColor::Red, FLinearColor::Green, 5.0f);
	}
	
	return OutLineTrace;
}
#pragma endregion

#pragma region ClimbCore

void UClimbForgeMovementComponent::ToggleClimbing(const bool bEnableClimb)
{
	if (bEnableClimb)
	{
		if (CanStartClimbing())
		{
			// Enter Climb
			Debug::Print(TEXT("Can Climb"));
			StartClimbing();
		}
		else
		{
			Debug::Print(TEXT("Cannot Climb"));
		}
	}
	else
	{
		// Stop climb
		StopClimbing();
	}
}

bool UClimbForgeMovementComponent::CanStartClimbing()
{
	if (IsFalling()) return false;
	if (!TraceClimbableSurfaces()) return false;
	if (!TraceFromEyeHeight(100.0f).bBlockingHit) return false;

	return true;
}

void UClimbForgeMovementComponent::StartClimbing()
{
	SetMovementMode(MOVE_Custom, MOVE_Climbing);
}

void UClimbForgeMovementComponent::StopClimbing()
{
	SetMovementMode(MOVE_Falling);
}

bool UClimbForgeMovementComponent::IsClimbing() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == MOVE_Climbing;
}

bool UClimbForgeMovementComponent::TraceClimbableSurfaces()
{
	// Don't want to start right from the character location but a few units in front.
	const FVector StartOffset = UpdatedComponent->GetForwardVector() * 25.0f;
	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;

	// Don't want too much distance between start and end as need to generate fewer capsules for collision. 
	// In this case 2 start and end as the forward vector is a unit vector.
	const FVector End = Start + UpdatedComponent->GetForwardVector();

	ClimbableSurfacesHits = CapsuleSweepTraceByChannel(Start, End, true);
	return !ClimbableSurfacesHits.IsEmpty();
}

FHitResult UClimbForgeMovementComponent::TraceFromEyeHeight(const float TraceDistance, const float TraceStartOffset)
{
	const FVector EyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + TraceStartOffset);
	const FVector Start = UpdatedComponent->GetComponentLocation() + EyeHeightOffset;
	const FVector End = Start + (UpdatedComponent->GetForwardVector() * TraceDistance);
	return LineTraceByChannel(Start, End);
}

void UClimbForgeMovementComponent::PhysClimbing(const float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	// TODO - Process all climbable surfaces info
	TraceClimbableSurfaces();
	ProcessClimbableSurfaces();
	// TODO - Check to see if climbing needs to stop
	RestorePreAdditiveRootMotionVelocity();

	if( !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() )
	{
		// TODO - define max climb speed and acceleration
		CalcVelocity(DeltaTime, Friction, true, MaxBrakeClimbingDeceleration);
	}

	ApplyRootMotionToVelocity(DeltaTime);

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * DeltaTime;
	FHitResult Hit(1.f);

	// TODO - Handle Climb rotation.
	SafeMoveUpdatedComponent(Adjusted, UpdatedComponent->GetComponentQuat(), true, Hit);

	if (Hit.Time < 1.f)
	{
		HandleImpact(Hit, DeltaTime, Adjusted);
		SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
	}

	if(!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / DeltaTime;
	}

	// TODO - Snap movement to climbable surfaces.
}

void UClimbForgeMovementComponent::ProcessClimbableSurfaces()
{
	ClimbableSurfaceLocation = FVector::ZeroVector;
	ClimbableSurfaceNormal = FVector::ZeroVector;

	if (ClimbableSurfacesHits.IsEmpty()) return;

	for (FHitResult SurfaceHits : ClimbableSurfacesHits)
	{
		ClimbableSurfaceLocation += SurfaceHits.ImpactPoint;
		ClimbableSurfaceNormal += SurfaceHits.ImpactNormal;
	}

	ClimbableSurfaceLocation /= ClimbableSurfacesHits.Num();
	ClimbableSurfaceNormal = ClimbableSurfaceNormal.GetSafeNormal();

	Debug::Print(TEXT("ClimbableSurfaceLocation:: ")+ ClimbableSurfaceLocation.ToCompactString(), FColor::Red, 1.0f);
	Debug::Print(TEXT("ClimbableSurfaceNormal:: ")+ ClimbableSurfaceNormal.ToCompactString(), FColor::Orange, 2.0f);
}
#pragma endregion




