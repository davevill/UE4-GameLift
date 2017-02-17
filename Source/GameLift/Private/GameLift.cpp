//Copyright 2016 davevillz, https://github.com/davevill


#include "GameLiftPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FGameLiftModule"



//#define _HAS_EXCEPTIONS 0
#pragma warning( push )
#pragma warning( disable : 4530)
#include <aws/core/Aws.h>

#pragma warning( pop )



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