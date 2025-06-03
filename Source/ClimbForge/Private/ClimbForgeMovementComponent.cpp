// Copyright (c) Samarth Shroff. All Rights Reserved.
// This work is protected under applicable copyright laws in perpetuity.
// Licensed under the CC BY-NC-ND 4.0 License. See LICENSE file for details.

#include "ClimbForgeMovementComponent.h"

#include "ClimbForgeCharacter.h"
#include "ClimbingDirection.h"
#include "CustomMovementMode.h"
#include "DebugHelper.h"
#include "KismetTraceUtils.h"
#include "MotionWarpingComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"

#pragma region Overridden Functions

void UClimbForgeMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	ClimbQueryParams.AddIgnoredActor(GetOwner());

	OwnerColliderCapsuleHalfHeight = GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	
	OwnerActorAnimInstance = GetCharacterOwner()->GetMesh()->GetAnimInstance();
	if (OwnerActorAnimInstance != nullptr)
	{
		//OwnerActorAnimInstance->OnMontageEnded.AddDynamic(this, &UClimbForgeMovementComponent::MontageEnded);
		OwnerActorAnimInstance->OnMontageBlendingOut.AddDynamic(this, &UClimbForgeMovementComponent::MontageEnded);		
	}
}

void UClimbForgeMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TraceClimbableSurfaces();

	if (bMoveToTargetAfterClimb)
	{
		const FVector ToTarget = ClimbToLedgeTargetLocation - UpdatedComponent->GetComponentLocation();
		const float Distance = ToTarget.Size2D();
		const FVector DesiredVelocity = ToTarget.GetSafeNormal2D() * 230.0f; //230.0f from the walk/run blendspace. walk anim is played at 230.0f
		Velocity = DesiredVelocity;
		Acceleration = Velocity;// Not correct physics but faking it to set the bShouldMove to true.
		Acceleration.Z = 0.0f;

		if (Distance < 5.0f)
		{
			bMoveToTargetAfterClimb = false;
			Acceleration = FVector::ZeroVector;
			ClimbToLedgeTargetLocation = FVector::ZeroVector;
			LedgeSurfaceSlopeDegrees = 0.0f;
			StopMovementImmediately();		
		}
	}
}

void UClimbForgeMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	if (IsClimbing())
	{
		bOrientRotationToMovement = false;
		// Half of 96.0f that is in AClimbForgeCharacter constructor.
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(OwnerColliderCapsuleHalfHeight*0.5f);
		OnEnterClimbingMode.ExecuteIfBound();
	}

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == MOVE_Climbing)
	{
		bOrientRotationToMovement = true;		
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(OwnerColliderCapsuleHalfHeight);

		// Set Rotation to Stand
		const FRotator StandRotation = FRotator(0.0f, UpdatedComponent->GetComponentRotation().Yaw, 0.0f);
		UpdatedComponent->SetRelativeRotation(StandRotation);
		
		StopMovementImmediately();
		OnExitClimbingMode.ExecuteIfBound();
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

FVector UClimbForgeMovementComponent::ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const
{
	bool bIsRootMotionAnimation = IsFalling() && OwnerActorAnimInstance != nullptr && OwnerActorAnimInstance->IsAnyMontagePlaying();

	if (bIsRootMotionAnimation)
	{
		return RootMotionVelocity;
	}
	return Super::ConstrainAnimRootMotionVelocity(RootMotionVelocity, CurrentVelocity);	
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
			OutCapsuleTraceHitResult, FLinearColor::Blue, FLinearColor::Red, 5.0f);
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
			PlayMontage(IdleToClimbMontage);
		}
		else
		if (CanStartClimbingDown())
		{
			PlayMontage(ClimbDownFromLegdeMontage);
		}
		else
		{
			TryStartVaulting();
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
	//if (!TraceClimbableSurfaces()) return false;

	// Check if it is a climbable surface by checking it's slope in degrees.
	for (FHitResult& Hit : ClimbableSurfacesHits)
	{
		// This is in theory the surface (to climb) normal projected onto a horizontal plane as
		// GetSafeNormal2D uses on X and Y components.
		const FVector HorizontalProjectedNormal = Hit.Normal.GetSafeNormal2D();

		// The slope or angle between the character forward and the hit normal. This determines if the surface is climbable.
		const float HorizontalDot = FVector::DotProduct(UpdatedComponent->GetForwardVector(), -HorizontalProjectedNormal);
		// This is theta = acos(a.b/|a|x|b|) as both the vectors are normalized or unit vector their magnitudes will be 1 thus 
		// theta = acos(a.b)
		const float AngleInDegrees = FMath::RadiansToDegrees(FMath::Acos(HorizontalDot));

		// Detect if the surface to climb is a ceiling or not. If a ceiling then don't climb.
		const float Steepness = FVector::DotProduct(Hit.Normal, HorizontalProjectedNormal);
		// IF nearly zero means that the normal of this surface is parallel to upvector so it can be a ceiling or a floor
		const bool bIsCeilingOrFloor = FMath::IsNearlyZero(Steepness);

		// Calculate the length of the trace to use for checking if there is a surface at eye height. This check is for the surface that give
		// a valid hit from the ClimbableSurfacesHits but are like small ledges that are not climbable.
		constexpr float BaseLength = 80.0f;
		const float SteepnessMultiplier = 1.0f + (1.0f - Steepness) * 5.0f;
	
		FHitResult SurfaceAtEyeHeightTraceResult = TraceFromEyeHeight(BaseLength * SteepnessMultiplier);
		
		if (AngleInDegrees < MinimumClimbableAngleInDegrees && !bIsCeilingOrFloor && SurfaceAtEyeHeightTraceResult.bBlockingHit)
		{
			return true;
		}
	}

	
	//if (!TraceFromEyeHeight(100.0f).bBlockingHit) return false;
	
	// const float SurfaceSteepness = FVector::DotProduct(ClimbableSurfaceNormal, UpdatedComponent->GetForwardVector());
	// const float TraceDistance = 100.0f;
	// const float SteepnessMultiplier = 1.0f+(1.0f-SurfaceSteepness)*5.0f;	
	// if (!TraceFromEyeHeight(TraceDistance*SteepnessMultiplier).bBlockingHit) return false;

	return false;
}

bool UClimbForgeMovementComponent::CanStartClimbingDown()
{
	if(IsFalling()) return false;

	 const FVector ComponentForward = UpdatedComponent->GetForwardVector();
	 const FVector DownVector = -1.0f * UpdatedComponent->GetUpVector();
	
	 const FVector WalkableSurfaceTraceStart = UpdatedComponent->GetComponentLocation() + ComponentForward * ClimbDownWalkableSurfaceTraceOffset;
	 const FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart + DownVector * 100.f;
	
	 FHitResult WalkableSurfaceHit = LineTraceByChannel(WalkableSurfaceTraceStart,WalkableSurfaceTraceEnd);
	
	 const FVector LedgeTraceStart = WalkableSurfaceHit.TraceStart + ComponentForward * ClimbDownLedgeTraceOffset;
	 const FVector LedgeTraceEnd = LedgeTraceStart + DownVector * 200.f;
	
	 FHitResult LedgeTraceHit = LineTraceByChannel(LedgeTraceStart,LedgeTraceEnd);
	
	 if(WalkableSurfaceHit.bBlockingHit && !LedgeTraceHit.bBlockingHit)
	 {
	 	return true;
	 }
	
	return false;
}

