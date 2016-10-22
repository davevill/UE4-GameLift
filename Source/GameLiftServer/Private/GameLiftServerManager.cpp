//Copyright 2016 davevillz, https://github.com/davevill

#include "GameLiftServerPrivatePCH.h"
#include "GameLiftServerManager.h"


#include "aws/gamelift/server/GameLiftServerAPI.h"




UGameLiftServerManager::UGameLiftServerManager()
{
	bInitialized = false;
}

void UGameLiftServerManager::Tick(float DeltaTime)
{
	ensure(bInitialized);
}

void UGameLiftServerManager::Init()
{
	UGameInstance* GameInstance = Cast<UGameInstance>(GetOuter());

	if (GameInstance/* && GameInstance->IsDedicatedServerInstance()*/)
	{
		//Aws::GameLift::Server::InitSDKOutcome InitOutcome = Aws::GameLift::Server::InitSDK();
	}

	//We want to passrhtought since we only fully initialize in dedicated mode
	bInitialized = true;
}

void UGameLiftServerManager::Shutdown()
{
	ensure(bInitialized);
	if (bInitialized)
	{

	}

}

class UWorld* UGameLiftServerManager::GetWorld() const
{
	if (GameInstance)
	{
		return GameInstance->GetWorld();
	}

	return nullptr;
}


UGameLiftServerManager* UGameLiftServerManager::Get(UObject* WorldContextObject)
{
	if (WorldContextObject)
	{
		for (TObjectIterator<UGameLiftServerManager> Itr; Itr; ++Itr)
		{
			if (Itr->GetWorld() == WorldContextObject->GetWorld() && Itr->bInitialized)
			{
				return *Itr;
			}
		}
	}

	return nullptr;
}


