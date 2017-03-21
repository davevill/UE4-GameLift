//Copyright 2016 davevillz, https://github.com/davevill

#pragma once

#include "ModuleManager.h"

#define _GLIBCXX_FULLY_DYNAMIC_STRING 0 


class FGameLiftModule : public IModuleInterface
{

	static void* LibraryHandle;

public:


	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

protected:
	bool LoadDependency(const FString& Dir, const FString& Name, void*& Handle);
	void FreeDependency(void*& Handle);

};

DECLARE_LOG_CATEGORY_EXTERN(GameLiftLog, Log, All);


