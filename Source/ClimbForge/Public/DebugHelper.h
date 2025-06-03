// Copyright (c) Samarth Shroff. All Rights Reserved.
// This work is protected under applicable copyright laws in perpetuity.
// Licensed under the CC BY-NC-ND 4.0 License. See LICENSE file for details.
#pragma once

namespace Debug
{
	static void Print(const FString& Message, const FColor Color = FColor::MakeRandomColor(), const int32 InKey = -1)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(InKey, 6.0f, Color, Message);
		}

		UE_LOG(LogTemp, Display, TEXT("%s"), *Message);		
	}
}
