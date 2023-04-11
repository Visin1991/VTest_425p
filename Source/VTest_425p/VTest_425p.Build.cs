// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VTest_425p : ModuleRules
{
	public VTest_425p(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay","UMG" });
	}
}
