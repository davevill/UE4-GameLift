//Copyright 2016 davevillz, https://github.com/davevill

#include "GameLiftServerPrivatePCH.h"
#include "GameLiftServerManager.h"


#define _HAS_EXCEPTIONS 0
#pragma warning( push )
#pragma warning( disable : 4530)
// Your function
#include "aws/gamelift/server/GameLiftServerAPI.h"

#pragma warning( pop ) 



UGameLiftServerManager::UGameLiftServerManager()
{
	bInitialized = false;
	Callbacks = nullptr;
}

void UGameLiftServerManager::Tick(float DeltaTime)
{
	ensure(bInitialized);
}



class FGameLiftServerCallbacks
{
	TWeakObjectPtr<UGameLiftServerManager> Manager;

public:

	FGameLiftServerCallbacks(UGameLiftServerManager* Owner) :
		Manager(Owner)
	{

	}

	void OnStartGameSession(Aws::GameLift::Server::Model::GameSession GameSession)
	{
		const std::vector<Aws::GameLift::Server::Model::GameProperty>& Properties = GameSession.GetGameProperties();

		// game-specific tasks when starting a new game session, such as loading map
		Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::ActivateGameSession();
	}

	void OnProcessTerminate()
	{
		Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::ProcessEnding();
	}

	bool OnHealthCheck()
	{
		return true;
	}

};


void UGameLiftServerManager::Init()
{
	GameInstance = Cast<UGameInstance>(GetOuter());

	Callbacks = new FGameLiftServerCallbacks(this);

	if (GameInstance && GameInstance->IsDedicatedServerInstance())
	{
		Aws::GameLift::Server::InitSDKOutcome InitOutcome = Aws::GameLift::Server::InitSDK();

		Aws::GameLift::Server::ProcessParameters ProcessReadyParameter = Aws::GameLift::Server::ProcessParameters(
			std::bind(&FGameLiftServerCallbacks::OnStartGameSession, Callbacks, std::placeholders::_1),
			std::bind(&FGameLiftServerCallbacks::OnProcessTerminate, Callbacks),
			std::bind(&FGameLiftServerCallbacks::OnHealthCheck, Callbacks),
			7777,
			Aws::GameLift::Server::LogParameters()
		);

		Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::ProcessReady(ProcessReadyParameter);
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

	if (Callbacks)
	{
		delete Callbacks;	
		Callbacks = nullptr;
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


