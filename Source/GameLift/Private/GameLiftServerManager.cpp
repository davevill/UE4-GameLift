//Copyright 2016 davevillz, https://github.com/davevill

#include "GameLiftPrivatePCH.h"
#include "GameLiftServerManager.h"
#include "GameLiftStatics.h"


#define _HAS_EXCEPTIONS 0
#pragma warning( push )
#pragma warning( disable : 4530)
// Your function
#include "aws/gamelift/server/GameLiftServerAPI.h"

#pragma warning( pop ) 





#define GAMELIFT_EVENT_ON_START_GAME_SESSION 1
#define GAMELIFT_EVENT_ON_PROCESS_TERMINATE 2
#define GAMELIFT_EVENT_ON_HEALTHCHECK 3


class FGameLiftServerCallbacks
{
	TWeakObjectPtr<UGameLiftServerManager> Manager;

	Aws::GameLift::Server::Model::GameSession StoredGameSession;

	TQueue<int32> EventQueue;

public:

	FGameLiftServerCallbacks(UGameLiftServerManager* Owner) :
		Manager(Owner)
	{

	}

	void OnStartGameSession(Aws::GameLift::Server::Model::GameSession GameSession)
	{
		//Store a copy
		StoredGameSession = GameSession;

		EventQueue.Enqueue(GAMELIFT_EVENT_ON_START_GAME_SESSION);
	}

	void OnProcessTerminate()
	{
		EventQueue.Enqueue(GAMELIFT_EVENT_ON_PROCESS_TERMINATE);
	}

	bool OnHealthCheck()
	{
		EventQueue.Enqueue(GAMELIFT_EVENT_ON_HEALTHCHECK);
		//TODO is this a thread? can it get locked until the game code runs its health check?
		return true;
	}


	void PollEvents()
	{
		int32 Event;

		if (EventQueue.Dequeue(Event))
		{
			switch (Event)
			{
			case GAMELIFT_EVENT_ON_START_GAME_SESSION:
				{
					const std::vector<Aws::GameLift::Server::Model::GameProperty>& Properties = StoredGameSession.GetGameProperties();

					check(Manager.IsValid());

					FGameLiftGameSession UnrealGameSession;

					UnrealGameSession.SessionId = StoredGameSession.GetGameSessionId().c_str();
					UnrealGameSession.MaxPlayers = StoredGameSession.GetMaximumPlayerSessionCount();

					for (int32 i = 0; i < Properties.size(); i++)
					{
						const Aws::GameLift::Server::Model::GameProperty& Property = Properties[i];

						FGameLiftProperty UnrealProperty;
						UnrealProperty.Key = Property.GetKey().c_str();
						UnrealProperty.Value = Property.GetValue().c_str();

						UnrealGameSession.Properties.Add(UnrealProperty);
					}

					Manager->OnStartGameSession(UnrealGameSession);
				}
				break;
			case GAMELIFT_EVENT_ON_PROCESS_TERMINATE:
				{
					Manager->OnProcessTerminate();
				}
				break;
			case GAMELIFT_EVENT_ON_HEALTHCHECK:
				{
					//TODO this does nothing right now
					Manager->OnHealthCheck();
				}
				break;
			};


		}

	}

};





UGameLiftServerManager::UGameLiftServerManager()
{
	bInitialized = false;
	Callbacks = nullptr;
	bGameSessionActive = false;
}

void UGameLiftServerManager::Tick(float DeltaTime)
{
	ensure(bInitialized && Callbacks);

	if (bInitialized && Callbacks)
	{
		Callbacks->PollEvents();
	}
}

