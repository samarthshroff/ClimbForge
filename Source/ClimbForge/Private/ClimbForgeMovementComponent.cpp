// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbForgeMovementComponent.h"

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

	ClimbableSurfacesHits = CapsuleSweepTraceByChannel(Start, End, true, true);
	return !ClimbableSurfacesHits.IsEmpty();
}

FHitResult UClimbForgeMovementComponent::TraceFromEyeHeight(const float TraceDistance, const float TraceStartOffset)
{
	const FVector EyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + TraceStartOffset);
	const FVector Start = UpdatedComponent->GetComponentLocation() + EyeHeightOffset;
	const FVector End = Start + (UpdatedComponent->GetForwardVector() * TraceDistance);
	return LineTraceByChannel(Start, End, true, true);
}
#pragma endregion




