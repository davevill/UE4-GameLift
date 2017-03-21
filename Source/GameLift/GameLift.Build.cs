//Copyright 2016 davevillz, https://github.com/davevill

using System.IO;
using UnrealBuildTool;
using System;


public class GameLift : ModuleRules
{
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
                "HTTP",
                "Projects"
            }
		);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);


        string BaseDirectory = System.IO.Path.GetFullPath(System.IO.Path.Combine(ModuleDirectory, "..", ".."));
        string SDKDirectory = System.IO.Path.Combine(BaseDirectory, "ThirdParty", Target.Platform.ToString());

        bool bHasGameLiftSDK = System.IO.Directory.Exists(SDKDirectory);

        if (bHasGameLiftSDK)
        {

            PrivateIncludePaths.Add(Path.Combine(BaseDirectory, "ThirdParty", "include"));

            if (Target.Type == TargetRules.TargetType.Server || true)
            {
                Definitions.Add("WITH_GAMELIFT=1");

                if (Target.Platform == UnrealTargetPlatform.Linux)
                {
                    string SDKLib = System.IO.Path.Combine(SDKDirectory, "libaws-cpp-sdk-gamelift-server.so");

                    PublicLibraryPaths.Add(SDKDirectory);
                    PublicAdditionalLibraries.Add(SDKLib);
                    RuntimeDependencies.Add(new RuntimeDependency(SDKLib));
                }
                else if (Target.Platform == UnrealTargetPlatform.Win64)
                {
                    PublicLibraryPaths.Add(SDKDirectory);
                    PublicAdditionalLibraries.Add(System.IO.Path.Combine(SDKDirectory, "aws-cpp-sdk-gamelift-server.lib"));
                    PublicDelayLoadDLLs.Add("aws-cpp-sdk-gamelift-server.dll");
                    string SDKLibWindows = System.IO.Path.Combine(SDKDirectory, "aws-cpp-sdk-gamelift-server.dll");
                    RuntimeDependencies.Add(new RuntimeDependency(SDKLibWindows));
                }
            }
            else
            {
                Definitions.Add("WITH_GAMELIFT=0");
            }
        }
        else
        {
            Definitions.Add("WITH_GAMELIFT=0");
        }

	}
}
