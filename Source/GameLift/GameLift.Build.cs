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
		LoadServerSDK(Target);
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

	public bool LoadServerSDK(TargetInfo Target)
    {
		bool is64bit = (Target.Platform == UnrealTargetPlatform.Win64);
        bool isLibrarySupported = false;
        string Prefix = Path.Combine(ThirdPartyPath, "aws-gamelift");
 
        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            isLibrarySupported = true;
 
            PublicAdditionalLibraries.Add(Path.Combine(Prefix, "bin", "aws-cpp-sdk-gamelift-server.lib"));

            string ExtraLibPrefix = Path.Combine(Prefix, "gamelift-server-sdk", "3rdParty", "libs", "release");


            PublicAdditionalLibraries.Add(Path.Combine(ExtraLibPrefix, "boost_date_time.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(ExtraLibPrefix, "boost_random.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(ExtraLibPrefix, "boost_system.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(ExtraLibPrefix, "sioclient.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(ExtraLibPrefix, "protobuf.lib"));
        }
 
        if (isLibrarySupported)
        {
            PrivateIncludePaths.Add(Path.Combine(Prefix, "gamelift-server-sdk", "include"));

        }

		//this is so we can macro out code intended for the dedicated server
		Definitions.Add(string.Format("WITH_GAMELIFT_SERVER={0}", isLibrarySupported ? 1 : 0 ) );
 
        return isLibrarySupported;
    }
}
