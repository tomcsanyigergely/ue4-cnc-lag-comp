// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ReplicationTest : ModuleRules
{
	public ReplicationTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