bool UClimbForgeMovementComponent::ShouldStopClimbing()
{
	if (ClimbableSurfacesHits.IsEmpty()) return true;

	// Calculate the Dot Product between the Surface Normal and the Player's or world's Up Vector (assuming both are same in this case)..
	// The dot product tells us how aligned two vectors are:
	//   -   1.0: Vectors are perfectly aligned (pointing in the same direction).
	//   -   0.0: Vectors are perpendicular (90 degrees apart).
	//   -  -1.0: Vectors are perfectly opposite (180 degrees apart).
	float DotProduct = FVector::DotProduct(ClimbableSurfaceNormal, FVector::UpVector);

	// Define Thresholds for Non-Climbable Surfaces.
	// These values determine what is considered a "floor" or a "ceiling".
	// You will likely need to fine-tune these based on your game's specific needs and level design.

	// Threshold for a non-climbable floor:
	// If the normal points mostly UP (aligned with PlayerUpVector), it's a floor.
	// A dot product close to 1.0 indicates a floor.
	const float FloorDotProductThreshold = 0.8f; // Example: Normal is within ~37 degrees of pure up

	// Threshold for a non-climbable ceiling:
	// If the normal points mostly DOWN (opposite to PlayerUpVector), it's a ceiling.
	// A dot product close to -1.0 indicates a ceiling.
	const float CeilingDotProductThreshold = -0.985f; // Example: Normal is within ~37 degrees of pure down

	// Determine if the surface is climbable.
	bool bIsClimbableSurface = true; // Assume climbable by default

	if (DotProduct < CeilingDotProductThreshold || DotProduct > FloorDotProductThreshold)
	{
	    // The surface normal is pointing predominantly upwards. This is a floor.
	    bIsClimbableSurface = false;
	    //UE_LOG(LogTemp, Log, TEXT("Surface is a non-climbable Floor or Ceiling. DotProduct: %f"), DotProduct);
	}
	else
	{
	    // The surface normal is mostly horizontal relative to the player's up vector.
	    // This means it's a wall or a climbable slope.
	    bIsClimbableSurface = true;
	   //UE_LOG(LogTemp, Log, TEXT("Surface is a Climbable Wall/Slope. DotProduct: %f"), DotProduct);
	}
	return !bIsClimbableSurface;
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

FVector UClimbForgeMovementComponent::GetUnrotatedClimbingVelocity() const
{
	return UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), Velocity);
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

FHitResult UClimbForgeMovementComponent::TraceFromEyeHeight(const float TraceDistance, const float TraceStartOffset, const bool bShowDebugShape, const bool bShowPersistent)
{
	const FVector EyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + TraceStartOffset);
	const FVector Start = UpdatedComponent->GetComponentLocation() + EyeHeightOffset;
	const FVector End = Start + (UpdatedComponent->GetForwardVector() * TraceDistance);
	return LineTraceByChannel(Start, End, bShowDebugShape, bShowPersistent);
}

bool UClimbForgeMovementComponent::HasReachedTheFloor()
{
	// As we want to check for floor hit we need the down vector
	const FVector DownVector = -1.0f*UpdatedComponent->GetUpVector();
	// The constant value decides how close to the floor the actor needs to be for the transition to initiate.
	const FVector StartOffset = DownVector * 35.0f;
	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector End = Start + DownVector;

	TArray<FHitResult> PossibleFloorHits = CapsuleSweepTraceByChannel(Start, End);

	if (PossibleFloorHits.IsEmpty()) return false;

	for (const FHitResult& Hit : PossibleFloorHits)
	{
		// The hit is a legit floor only when the dot product of the impact normal and the up vector is equal
		// to cos(1). a and b are parallel when dot(a,b) = a.Size()*b.Size()*cos(1). As the vectors are unit vectors
		// here dot(a,b) = cos(1) which is what parallel function checks against.

		// Collision with floor will be hit even when actor is about to climb up from floor but then the Z of the velocity will be
		// either positive (jumping) or 0 (walking, running or other ground locomotion). it is negative only when actor is climbing down.
		const bool bIsLegitFloor = FVector::Parallel(Hit.ImpactNormal, FVector::UpVector)  &&
			GetUnrotatedClimbingVelocity().Z < -10.0f;
		
		if (bIsLegitFloor)
		{
			return true;
		}
	}
	
	return false;
}

