//Copyright 2016 davevillz, https://github.com/davevill

using System.IO;
using UnrealBuildTool;
using System;


public class GameLift : ModuleRules
{
	private string ModulePath
    {
        get { return ModuleDirectory; }
    }
 
    private string ThirdPartyPath
    {
        get { return Path.GetFullPath( Path.Combine( ModulePath, "../../ThirdParty/" ) ); }
    }


	public GameLift(TargetInfo Target)
	{
		
		PublicIncludePaths.AddRange(
			new string[]
			{
				"GameLift/Public"
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[]
			{
				"GameLift/Private"
			}
		);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", "CoreUObject", "Engine", "InputCore"
            }
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
                "Networking",
                "HTTP"
            }
		);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
       


		LoadSDK(Target);
	}

	public bool LoadSDK(TargetInfo Target)
    {
		bool is64bit = (Target.Platform == UnrealTargetPlatform.Win64);
        bool isLibrarySupported = false;
        string Prefix = Path.Combine(ThirdPartyPath, "aws-sdk-cpp");
 
        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            isLibrarySupported = true;
 
            PublicAdditionalLibraries.Add(Path.Combine(Prefix, "aws-cpp-sdk-core", "Release", "aws-cpp-sdk-core.lib")); 
            PublicAdditionalLibraries.Add(Path.Combine(Prefix, "aws-cpp-sdk-gamelift", "Release", "aws-cpp-sdk-gamelift.lib")); 
            PublicAdditionalLibraries.Add("bcrypt.lib"); 
            PublicAdditionalLibraries.Add("Winhttp.lib"); 
        }
 
        if (isLibrarySupported)
        {
            PrivateIncludePaths.Add(Path.Combine(Prefix, "aws-cpp-sdk-core", "include"));
            PrivateIncludePaths.Add(Path.Combine(Prefix, "aws-cpp-sdk-gamelift", "include"));
        }
 
        return isLibrarySupported;
    }
}
