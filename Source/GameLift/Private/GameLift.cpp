//Copyright 2016 davevillz, https://github.com/davevill


#include "GameLiftPrivatePCH.h"
#include "Core.h"
#include "ModuleManager.h"
#include "IPluginManager.h"


#define LOCTEXT_NAMESPACE "FGameLiftModule"

void* FGameLiftModule::LibraryHandle = nullptr;

void FGameLiftModule::StartupModule()
{
#if PLATFORM_WINDOWS
	FString BaseDir = IPluginManager::Get().FindPlugin("GameLift")->GetBaseDir();
	const FString SDKDir = FPaths::Combine(*BaseDir, TEXT("ThirdParty"), TEXT("gamelift-server-sdk"), TEXT("lib"));
	const FString LibName = TEXT("aws-cpp-sdk-gamelift-server");
	const FString LibDir = FPaths::Combine(*SDKDir, TEXT("Win64"));

	if (!LoadDependency(LibDir, LibName, LibraryHandle))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT(LOCTEXT_NAMESPACE, "Failed to load aws-cpp-sdk-gamelift-server library. Plug-in will not be functional."));
		FreeDependency(LibraryHandle);
	}
#endif
}

void FGameLiftModule::ShutdownModule()
{
	FreeDependency(LibraryHandle);
}

bool FGameLiftModule::LoadDependency(const FString& Dir, const FString& Name, void*& Handle)
{
	FString Lib = Name + TEXT(".") + FPlatformProcess::GetModuleExtension();
	FString Path = Dir.IsEmpty() ? *Lib : FPaths::Combine(*Dir, *Lib);

	Handle = FPlatformProcess::GetDllHandle(*Path);

	if (Handle == nullptr)
	{
		return false;
	}

	return true;
}

void FGameLiftModule::FreeDependency(void*& Handle)
{
#if !PLATFORM_LINUX
	if (Handle != nullptr)
	{
		FPlatformProcess::FreeDllHandle(Handle);
		Handle = nullptr;
	}
#endif
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGameLiftModule, GameLift)
DEFINE_LOG_CATEGORY(GameLiftLog);