bool UClimbForgeMovementComponent::HasReachedTheLedge()
{
	if (OwnerActorAnimInstance == nullptr) return false;
	if (OwnerActorAnimInstance->Montage_IsPlaying(ClimbToTopMontage)) return true;
	
	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	const float TraceDistance = Capsule->GetUnscaledCapsuleRadius() * 2.5f;
	const FHitResult LedgeHit = TraceFromEyeHeight(TraceDistance, 20.0f);

	if (!LedgeHit.bBlockingHit)
	{
		const FVector WalkableSurfaceStart = LedgeHit.TraceEnd + UpdatedComponent->GetUpVector()*OwnerColliderCapsuleHalfHeight;
		const FVector WalkableSurfaceEnd = WalkableSurfaceStart + UpdatedComponent->GetUpVector()*-2.0f*OwnerColliderCapsuleHalfHeight;
		const FHitResult WalkableSurfaceHit = LineTraceByChannel(WalkableSurfaceStart, WalkableSurfaceEnd);

		if (WalkableSurfaceHit.bBlockingHit && WalkableSurfaceHit.Normal.Z >= GetWalkableFloorZ())
		{
			// Do a capsule sweep that mimics tha character and see if it has hits. If it has hits then character cannot climb up the ledge.
			// This is (CompLoc.X, WalkableSurfaceStart.Y, WalkableSurfaceStart.Z) == right above the character.			
			const FVector CapsuleStart = FVector(UpdatedComponent->GetComponentLocation().X, WalkableSurfaceStart.Y, WalkableSurfaceStart.Z);
			FCollisionShape CapsuleCollision = FCollisionShape::MakeCapsule(ClimbCollisionCapsuleRadius, OwnerColliderCapsuleHalfHeight);
			FHitResult CapsuleHit;
			const bool bCapsuleHit = GetWorld()->SweepSingleByChannel(CapsuleHit, CapsuleStart, WalkableSurfaceStart, FQuat::Identity,
				ClimbableSurfaceTraceChannel, CapsuleCollision, ClimbQueryParams);
			
			// DrawDebugCapsuleTraceSingle(GetWorld(), CapsuleStart, WalkableSurfaceStart, ClimbCollisionCapsuleRadius, 
			// OwnerColliderCapsuleHalfHeight, EDrawDebugTrace::ForOneFrame, bCapsuleHit, CapsuleHit, FLinearColor::Red, FLinearColor::Green, 25.0f);

			if (!bCapsuleHit)
			{
				ClimbToLedgeTargetLocation = WalkableSurfaceHit.Location;
				// Check the slope of the ledge. If it is not flat then we have to give the Target location
				// to the montage via motion warp so that by the end of the animation montage the character is almost at the
				// target location. If it is not at the exact place then the logic in tick will handle the rest by giving manual velocity and
				// the system playing the walk animation INSTEAD of teleporting and glitching.
				const float Dot = FVector::DotProduct(WalkableSurfaceHit.Normal.GetSafeNormal(), UpdatedComponent->GetUpVector());
				LedgeSurfaceSlopeDegrees = FMath::RadiansToDegrees( FMath::Acos(Dot));

				if (!FMath::IsNearlyZero(LedgeSurfaceSlopeDegrees))
				{
					SetMotionWarpTarget("LedgeWarpOffset", ClimbToLedgeTargetLocation);
					bUsedMotionWarpForLedgeClimb = true;
				}
				if (GetUnrotatedClimbingVelocity().Z > 10.0f)
				{
					return true;
				}
			}
		}
	} 

	return false;	
}

void UClimbForgeMovementComponent::TryStartVaulting()
{
	FVector VaultStartPosition = FVector::ZeroVector;
	FVector VaultLandPosition = FVector::ZeroVector;
	if (CanStartVaulting(VaultStartPosition,  VaultLandPosition))
	{
		//Start Vaulting - motion warp and play montage;
		SetMotionWarpTarget("VaultStart", VaultStartPosition);
		SetMotionWarpTarget("VaultLand", VaultLandPosition);
		StartClimbing();
		PlayMontage(VaultingMontage);
	}
}

