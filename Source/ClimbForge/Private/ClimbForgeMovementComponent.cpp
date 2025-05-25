// Copyright (c) Samarth Shroff. All Rights Reserved.
// This work is protected under applicable copyright laws in perpetuity.
// Licensed under the CC-BY-4.0 License. See LICENSE file for details.

#include "ClimbForgeMovementComponent.h"

#include "CustomMovementMode.h"
#include "DebugHelper.h"
#include "KismetTraceUtils.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

#pragma region Overridden Functions
void UClimbForgeMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	ClimbQueryParams.AddIgnoredActor(GetOwner());

	OwnerColliderCapsuleHalfHeight = GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	
	OwnerActorAnimInstance = GetCharacterOwner()->GetMesh()->GetAnimInstance();
	if (OwnerActorAnimInstance != nullptr)
	{
		OwnerActorAnimInstance->OnMontageEnded.AddDynamic(this, &UClimbForgeMovementComponent::MontageEnded);
		OwnerActorAnimInstance->OnMontageBlendingOut.AddDynamic(this, &UClimbForgeMovementComponent::MontageEnded);		
	}
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
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(OwnerColliderCapsuleHalfHeight*0.5f);
	}

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == MOVE_Climbing)
	{
		bOrientRotationToMovement = true;		
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(OwnerColliderCapsuleHalfHeight);

		const FRotator CurrentRotation = UpdatedComponent->GetComponentRotation();
		const FRotator CorrectRotation = FRotator(0.0f, CurrentRotation.Yaw, 0.0f);
		UpdatedComponent->SetRelativeRotation(CorrectRotation);
		
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

float UClimbForgeMovementComponent::GetMaxSpeed() const
{
	if (IsClimbing())
	{
		return MaxClimbSpeed;
	}
	return Super::GetMaxSpeed();
}

float UClimbForgeMovementComponent::GetMaxAcceleration() const
{
	if (IsClimbing())
	{
		return MaxClimbAcceleration;
	}
	return Super::GetMaxAcceleration();
}
#pragma endregion 

#pragma region ClimbTraces
TArray<FHitResult> UClimbForgeMovementComponent::CapsuleSweepTraceByChannel(const FVector& Start, const FVector& End, const bool bShowDebugShape, const bool bShowPersistent)
{
	TArray<FHitResult> OutCapsuleTraceHitResult;

	const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(ClimbCollisionCapsuleRadius, ClimbCollisionCapsuleHalfHeight);	
	
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
		DrawDebugCapsuleTraceMulti(GetWorld(), Start, End, ClimbCollisionCapsuleRadius, ClimbCollisionCapsuleHalfHeight, DebugTraceType, bHit,
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
			PlayMontage(IdleToClimbMontage);
			//StartClimbing();
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
	
	// const float SurfaceSteepness = FVector::DotProduct(ClimbableSurfaceNormal, UpdatedComponent->GetForwardVector());
	// const float TraceDistance = 100.0f;
	// const float SteepnessMultiplier = 1.0f+(1.0f-SurfaceSteepness)*5.0f;	
	// if (!TraceFromEyeHeight(TraceDistance*SteepnessMultiplier).bBlockingHit) return false;

	return true;
}

bool UClimbForgeMovementComponent::ShouldStopClimbing()
{
	if (ClimbableSurfacesHits.IsEmpty()) return true;

	// This is theta = acos(a.b/|a|x|b|) as both the vectors are normalized or unit vector their magnitudes will be 1 thus 
	// theta = acos(a.b)
	const float DotResult = FVector::DotProduct(ClimbableSurfaceNormal, FVector::UpVector);
	const float AngleInDeg = FMath::RadiansToDegrees(FMath::Acos(DotResult));

	if (AngleInDeg <= MinimumClimbableAngleInDegrees)
	{
		return true;
	}
	return false;
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

	ClimbableSurfacesHits = CapsuleSweepTraceByChannel(Start, End);
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
	if (ShouldStopClimbing())
	{
		StopClimbing();
	}
	
	RestorePreAdditiveRootMotionVelocity();

	if( !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() )
	{
		// TODO - define max climb speed and acceleration
		CalcVelocity(DeltaTime, ClimbFriction, true, MaxBrakeClimbDeceleration);
	}

	ApplyRootMotionToVelocity(DeltaTime);

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * DeltaTime;
	FHitResult Hit(1.f);

	// TODO - Handle Climb rotation.
	SafeMoveUpdatedComponent(Adjusted, GetClimbRotation(DeltaTime), true, Hit);

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
	SnapToClimbableSurface(DeltaTime);
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

	// Debug::Print(TEXT("ClimbableSurfaceLocation:: ")+ ClimbableSurfaceLocation.ToCompactString(), FColor::Red, 1.0f);
	// Debug::Print(TEXT("ClimbableSurfaceNormal:: ")+ ClimbableSurfaceNormal.ToCompactString(), FColor::Orange, 2.0f);
}

FQuat UClimbForgeMovementComponent::GetClimbRotation(const float DeltaTime) const
{
	const FQuat CurrentQuat = UpdatedComponent->GetComponentQuat();

	// If we want to use the root motion to override the rotation
	if (HasAnimRootMotion() && CurrentRootMotion.HasOverrideVelocity())
	{
		return CurrentQuat;
	}

	// as the target needs to be the climbable surface and not it's normal.
	const FQuat TargetQuat = FRotationMatrix::MakeFromX(-1.0f*ClimbableSurfaceNormal).ToQuat();

	return FMath::QInterpTo(CurrentQuat,TargetQuat,DeltaTime,5.0f);	
}

inline void UClimbForgeMovementComponent::SnapToClimbableSurface(const float DeltaTime) const
{
	const FVector CurrentLocation = UpdatedComponent->GetComponentLocation();
	const FVector ForwardVector = UpdatedComponent->GetForwardVector();

	// Get the distance between the actor and the climbable surface location and project it onto the plane of the
	// actor's forward vector. This is the vector facing the surface and having length equal to the distance
	// between actor and the surface.
	const FVector ProjectedVector = (ClimbableSurfaceLocation - CurrentLocation).ProjectOnTo(ForwardVector);

	// The Vector (distance and direction) required for us to snap the actor to the climbable surface.
	const FVector SnapVector = -1.0f*ClimbableSurfaceNormal*ProjectedVector.Length();

	UpdatedComponent->MoveComponent(SnapVector*DeltaTime*MaxClimbSpeed, UpdatedComponent->GetComponentQuat(), true);	
}

void UClimbForgeMovementComponent::PlayMontage(const TObjectPtr<UAnimMontage>& MontageToPlay) const
{
	if(MontageToPlay == nullptr) return;
	if (OwnerActorAnimInstance == nullptr) return;

	OwnerActorAnimInstance->Montage_Play(MontageToPlay);
}

void UClimbForgeMovementComponent::MontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == IdleToClimbMontage)
	{
		StartClimbing();
	}
}

#pragma endregion




