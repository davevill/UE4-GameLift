//Copyright 2016 davevillz, https://github.com/davevill


#include "GameLiftPrivatePCH.h"
#include <aws/core/Aws.h>

#define LOCTEXT_NAMESPACE "FGameLiftModule"




void FGameLiftModule::StartupModule()
{
	Aws::SDKOptions Options;
	Aws::InitAPI(Options);

	Aws::ShutdownAPI(Options);
}

void FGameLiftModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGameLiftModule, GameLift)
DEFINE_LOG_CATEGORY(GameLiftLog);