bool UClimbForgeMovementComponent::CanStartVaulting(FVector& VaultStartPosition, FVector& VaultLandPosition)
{
	if (IsFalling()) return false;

	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector ForwardVector = UpdatedComponent->GetForwardVector();
	const FVector UpVector = UpdatedComponent->GetUpVector();
	const FVector DownVector = -1.0f * UpVector;

	float HighestZClearance = 0.0f;
	VaultStartPosition = VaultLandPosition = FVector::ZeroVector;

	// Distance between subsequent landing traces
	constexpr float LandingTraceInterval = 100.0f;
	// Start traces this far above character's origin
	constexpr float TraceHeightAboveChar = 100.0f;
	// How far down the traces go
	constexpr float VerticalTraceDepth = 100.0f;
	const float VerticalTraceDepthSquaredHalf = (VerticalTraceDepth*VerticalTraceDepth)*0.5f;
	// How many landing traces to perform after the initial vault obstacle trace
	//constexpr int32 TotalLandingTraces = 5;

	// 1. Find the Vault Obstacle (Vault Start Position)
	const float SpeedRatio = Velocity.Size() / GetMaxSpeed();
	const float InitialTraceDistance = FMath::Lerp(MinimumVaultTraceDistance, MaximumVaultTraceDistance, SpeedRatio);

	const FVector ObstacleTraceStart = ComponentLocation + (UpVector * TraceHeightAboveChar) + (ForwardVector * InitialTraceDistance);
	const FVector ObstacleTraceEnd = ObstacleTraceStart + (DownVector * VerticalTraceDepth);

	const FHitResult ObstacleHit = LineTraceByChannel(ObstacleTraceStart, ObstacleTraceEnd);

	if (!ObstacleHit.bBlockingHit)
	{
		// No obstacle found to vault over immediately in front.
		return false;
	}
	if ( FVector::DistSquared(ObstacleHit.Location, ObstacleHit.TraceStart) < VerticalTraceDepthSquaredHalf )
	{
		// If the impact point is closer to the trace start then
		// the impact is probably a climbable surface instead of a vaulting surface.
		return false;
	}

	VaultStartPosition = ObstacleHit.Location;
	HighestZClearance = FMath::Max(HighestZClearance, ObstacleHit.Location.Z);

	FVector LandingTraceStart = ObstacleHit.Location + (UpVector * 20.0f); // raise slightly above surface
	// Max vault obstacle length. If the obstacle is longer than this then the vault montage ends on the obstacle
	// else it ends on the ground.
	float MaxVaultLength = 300.0f;

	FVector ObstacleEdgeLocation;
	FHitResult ObstacleEdgeDetectionHit;
	bool bObstacleEdgeFound = false;

	for (float Distance = 50.0f; Distance <= MaxVaultLength; Distance += 50.0f)
	{
		FVector Start = LandingTraceStart + ForwardVector * Distance;
		FVector End = Start + (DownVector * VerticalTraceDepth);
		
		ObstacleEdgeDetectionHit = LineTraceByChannel(Start, End);		
		if (!ObstacleEdgeDetectionHit.bBlockingHit)
		{
			// No ledge beneath — we've reached the end of the ledge
			ObstacleEdgeLocation = Start;
			bObstacleEdgeFound = true;
			break;
		}
	}
	
	if (bObstacleEdgeFound)
	{
		// End of obstacle + padding
		// Find the Ground Z or else vault montage will end mid air.
		const FVector GroundZTraceStart = ObstacleEdgeLocation + ForwardVector * 30.0f;
		const FVector GroundZTraceEnd = GroundZTraceStart + (DownVector * VerticalTraceDepth*5.0f);
		const FHitResult GroundHit = LineTraceByChannel(GroundZTraceStart, GroundZTraceEnd);
		
		if (GroundHit.bBlockingHit)
		{
			VaultLandPosition = GroundHit.Location;
		}
	}
	else
	{
		VaultLandPosition = ObstacleEdgeDetectionHit.Location; // default vault forward
	}

	if (VaultStartPosition != FVector::ZeroVector && VaultLandPosition != FVector::ZeroVector)
	{
		return true;
	}
	
	return false;
}

