//Copyright 2016 davevillz, https://github.com/davevill


#include "GameLiftServerPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FGameLiftServerModule"




void FGameLiftServerModule::StartupModule()
{
}

void FGameLiftServerModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGameLiftServerModule, GameLiftServer)
DEFINE_LOG_CATEGORY(GameLiftServerLog);