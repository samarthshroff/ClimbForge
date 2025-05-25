// Copyright (c) Samarth Shroff. All Rights Reserved.
// This work is protected under applicable copyright laws in perpetuity.
// Licensed under the CC-BY-4.0 License. See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CharacterAnimInstance.generated.h"

class UClimbForgeMovementComponent;
class AClimbForgeCharacter;
/**
 * 
 */
UCLASS()
class CLIMBFORGE_API UCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

private:
#pragma region Core Variables
	UPROPERTY()
	TObjectPtr<AClimbForgeCharacter> OwnerActor;

	UPROPERTY()
	TObjectPtr<UClimbForgeMovementComponent> OwnerMovementComponent;
#pragma endregion

protected:
#pragma region Essential Movement Data

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "Essential Movement Data", meta=(AllowPrivateAccess=true))
	float GroundSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "Essential Movement Data", meta=(AllowPrivateAccess=true))
	float AirSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "Essential Movement Data", meta=(AllowPrivateAccess=true))
	FVector Velocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "Essential Movement Data", meta=(AllowPrivateAccess=true))
	bool bShouldMove;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "Essential Movement Data", meta=(AllowPrivateAccess=true))
	bool bIsFalling;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "Essential Movement Data", meta=(AllowPrivateAccess=true))
	bool bIsClimbing;

#pragma endregion
	
protected:
#pragma region Overidden Functions
	void NativeInitializeAnimation() override;
	void NativeUpdateAnimation(float DeltaSeconds) override;
#pragma endregion
};