void UClimbForgeMovementComponent::PhysClimbing(const float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	// TODO - Process all climbable surfaces info
	
	ProcessClimbableSurfaces();
	// TODO - Check to see if climbing needs to stop
	if (ShouldStopClimbing() || HasReachedTheFloor())
	{
		StopClimbing();
	}
	
	RestorePreAdditiveRootMotionVelocity();

	if(!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() )
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

	if (HasReachedTheLedge())
	{
		// Reset the character rotation leaving only the Yaw unaffected. This improves the motion when the character is climbing steep surfaces.
		const FRotator StandRotation = FRotator(0, UpdatedComponent->GetComponentRotation().Yaw, 0);
		UpdatedComponent->SetRelativeRotation(StandRotation);
		//SetMotionWarpTarget("WalkToTargetAfterClimb", WalkToTargetAfterClimb);
		PlayMontage(ClimbToTopMontage);
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

	const FVector SweepStart = UpdatedComponent->GetComponentLocation();
	const FCollisionShape SphereShape = FCollisionShape::MakeSphere(5.0f);

	//Debug::Print(TEXT("ClimbableSurfacesHits:: ")+FString::FromInt(ClimbableSurfacesHits.Num()));
	
	for (FHitResult SurfaceHits : ClimbableSurfacesHits)
	{
		// Sometimes with overlapping surfaces we can get slightly incorrect normals.
		// The sweep is already present in the collision done by overlap thus making the normal(s) point towards the center.
		//  This makes the character rotate incorrectly while climbing.
		// So doing a sphere sweep to each hit we have from the character location would give us correct straight normals.
		const FVector DirectionVector = (SurfaceHits.ImpactPoint - SweepStart).GetSafeNormal();
		const FVector End = SweepStart + DirectionVector * 100.0f;// some arbitrary length.

		FHitResult SphereHit;
		const bool bHit = GetWorld()->SweepSingleByChannel(SphereHit, SweepStart, End, FQuat::Identity, ClimbableSurfaceTraceChannel,
			SphereShape, ClimbQueryParams);

		DrawDebugSphereTraceSingle(GetWorld(), SweepStart, End, 5.0f,EDrawDebugTrace::ForOneFrame, bHit, SphereHit, FColor::Blue, FColor::Red, 25.0f);
		
		
		ClimbableSurfaceLocation += SphereHit.ImpactPoint;
		ClimbableSurfaceNormal += SphereHit.ImpactNormal;
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
	// Do not try to snap while a montage (mainly a climb dash montage) is playing.
	// this let's the character move to a neighboring wall.
	if (OwnerActorAnimInstance->IsAnyMontagePlaying() &&
		(OwnerActorAnimInstance->GetCurrentActiveMontage() == ClimbDashLeftMontage ||
		OwnerActorAnimInstance->GetCurrentActiveMontage() == ClimbDashRightMontage ||
		OwnerActorAnimInstance->GetCurrentActiveMontage() == ClimbDashUpMontage ||
		OwnerActorAnimInstance->GetCurrentActiveMontage() == ClimbDashDownMontage)) return;
	
	const FVector CurrentLocation = UpdatedComponent->GetComponentLocation();
	const FVector ForwardVector = UpdatedComponent->GetForwardVector();

	// Get the distance between the actor and the climbable surface location and project it onto the plane of the
	// actor's forward vector. This is the vector facing the surface and having length equal to the distance
	// between actor and the surface.
	const FVector ProjectedVector = (ClimbableSurfaceLocation - CurrentLocation).ProjectOnTo(ForwardVector);

	// The Vector (distance and direction) required for us to snap the actor to the climbable surface.
	const FVector SnapVector = -1.0f*ClimbableSurfaceNormal*(ProjectedVector.Length()-45.0f);

	const float SnapSpeed = 5.0f * ((Velocity.Length() / MaxClimbSpeed) + 1);
	UpdatedComponent->MoveComponent(SnapVector * SnapSpeed * DeltaTime, UpdatedComponent->GetComponentQuat(), true);
}

void UClimbForgeMovementComponent::PlayMontage(const TObjectPtr<UAnimMontage>& MontageToPlay) const
{
	if(MontageToPlay == nullptr) return;
	if (OwnerActorAnimInstance == nullptr) return;
	if (OwnerActorAnimInstance->IsAnyMontagePlaying()) return;

	OwnerActorAnimInstance->Montage_Play(MontageToPlay);
}

void UClimbForgeMovementComponent::MontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == IdleToClimbMontage || Montage == ClimbDownFromLegdeMontage)
	{
		StartClimbing();
		StopMovementImmediately();
	}
	else
	if (Montage == ClimbToTopMontage)
	{
		if (bUsedMotionWarpForLedgeClimb)
		{
			bUsedMotionWarpForLedgeClimb = false;
			if (const AClimbForgeCharacter* Owner = Cast<AClimbForgeCharacter>(CharacterOwner))
			{		
				Owner->GetMotionWarpingComponent()->RemoveWarpTarget("LedgeWarpOffset");					
			}
		}

		// Check if the target location is in front or behind the character's current location.
		// Proceed with the logic only if the target location is in front.
		FVector CurrentLocation = UpdatedComponent->GetComponentLocation();
		const FVector ForwardVector = UpdatedComponent->GetForwardVector();		
		// Get the direction from character to target location.
		const FVector ToTarget = (ClimbToLedgeTargetLocation - CurrentLocation).GetSafeNormal();
		const float DotValue = FVector::DotProduct(ForwardVector, ToTarget);

		// The target is in front.
		if (DotValue > 0.0f)
		{
			// Adjust the character Z here as in the Tick logic we consider only X and Y components
			// as Ground Speed in anim instance is 2D.
			const float ZOffset = CurrentLocation.Z - GetActorFeetLocation().Z;
			CurrentLocation.Z = ZOffset + ClimbToLedgeTargetLocation.Z;
			UpdatedComponent->SetWorldLocation(CurrentLocation);
			bMoveToTargetAfterClimb = true;
		}
		SetMovementMode(MOVE_Walking);	
	}
	else
	if (Montage == VaultingMontage)
	{
		SetMovementMode(MOVE_Walking);	
	}
	else
	if (Montage == ClimbDashLeftMontage || Montage == ClimbDashRightMontage)
	{
		FVector CurrentLocation = UpdatedComponent->GetComponentLocation();
		if (CurrentLocation.Z != CharacterLocationBeforeDashMontage.Z)
		{
			CurrentLocation.Z = CharacterLocationBeforeDashMontage.Z;
			UpdatedComponent->SetWorldLocation(CurrentLocation);
		}
	}
}

