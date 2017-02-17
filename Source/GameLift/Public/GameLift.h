//Copyright 2016 davevillz, https://github.com/davevill

#pragma once

#include "ModuleManager.h"

#define _GLIBCXX_FULLY_DYNAMIC_STRING 0 


class FGameLiftModule : public IModuleInterface
{
public:


	virtual void StartupModule() override;
	virtual void ShutdownModule() override;



};

DECLARE_LOG_CATEGORY_EXTERN(GameLiftLog, Log, All);


