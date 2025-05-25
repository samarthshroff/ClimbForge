// Copyright (c) Samarth Shroff. All Rights Reserved.
// This work is protected under applicable copyright laws in perpetuity.
// Licensed under the CC-BY-4.0 License. See LICENSE file for details.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ClimbForgeMovementComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLIMBFORGE_API UClimbForgeMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

private:
#pragma region ClimbCoreVariables
	TArray<FHitResult> ClimbableSurfacesHits;

	FCollisionQueryParams ClimbQueryParams;

	FVector ClimbableSurfaceLocation;
	
	FVector ClimbableSurfaceNormal;

	float OwnerColliderCapsuleHalfHeight;

	UPROPERTY()
	TObjectPtr<UAnimInstance> OwnerActorAnimInstance;
	
#pragma endregion
	
#pragma region ClimbBPVariables
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Character Movement: Climb", meta=(AllowPrivateAccess=true))
	float ClimbCollisionCapsuleRadius = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Character Movement: Climb", meta=(AllowPrivateAccess=true))
	float ClimbCollisionCapsuleHalfHeight = 72.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Character Movement: Climb", meta=(AllowPrivateAccess=true))
	TEnumAsByte<ECollisionChannel> ClimbableSurfaceTraceChannel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Character Movement: Climb", meta=(AllowPrivateAccess=true))
	float ClimbFriction = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Character Movement: Climb", meta=(AllowPrivateAccess=true))
	float MaxBrakeClimbDeceleration = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Character Movement: Climb", meta=(AllowPrivateAccess=true))
	float MaxClimbSpeed = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Character Movement: Climb", meta=(AllowPrivateAccess=true))
	float MaxClimbAcceleration = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Character Movement: Climb", meta=(AllowPrivateAccess=true))
	float MinimumClimbableAngleInDegrees = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Character Movement: Climb", meta=(AllowPrivateAccess=true))
	TObjectPtr<UAnimMontage> IdleToClimbMontage;
	
#pragma endregion

public:
#pragma region ClimbCore
	void ToggleClimbing(const bool bEnableClimb);
	bool IsClimbing() const;
	FORCEINLINE FVector GetClimbableSurfaceNormal() const {return ClimbableSurfaceNormal;}
#pragma endregion

protected:
#pragma region Overridden Functions
	void BeginPlay() override;
	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	void PhysCustom(float DeltaTime, int32 Iterations) override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
#pragma endregion

	
private:
#pragma region ClimbTraces
	// Use the Capsule shape with SweepMultiByChannel to check for any climbable surfaces from the ClimbableSurfaceTraceChannel 
	TArray<FHitResult> CapsuleSweepTraceByChannel(const FVector& Start, const FVector& End, const bool bShowDebugShape = false, const bool bShowPersistent = 
	false);

	// Use the LineTraceSingleByChannel to check for any climbable surface from the ClimbableSurfaceTraceChannel which is
	// at the given start and end, usually the eye height, as character can be in front of a ledge which would come as a
	// hit from the capsule sweep but is not a legit climbable surface.
	FHitResult LineTraceByChannel(const FVector& Start, const FVector& End, const bool bShowDebugShape = false, const bool bShowPersistent = false);
#pragma endregion

#pragma region ClimbCore
	// Trace for all climbable surfaces.
	bool TraceClimbableSurfaces();

	// Trace from the eye height and see if the ray collides with an object.
	// This helps us decide whether the character can climb.
	FHitResult TraceFromEyeHeight(const float TraceDistance, const float TraceStartOffset = 0.0f);

	bool CanStartClimbing();
	bool ShouldStopClimbing();
	void StartClimbing();
	void StopClimbing();
	void PhysClimbing(float DeltaTime, int32 Iterations);

	// Get the average location from all the climbable hit results.
	void ProcessClimbableSurfaces();

	// Get the rotation required to rotate the actor to face the climbable surface.
	FQuat GetClimbRotation(float DeltaTime) const;

	// Snap the actor (movement) to the climbable surface and lock onto it.
	void SnapToClimbableSurface(float DeltaTime) const;

	void PlayMontage(const TObjectPtr<UAnimMontage>& MontageToPlay) const;

	UFUNCTION()
	void MontageEnded(UAnimMontage* Montage, bool bInterrupted);

#pragma endregion
};
