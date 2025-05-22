// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ClimbForgeMovementComponent.generated.h"

UENUM(BlueprintType)
enum ECustomMovementMode : int
{
	MOVE_Climbing UMETA(DisplayName="Climb Mode"),
	MOVE_CUSTOM_MAX		UMETA(Hidden),
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLIMBFORGE_API UClimbForgeMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

private:
#pragma region ClimbCoreVariables
	TArray<FHitResult> ClimbableSurfacesHits;
#pragma endregion
	
#pragma region ClimbBPVariables
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Character Movement: Climb", meta=(AllowPrivateAccess=true))
	float CollisionCapsuleRadius = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Character Movement: Climb", meta=(AllowPrivateAccess=true))
	float CollisionCapsuleHalfHeight = 72.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Character Movement: Climb", meta=(AllowPrivateAccess=true))
	TEnumAsByte<ECollisionChannel> ClimbableSurfaceTraceChannel;

	FCollisionQueryParams ClimbQueryParams;
#pragma endregion

public:
#pragma region ClimbCore
	void ToggleClimbing(const bool bEnableClimb);
	bool IsClimbing() const;
#pragma endregion

protected:
	void BeginPlay() override;
	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
private:
#pragma region ClimbTraces
	// Use the Capsule shape with SweepMultiByChannel to check for any climbable surfaces from the ClimbableSurfaceTraceChannel 
	TArray<FHitResult> CapsuleSweepTraceByChannel(const FVector& Start, const FVector& End, const bool bShowDebugShape, const bool bShowPersistent);

	// Use the LineTraceSingleByChannel to check for any climbable surface from the ClimbableSurfaceTraceChannel which is
	// at the given start and end, usually the eye height, as character can be in front of a ledge which would come as a
	// hit from the capsule sweep but is not a legit climbable surface.
	FHitResult LineTraceByChannel(const FVector& Start, const FVector& End, const bool bShowDebugShape, const bool bShowPersistent);
#pragma endregion

#pragma region ClimbCore
	// Trace for all climbable surfaces.
	bool TraceClimbableSurfaces();

	// Trace from the eye height and see if the ray collides with an object.
	// This helps us decide whether the character can climb.
	FHitResult TraceFromEyeHeight(const float TraceDistance, const float TraceStartOffset = 0.0f);

	bool CanStartClimbing();
	void StartClimbing();
	void StopClimbing();
#pragma endregion
};