void UClimbForgeMovementComponent::SetMotionWarpTarget(const FName& InWarpTargetName, const FVector& InTargetLocation)
{
	if (const AClimbForgeCharacter* Owner = Cast<AClimbForgeCharacter>(CharacterOwner))
	{		
		Owner->GetMotionWarpingComponent()->AddOrUpdateWarpTargetFromLocation(InWarpTargetName, InTargetLocation);
	}	
}

void UClimbForgeMovementComponent::RequestClimbDash()
{	
	const FVector UnrotatedLastInputVector = UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), GetLastInputVector());
		
	const float VerticalAxisDotResult = FVector::DotProduct(UnrotatedLastInputVector.GetSafeNormal(), UpdatedComponent->GetUpVector());
	
	const FVector UnrotatedRightVector = UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), UpdatedComponent->GetRightVector());
	const float HorizontalAxisDotResult = FVector::DotProduct(UnrotatedLastInputVector.GetSafeNormal(), UnrotatedRightVector);
	
	if (VerticalAxisDotResult > 0.9f)
	{
		TryPerformClimbDash(EClimbingDirection::Up);
	}
	else
	if (VerticalAxisDotResult < -0.9f)
	{
		TryPerformClimbDash(EClimbingDirection::Down);
	}
	else
	if (HorizontalAxisDotResult > 0.9f)
	{
		TryPerformClimbDash(EClimbingDirection::Right);
	}
	else
	if (HorizontalAxisDotResult < -0.9f)
	{
		TryPerformClimbDash(EClimbingDirection::Left);
	}
	else
	{
		Debug::Print(TEXT("Invalid direction for dash"), FColor::Red, 1);		
	}
}

