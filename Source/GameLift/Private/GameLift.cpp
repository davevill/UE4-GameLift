//Copyright 2016 davevillz, https://github.com/davevill


#include "GameLiftPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FGameLiftModule"
#include <aws/core/Aws.h>



Aws::SDKOptions GAWSOptions;

void FGameLiftModule::StartupModule()
{
	Aws::InitAPI(GAWSOptions);
}

void FGameLiftModule::ShutdownModule()
{
	Aws::ShutdownAPI(GAWSOptions);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGameLiftModule, GameLift)
DEFINE_LOG_CATEGORY(GameLiftLog);