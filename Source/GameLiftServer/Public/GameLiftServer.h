//Copyright 2016 davevillz, https://github.com/davevill

#pragma once

#include "ModuleManager.h"

class FGameLiftServerModule : public IModuleInterface
{
private:

public:


	virtual void StartupModule() override;
	virtual void ShutdownModule() override;



};

DECLARE_LOG_CATEGORY_EXTERN(GameLiftServerLog, Log, All);
