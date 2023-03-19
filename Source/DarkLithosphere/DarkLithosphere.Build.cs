// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DarkLithosphere : ModuleRules
{
	public DarkLithosphere(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		//bUseRTTI = true;
		CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", 
															"HeadMountedDisplay", "UnrealSandboxToolkit", "UnrealSandboxTerrain", 
															"Json", "JsonUtilities", "AIModule", "UMG", "ALSV4_CPP", "NavigationSystem", "GameplayTasks", "HTTP" });
	}
}
