// Copyright (c) Samarth Shroff. All Rights Reserved.
// This work is protected under applicable copyright laws in perpetuity.
// Licensed under the CC-BY-4.0 License. See LICENSE file for details.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ClimbForgeCharacter.generated.h"

class UClimbForgeMovementComponent;
class UMotionWarpingComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AClimbForgeCharacter : public ACharacter
{
	GENERATED_BODY()

private:
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

#pragma region Input
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> ClimbingMappingContext;
	
	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	/** Climb Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ClimbAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ClimbMoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ClimbHopAction;
	
#pragma endregion

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClimbForgeMovementComponent> ClimbForgeMovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;
	
public:
	AClimbForgeCharacter(const FObjectInitializer& ObjectInitializer);
	
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE UClimbForgeMovementComponent* GetClimbForgeMovementComponent() const { return ClimbForgeMovementComponent; }
	FORCEINLINE UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }
	
protected:
	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
private:
	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for climb input */
	void ClimbStarted(const FInputActionValue& Value);

	void HandleClimbingMovement(const FInputActionValue& Value);
	void HandleGroundMovement(const FInputActionValue& Value);


	void AddInputMappingContext(const UInputMappingContext* InContext, int32 InPriority);
	void RemoveInputMappingContext(const UInputMappingContext* InContext);
};



