// Copyright (c) Samarth Shroff. All Rights Reserved.
// This work is protected under applicable copyright laws in perpetuity.
// Licensed under the CC BY-NC-ND 4.0 License. See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "ClimbingDirection.generated.h"

UENUM(BlueprintType)
enum class EClimbingDirection : uint8
{
	Idle UMETA(DisplayName="Idle Or None"),
	Up UMETA(DisplayName="Up"),
	Down UMETA(DisplayName="Down"),
	Left UMETA(DisplayName="Left"),
	Right UMETA(DisplayName="Right")	
};