void UGameLiftServerManager::Init()
{
	GameInstance = Cast<UGameInstance>(GetOuter());

	Callbacks = new FGameLiftServerCallbacks(this);


	const bool bEnabledByCommandLine = FParse::Param(FCommandLine::Get(), TEXT("GAMELIFT"));

	if (bEnabledByCommandLine)
	{
		UE_LOG(GameLiftLog, Log, TEXT("GameLift Server API enabled by command line"));
	}
	else
	{
		if (GameInstance && GameInstance->IsDedicatedServerInstance())
		{
			UE_LOG(GameLiftLog, Warning, TEXT("Running dedicated server but -GAMELIFT command line param is not set"));
		}
	}

	if (GameInstance && GameInstance->IsDedicatedServerInstance() && bEnabledByCommandLine)
	{
		Aws::GameLift::Server::InitSDKOutcome InitOutcome = Aws::GameLift::Server::InitSDK();

		const int32 Port = FURL::UrlConfig.DefaultPort;

		Aws::GameLift::Server::ProcessParameters ProcessReadyParameter = Aws::GameLift::Server::ProcessParameters(
			std::bind(&FGameLiftServerCallbacks::OnStartGameSession, Callbacks, std::placeholders::_1),
			std::bind(&FGameLiftServerCallbacks::OnProcessTerminate, Callbacks),
			std::bind(&FGameLiftServerCallbacks::OnHealthCheck, Callbacks),
			Port,
			Aws::GameLift::Server::LogParameters()
		);

		UE_LOG(GameLiftLog, Log, TEXT("Sending GameLift::Server::ProcessReady with port %d"), Port);

		Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::ProcessReady(ProcessReadyParameter);


		if (!Outcome.IsSuccess())
		{
			FString Error = Outcome.GetError().GetErrorMessage().c_str();

			UE_LOG(GameLiftLog, Warning, TEXT("GameLift::Server::ProcessReady failed: %s"), *Error);
			return;
		}

		UE_LOG(GameLiftLog, Log, TEXT("Gamelift Server initialized successfully"));
		bInitialized = true;
	}

	if (bInitialized == false)
	{
		UE_LOG(GameLiftLog, Log, TEXT("Gamelift Server didn't initialized or its disabled"));
	}
}

void UGameLiftServerManager::Shutdown()
{
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

void UGameLiftServerManager::OnStartGameSession(const FGameLiftGameSession& GameSession)
{
	FString MapName = UGameLiftStatics::GetGameProperty(this, GameSession, GAMELIFT_PROPERTY_MAP);

	//TODO FIX ME use a map whitelist and pretty much everything that is stored in game properties

	FURL Url;

	Url.AddOption(TEXT("listen"));
	Url.AddOption(*FString::Printf(TEXT("maxPlayers=%d"), GameSession.MaxPlayers));


	UGameplayStatics::OpenLevel(this, *MapName, true, Url.ToString());

	bGameSessionActive = false;

	ActivateGameSession();
}

bool UGameLiftServerManager::AcceptPlayerSession(const FString& Options, FString& ErrorMessage)
{
	if (bInitialized == false) return true;

	const FString PlayerSessionId = UGameplayStatics::ParseOption(Options, GAMELIFT_URL_PLAYER_SESSION_ID);

	UE_LOG(GameLiftLog, Log, TEXT("GameLift::Server::AcceptPlayerSession with player session id %s"), *PlayerSessionId);

	Aws::GameLift::GenericOutcome ConnectOutcome = Aws::GameLift::Server::AcceptPlayerSession(TCHAR_TO_ANSI(*PlayerSessionId));

	if (!ConnectOutcome.IsSuccess())
	{
		//We dont want to send the gamelift error messages to users, just log it in the backend
		ErrorMessage = TEXT("Internal error");

		const FString LogErrorMessage = ConnectOutcome.GetError().GetErrorMessage().c_str();

		UE_LOG(GameLiftLog, Warning, TEXT("GameLift::Server::AcceptPlayerSession failed with error %s"), *LogErrorMessage);
		return false;
	}

	return true;
}

bool UGameLiftServerManager::ActivateGameSession()
{
	ensure(bGameSessionActive == false);
	if (bGameSessionActive) return false;

	Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::ActivateGameSession();
	UE_LOG(GameLiftLog, Log, TEXT("GameLift Activating Game Session"));
	return Outcome.IsSuccess();
}

void UGameLiftServerManager::OnProcessTerminate()
{
	UE_LOG(GameLiftLog, Log, TEXT("GameLift OnProcessTerminate, shutting down the server"));
	ProcessEnding();
}

bool UGameLiftServerManager::OnHealthCheck()
{
	UE_LOG(GameLiftLog, Log, TEXT("GameLift OnHealthCheck"));
	return true;
}

void UGameLiftServerManager::TerminateGameSession()
{
	if (bGameSessionActive)
	{
		UE_LOG(GameLiftLog, Log, TEXT("GameLift Terminating Game Session "));
		Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::TerminateGameSession();
		bGameSessionActive = false;
	}
}

void UGameLiftServerManager::ProcessEnding()
{
	Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::ProcessEnding();
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit);
}