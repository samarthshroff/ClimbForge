// Fill out your copyright notice in the Description page of Project Settings.

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
