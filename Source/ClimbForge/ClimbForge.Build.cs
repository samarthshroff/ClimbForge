// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ClimbForge : ModuleRules
{
	public ClimbForge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
