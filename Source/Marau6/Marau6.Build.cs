// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Marau6 : ModuleRules
{
	public Marau6(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule", "NavigationSystem", // Authoritative bot navigation and arena verification.
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate", "SlateCore", "HTTP", "Json", "JsonUtilities"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "BlueprintGraph", "AssetTools" });
        }

		PublicIncludePaths.AddRange(new string[] {
			"Marau6",
			"Marau6/Variant_Horror",
			"Marau6/Variant_Horror/UI",
			"Marau6/Variant_Shooter",
			"Marau6/Variant_Shooter/AI",
			"Marau6/Variant_Shooter/UI",
			"Marau6/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "HTTP", "Json", "JsonUtilities", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
