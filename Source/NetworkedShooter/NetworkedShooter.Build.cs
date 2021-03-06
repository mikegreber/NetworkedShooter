// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NetworkedShooter : ModuleRules
{
	public NetworkedShooter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject",
			"Engine",
			"InputCore",
			"HeadMountedDisplay",
			"OnlineSubsystem", 
			"OnlineSubsystemSteam", 
			"MultiplayerSessions", 
			"UMG",
			"Niagara",
			"GameplayAbilities", 
			"GameplayTags", 
			"GameplayTasks",
			"ReplicationGraph"
		});
		
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "NetCore" });
	}
}