void UClimbForgeMovementComponent::TryPerformClimbDash(const EClimbingDirection ClimbingDirection)
{
	FVector DashHitPoint = FVector::ZeroVector;
	
	if (CanStartClimbDash(ClimbingDirection, DashHitPoint))
	{
		SetMotionWarpTarget(FName("HopHitPoint"), DashHitPoint);
		switch (ClimbingDirection)
		{
			case EClimbingDirection::Up:
			{
				PlayMontage(ClimbDashUpMontage);
			}
			break;

			case EClimbingDirection::Down:
			{
				PlayMontage(ClimbDashDownMontage);
			}
			break;

			case EClimbingDirection::Left:
			{
				PlayMontage(ClimbDashLeftMontage);
			}
			break;

			case EClimbingDirection::Right:
			{				
				PlayMontage(ClimbDashRightMontage);
			}
			break;
		
		default: ;
		}
	}
}

bool UClimbForgeMovementComponent::CanStartClimbDash(const EClimbingDirection ClimbingDirection, FVector& OutDashHitPoint)
{
	FHitResult DashHit;
	FHitResult EdgeHit;
		
	switch (ClimbingDirection)
	{
		case EClimbingDirection::Up:
		{
			DashHit = TraceFromEyeHeight(ClimbDashTraceLength, ClimbDashEyeHeightTraceOffset);
			EdgeHit = TraceFromEyeHeight(ClimbDashTraceLength, ClimbDashEdgeTraceOffset);
		}
		break;

		case EClimbingDirection::Down:
		{
			DashHit = TraceFromEyeHeight(ClimbDashTraceLength, -2.0f*ClimbDashEdgeTraceOffset);
			if (DashHit.bBlockingHit)
			{
				OutDashHitPoint = DashHit.Location;
				return true;
			}			
		}
		break;

		case EClimbingDirection::Left:
		{		
			DashHit = TraceFromEyeHeight(ClimbDashTraceLength, ClimbDashEyeHeightTraceOffset);
			const FVector EyeLevelVector = UpdatedComponent->GetComponentLocation() + (UpdatedComponent->GetUpVector() * CharacterOwner->BaseEyeHeight + ClimbDashEyeHeightTraceOffset);
			const FVector Start = EyeLevelVector + (-1.0f*UpdatedComponent->GetRightVector()*ClimbDashEdgeTraceOffset);				
			const FVector End = Start + (UpdatedComponent->GetForwardVector() * ClimbDashTraceLength);
			EdgeHit = LineTraceByChannel(Start, End);
		}
		break;

		case EClimbingDirection::Right:
		{
			DashHit = TraceFromEyeHeight(ClimbDashTraceLength, ClimbDashEyeHeightTraceOffset);
			const FVector EyeLevelVector = UpdatedComponent->GetComponentLocation() + (UpdatedComponent->GetUpVector() * CharacterOwner->BaseEyeHeight + ClimbDashEyeHeightTraceOffset);
			const FVector Start = EyeLevelVector + (UpdatedComponent->GetRightVector()*ClimbDashEdgeTraceOffset);				
			const FVector End = Start + (UpdatedComponent->GetForwardVector() * ClimbDashTraceLength);
			EdgeHit = LineTraceByChannel(Start, End);
		}
		break;
		
		default: ;
	}

	if (DashHit.bBlockingHit && EdgeHit.bBlockingHit)
	{
		if (ClimbingDirection == EClimbingDirection::Left || ClimbingDirection == EClimbingDirection::Right)
		{
			CharacterLocationBeforeDashMontage = UpdatedComponent->GetComponentLocation();
			OutDashHitPoint = EdgeHit.Location;
			// Root motion is at the bottom of character and the hit point is at the top of the character capsule.
			OutDashHitPoint.Z = UpdatedComponent->GetComponentLocation().Z - (2.0f*CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
		}
		else
		{
			OutDashHitPoint = DashHit.Location;	
		}		
		
		return true;
	}
	return false;	
}

#pragma endregion




