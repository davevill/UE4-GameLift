//Copyright 2016 davevillz, https://github.com/davevill

#include "GameLiftPrivatePCH.h"
#include "GameLiftManager.h"
#include "GameLiftStatics.h"


#define _HAS_EXCEPTIONS 0
#pragma warning( push )
#pragma warning( disable : 4530)

#include "aws/gamelift/server/GameLiftServerAPI.h"

#include <aws/core/Aws.h>
#include <aws/gameLift/GameLiftClient.h>
#include <aws/gameLift/model/CreateGameSessionRequest.h>
#include <aws/gameLift/model/CreateGameSessionResult.h>
#include <aws/gameLift/model/DescribeGameSessionsRequest.h>
#include <aws/gameLift/model/DescribeGameSessionsResult.h>
#include <aws/gameLift/model/SearchGameSessionsRequest.h>
#include <aws/gameLift/model/SearchGameSessionsResult.h>
#include <aws/gameLift/model/CreatePlayerSessionsRequest.h>
#include <aws/gameLift/model/CreatePlayerSessionsResult.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/Outcome.h>

#pragma warning( pop ) 

#include "GameLiftTaskManager.h"




#define GAMELIFT_EVENT_ON_START_GAME_SESSION 1
#define GAMELIFT_EVENT_ON_PROCESS_TERMINATE 2


class FGameLiftServerCallbacks
{
	TWeakObjectPtr<UGameLiftManager> Manager;

	Aws::GameLift::Server::Model::GameSession StoredGameSession;

	TQueue<int32> EventQueue;

public:

	FGameLiftServerCallbacks(UGameLiftManager* Owner) :
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
		//Not implemented yet 
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
			};

		}

	}

};



class FGameLiftPrivate
{
public:

	TSharedPtr<Aws::GameLift::GameLiftClient> Client;

	Aws::Auth::AWSCredentials Credentials;

	TSharedPtr<FGameLiftTaskManager> TaskManager;

	Aws::Client::ClientConfiguration SetupClientConfiguration(EGameLiftRegion Region)
	{
		Aws::Client::ClientConfiguration ClientConfiguration;

		const char* RegionStr = Aws::Region::US_EAST_1;

		switch (Region)
		{
		case EGameLiftRegion::USEast1:      RegionStr = Aws::Region::US_EAST_1; break;
		case EGameLiftRegion::USWest2:      RegionStr = Aws::Region::US_WEST_2; break;
		case EGameLiftRegion::AsiaPacific1: RegionStr = Aws::Region::AP_NORTHEAST_1; break;
		case EGameLiftRegion::EUWest1:      RegionStr = Aws::Region::EU_WEST_1; break;
		};

		ClientConfiguration.region = RegionStr;

		return ClientConfiguration;
	}
};



class FGameLiftAPITaskWork : public IGameLiftTaskWork
{
public:

	Aws::GameLift::GameLiftClient Client;

	void Publish(UObject* Context)
	{
		UGameLiftManager* Component = Cast<UGameLiftManager>(Context);
		check(Component);

		Publish(Component);
	}

	virtual void Publish(UGameLiftManager* Component) = 0;
};







UGameLiftManager::UGameLiftManager()
{
	bInitialized = false;
	Callbacks = nullptr;
	bGameSessionActive = false;

	Pvt = MakeShareable(new FGameLiftPrivate);
	Pvt->TaskManager = MakeShareable(new FGameLiftTaskManager);
}

void UGameLiftManager::Tick(float DeltaTime)
{
	if (bInitialized && Callbacks)
	{
		Callbacks->PollEvents();

		if (bGameSessionActive)
		{
			GameSessionDuration += DeltaTime;
		}
	}

	Pvt->TaskManager->Tick(this);
}

bool UGameLiftManager::IsTickable() const
{
	return (Pvt->TaskManager->GetTaskNum() > 0) || bInitialized;
}

void UGameLiftManager::Init()
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

		UE_LOG(GameLiftLog, Log, TEXT("Gamelift Server initialized successfully"));
		bInitialized = true;

		ProcessReady();
	}

	if (bInitialized == false)
	{
		UE_LOG(GameLiftLog, Log, TEXT("Gamelift Server didn't initialized or its disabled"));
	}


	Pvt->Credentials.SetAWSAccessKeyId(TCHAR_TO_ANSI(*AccessKeyId));
	Pvt->Credentials.SetAWSSecretKey(TCHAR_TO_ANSI(*SecretAccessKey));
}

void UGameLiftManager::ProcessReady()
{
	ensure(bInitialized);
	ensure(bGameSessionActive == false);
	if (bInitialized && bGameSessionActive == false)
	{
		const int32 Port = FURL::UrlConfig.DefaultPort;

		std::vector<std::string> LogPaths;
		LogPaths.push_back(TCHAR_TO_ANSI(*FPaths::GameLogDir()));

		Aws::GameLift::Server::ProcessParameters ProcessReadyParameter = Aws::GameLift::Server::ProcessParameters(
			std::bind(&FGameLiftServerCallbacks::OnStartGameSession, Callbacks, std::placeholders::_1),
			std::bind(&FGameLiftServerCallbacks::OnProcessTerminate, Callbacks),
			std::bind(&FGameLiftServerCallbacks::OnHealthCheck, Callbacks),
			Port,
			Aws::GameLift::Server::LogParameters(LogPaths)
		);

		UE_LOG(GameLiftLog, Log, TEXT("GameLift::Server::ProcessReady with port %d"), Port);

		Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::ProcessReady(ProcessReadyParameter);

		if (!Outcome.IsSuccess())
		{
			FString Error = Outcome.GetError().GetErrorMessage().c_str();

			UE_LOG(GameLiftLog, Warning, TEXT("GameLift::Server::ProcessReady failed: %s"), *Error);
		}
	}
}

void UGameLiftManager::RequestExit()
{
	FGenericPlatformMisc::RequestExit(false);
}

void UGameLiftManager::Shutdown()
{
	if (bInitialized)
	{
		Aws::GameLift::Server::Destroy();
	}	

	if (Callbacks)
	{
		delete Callbacks;	
		Callbacks = nullptr;
	}	


	Pvt->TaskManager->AbandonAllTasks();
}

class UWorld* UGameLiftManager::GetWorld() const
{
	if (GameInstance)
	{
		return GameInstance->GetWorld();
	}

	return nullptr;
}

bool UGameLiftManager::IsFleetInstance() const
{
	return bInitialized;
}

UGameLiftManager* UGameLiftManager::Get(UObject* WorldContextObject)
{
	if (WorldContextObject)
	{
		for (TObjectIterator<UGameLiftManager> Itr; Itr; ++Itr)
		{
			if (Itr->GetWorld() == WorldContextObject->GetWorld())
			{
				return *Itr;
			}
		}
	}

	return nullptr;
}

void UGameLiftManager::OnStartGameSession(const FGameLiftGameSession& GameSession)
{
	FString MapName = UGameLiftStatics::GetGameProperty(this, GameSession, GAMELIFT_PROPERTY_MAP);

	//TODO FIX ME use a map whitelist and pretty much everything that is stored in game properties

	FURL Url;

	Url.AddOption(TEXT("listen"));
	Url.AddOption(*FString::Printf(TEXT("maxPlayers=%d"), GameSession.MaxPlayers));

	//Clear the player session id map
	PlayerSessions.Empty();

	//This is a blocking call
	UGameplayStatics::OpenLevel(this, *MapName, true, Url.ToString());


	bGameSessionActive = false;

	ActivateGameSession();
}

bool UGameLiftManager::AcceptPlayerSession(const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	if (bInitialized == false) return true;

	if (bGameSessionActive)
	{
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

		const FString UniqueIdStr = UniqueId.ToString();

		const FString* pPlayerSessionId = PlayerSessions.Find(UniqueIdStr);

		if (pPlayerSessionId)
		{
			UE_LOG(GameLiftLog, Warning, TEXT("GameLift AcceptPlayerSession UniqueId %s is already in the server, droping connection"), *UniqueIdStr);
			return false;
		}
		
		FString& SavedSessionId = PlayerSessions.Add(UniqueIdStr);
		SavedSessionId = PlayerSessionId;

		return true;
	}

	return false;
}

void UGameLiftManager::RemovePlayerSession(const FUniqueNetIdRepl& UniqueId)
{
	const FString UniqueIdStr = UniqueId.ToString();
	const FString* pPlayerSessionId = PlayerSessions.Find(UniqueId.ToString());

	//if a player joined with an unique id, it gotta leave with the same id
	ensure(pPlayerSessionId != nullptr);
	if (!pPlayerSessionId)
	{
		UE_LOG(GameLiftLog, Warning, TEXT("GameLift::Server::RemovePlayerSession UniqueId not found %s"), *UniqueIdStr);
		return;
	}

	Aws::GameLift::GenericOutcome ConnectOutcome = Aws::GameLift::Server::RemovePlayerSession(TCHAR_TO_ANSI(*(*pPlayerSessionId)));
	PlayerSessions.Remove(UniqueIdStr);
}

bool UGameLiftManager::ActivateGameSession()
{
	ensure(bGameSessionActive == false);
	if (bGameSessionActive) return false;

	Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::ActivateGameSession();
	UE_LOG(GameLiftLog, Log, TEXT("GameLift Activating Game Session"));

	bGameSessionActive = Outcome.IsSuccess();
	GameSessionDuration = 0.f;

	return Outcome.IsSuccess();
}

void UGameLiftManager::OnProcessTerminate()
{
	UE_LOG(GameLiftLog, Log, TEXT("GameLift OnProcessTerminate, shutting down the server"));

	Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::ProcessEnding();


	AGameMode* GameMode = GetWorld()->GetAuthGameMode<AGameMode>();

	if (GameMode)
	{
		GameMode->AbortMatch();

		if (GameMode->GameSession)
		{
			const FText KickText = FText::FromString(TEXT("Server is shutting down"));

			for (TActorIterator<APlayerController> Itr(GetWorld()); Itr; ++Itr)
			{
				GameMode->GameSession->KickPlayer(*Itr, KickText);
			}
		}
	}


	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, this, &UGameLiftManager::ProcessEnding, 10.f, false);
}

void UGameLiftManager::TerminateGameSession()
{
	if (bGameSessionActive && bInitialized)
	{
		UE_LOG(GameLiftLog, Log, TEXT("GameLift Terminating Game Session "));
		Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::TerminateGameSession();
		bGameSessionActive = false;
		
		//Loads the default map
		UGameplayStatics::OpenLevel(this, "ServerIdle", true);

		ProcessReady();
	}
}

void UGameLiftManager::ProcessEnding()
{
	if (bInitialized)
	{
		Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::ProcessEnding();

		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, this, &UGameLiftManager::RequestExit, 1.f, false);
	}
}

void UGameLiftManager::UpdatePlayerSessionCreationPolicy(EGameLiftPlayerSessionCreationPolicy Policy)
{
	Aws::GameLift::Server::Model::PlayerSessionCreationPolicy NewPolicy = Aws::GameLift::Server::Model::PlayerSessionCreationPolicy::ACCEPT_ALL;

	switch (Policy)
	{
	case EGameLiftPlayerSessionCreationPolicy::AcceptAll: NewPolicy = Aws::GameLift::Server::Model::PlayerSessionCreationPolicy::ACCEPT_ALL; break;
	case EGameLiftPlayerSessionCreationPolicy::DenyAll: NewPolicy = Aws::GameLift::Server::Model::PlayerSessionCreationPolicy::DENY_ALL; break;
	};


	if (bInitialized)
	{
		Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::UpdatePlayerSessionCreationPolicy(NewPolicy);
	}
}



// Client code



static void AWSGameSessionToUnreal(const Aws::GameLift::Model::GameSession& AWSGameSession, FGameLiftGameSession& GameSession)
{
	GameSession.SessionId = AWSGameSession.GetGameSessionId().c_str();
	GameSession.MaxPlayers = AWSGameSession.GetMaximumPlayerSessionCount();

	const std::vector<Aws::GameLift::Model::GameProperty>& Properties = AWSGameSession.GetGameProperties();

	for (int32 i = 0; i < Properties.size(); i++)
	{
		const Aws::GameLift::Model::GameProperty& Property = Properties[i];

		FGameLiftProperty UnrealProperty;
		UnrealProperty.Key = Property.GetKey().c_str();
		UnrealProperty.Value = Property.GetValue().c_str();

		GameSession.Properties.Add(UnrealProperty);
	}

	GameSession.IpAddress = AWSGameSession.GetIpAddress().c_str();
	GameSession.Port = AWSGameSession.GetPort();

	GameSession.PlayerSessionCreationPolicy = EGameLiftPlayerSessionCreationPolicy::AcceptAll;

	switch (AWSGameSession.GetPlayerSessionCreationPolicy())
	{
	case Aws::GameLift::Model::PlayerSessionCreationPolicy::ACCEPT_ALL: GameSession.PlayerSessionCreationPolicy = EGameLiftPlayerSessionCreationPolicy::AcceptAll; break;
	case Aws::GameLift::Model::PlayerSessionCreationPolicy::DENY_ALL:   GameSession.PlayerSessionCreationPolicy = EGameLiftPlayerSessionCreationPolicy::DenyAll; break;
	}
}



//CreateGameSession
class FGameLiftCreateGameSessionTaskWork : public FGameLiftAPITaskWork
{
public:

	Aws::GameLift::Model::CreateGameSessionRequest Request;
	Aws::GameLift::Model::CreateGameSessionOutcome Outcome;

	FGameLiftOnCreateGameSessionDelegate Callback;

	void DoWork()
	{
		Outcome = Client.CreateGameSession(Request);

		//Now wait for the gamesession to become active
		if (Outcome.IsSuccess())
		{
			Aws::GameLift::Model::DescribeGameSessionsRequest DescribeRequest;

			DescribeRequest.SetGameSessionId(Outcome.GetResult().GetGameSession().GetGameSessionId());

			//wait 30 seconds for the game session to be active
			for (int32 i = 0; i < 30; i++)
			{
				Aws::GameLift::Model::DescribeGameSessionsOutcome ActivateOutcome = Client.DescribeGameSessions(DescribeRequest);

				if (ActivateOutcome.IsSuccess() && ActivateOutcome.GetResult().GetGameSessions().size() > 0)
				{
					if (ActivateOutcome.GetResult().GetGameSessions()[0].GetStatus() == Aws::GameLift::Model::GameSessionStatus::ACTIVE)
					{
						break;
					}
				}

				FPlatformProcess::Sleep(1.f);
			}
		}
	}

	void Publish(UGameLiftManager* Component)
	{
		FGameLiftGameSession GameSession;

		bool bSucceed = Outcome.IsSuccess();

		if (bSucceed)
		{
			Aws::GameLift::Model::CreateGameSessionResult Result = Outcome.GetResult();

			AWSGameSessionToUnreal(Result.GetGameSession(), GameSession);

		}
		else
		{
			UE_LOG(GameLiftLog, Warning, TEXT("GameLift CreateGameSession failed: %s"), *FString(Outcome.GetError().GetMessageW().c_str()));
		}

		Callback.ExecuteIfBound(bSucceed, GameSession);
	};
};

void UGameLiftManager::CreateGameSession(int32 MaxPlayers, TArray<FGameLiftProperty> GameProperties, FGameLiftFleet Fleet, const FGameLiftOnCreateGameSessionDelegate& Callback)
{
	FGameLiftCreateGameSessionTaskWork* TaskWork = new FGameLiftCreateGameSessionTaskWork();
	TaskWork->Callback = Callback;

	Aws::Client::ClientConfiguration ClientConfiguration = Pvt->SetupClientConfiguration(Region);

	TaskWork->Client = Aws::GameLift::GameLiftClient(Pvt->Credentials, ClientConfiguration);

	if (Fleet.bAlias)
	{
		TaskWork->Request.SetAliasId(TCHAR_TO_ANSI(*Fleet.Id.ToString()));
	}
	else
	{
		TaskWork->Request.SetFleetId(TCHAR_TO_ANSI(*Fleet.Id.ToString()));
	}

	TaskWork->Request.SetMaximumPlayerSessionCount(MaxPlayers);

	for (int32 i = 0; i < GameProperties.Num(); i++)
	{
		const FGameLiftProperty& Property = GameProperties[i];

		Aws::GameLift::Model::GameProperty GameLiftProperty;

		GameLiftProperty.SetKey(TCHAR_TO_ANSI(*Property.Key.ToString()));
		GameLiftProperty.SetValue(TCHAR_TO_ANSI(*Property.Value));

		TaskWork->Request.AddGameProperties(GameLiftProperty);
	}

	Pvt->TaskManager->RegisterTask(TaskWork);
}





//CreatePlayerSessions
class FGameLiftCreatePlayerSessionsTaskWork : public FGameLiftAPITaskWork
{
public:

	Aws::GameLift::Model::CreatePlayerSessionsRequest Request;
	Aws::GameLift::Model::CreatePlayerSessionsOutcome Outcome;

	FGameLiftOnCreatePlayerSessionsDelegate Callback;

	void DoWork()
	{
		Outcome = Client.CreatePlayerSessions(Request);
	}

	void Publish(UGameLiftManager* Component)
	{
		TArray<FGameLiftPlayerSession> PlayerSessions;

		bool bSucceed = Outcome.IsSuccess();

		if (bSucceed)
		{
			Aws::GameLift::Model::CreatePlayerSessionsResult Result = Outcome.GetResult();

			const Aws::Vector<Aws::GameLift::Model::PlayerSession>& AWSPlayerSessions = Result.GetPlayerSessions();

			for (int32 i = 0; i < AWSPlayerSessions.size(); i++)
			{
				const Aws::GameLift::Model::PlayerSession& AWSPlayerSession = AWSPlayerSessions[i];

				FGameLiftPlayerSession PlayerSession;
				PlayerSession.PlayerSessionId = AWSPlayerSession.GetPlayerSessionId().c_str();
				PlayerSession.GameSessionId = AWSPlayerSession.GetGameSessionId().c_str();
				PlayerSession.PlayerId = AWSPlayerSession.GetPlayerId().c_str();

				PlayerSessions.Add(PlayerSession);
			}
		}
		else
		{
			UE_LOG(GameLiftLog, Warning, TEXT("GameLift CreatePlayerSessions failed: %s"), *FString(Outcome.GetError().GetMessageW().c_str()));
		}


		Callback.ExecuteIfBound(bSucceed, PlayerSessions);

	};
};

void UGameLiftManager::CreatePlayerSessions(const FString& GameSessionId, const TArray<FString>& PlayerIds, const FGameLiftOnCreatePlayerSessionsDelegate& Callback)
{
	FGameLiftCreatePlayerSessionsTaskWork* TaskWork = new FGameLiftCreatePlayerSessionsTaskWork();
	TaskWork->Callback = Callback;

	Aws::Client::ClientConfiguration ClientConfiguration = Pvt->SetupClientConfiguration(Region);

	TaskWork->Client = Aws::GameLift::GameLiftClient(Pvt->Credentials, ClientConfiguration);

	Aws::Vector<Aws::String> AWSPlayerIds;

	for (int32 i = 0; i < PlayerIds.Num(); i++)
	{
		AWSPlayerIds.push_back(TCHAR_TO_ANSI(*PlayerIds[i]));
	}

	TaskWork->Request.SetGameSessionId(TCHAR_TO_ANSI(*GameSessionId));
	TaskWork->Request.SetPlayerIds(AWSPlayerIds);

	Pvt->TaskManager->RegisterTask(TaskWork);
}







//CreateGameSession
class FGameLiftSearchGameSessionsTaskWork : public FGameLiftAPITaskWork
{
public:

	Aws::GameLift::Model::SearchGameSessionsRequest Request;
	Aws::GameLift::Model::SearchGameSessionsOutcome Outcome;

	bool bOnlyAcceptingPlayers;

	FGameLiftOnSearchGameSessionsDelegate Callback;

	void DoWork()
	{
		Outcome = Client.SearchGameSessions(Request);
	}

	void Publish(UGameLiftManager* Component)
	{
		TArray<FGameLiftGameSession> GameSessions;

		bool bSucceed = Outcome.IsSuccess();

		if (bSucceed)
		{
			Aws::GameLift::Model::SearchGameSessionsResult Result = Outcome.GetResult();

			const Aws::Vector<Aws::GameLift::Model::GameSession>& AWSGameSessions = Result.GetGameSessions();

			for (int32 i = 0; i < AWSGameSessions.size(); i++)
			{
				FGameLiftGameSession GameSession;

				AWSGameSessionToUnreal(AWSGameSessions[i], GameSession);

				if (bOnlyAcceptingPlayers && GameSession.PlayerSessionCreationPolicy != EGameLiftPlayerSessionCreationPolicy::AcceptAll)
				{
					continue;
				}

				GameSessions.Add(GameSession);
			}
		}
		else
		{
			UE_LOG(GameLiftLog, Warning, TEXT("GameLift SearchGameSessions failed: %s"), *FString(Outcome.GetError().GetMessageW().c_str()));
		}

		Callback.ExecuteIfBound(bSucceed, GameSessions);
	};
};

void UGameLiftManager::SearchGameSessions(const FString& FilterExpression, const FString& SortExpression, bool bOnlyAcceptingPlayers, FGameLiftFleet Fleet, const FGameLiftOnSearchGameSessionsDelegate& Callback)
{
	FGameLiftSearchGameSessionsTaskWork* TaskWork = new FGameLiftSearchGameSessionsTaskWork();
	TaskWork->Callback = Callback;

	Aws::Client::ClientConfiguration ClientConfiguration = Pvt->SetupClientConfiguration(Region);

	TaskWork->Client = Aws::GameLift::GameLiftClient(Pvt->Credentials, ClientConfiguration);
	TaskWork->Request.SetFilterExpression(TCHAR_TO_ANSI(*FilterExpression));
	TaskWork->Request.SetSortExpression(TCHAR_TO_ANSI(*SortExpression));
	TaskWork->bOnlyAcceptingPlayers = bOnlyAcceptingPlayers;

	if (Fleet.bAlias)
	{
		TaskWork->Request.SetAliasId(TCHAR_TO_ANSI(*Fleet.Id.ToString()));
	}
	else
	{
		TaskWork->Request.SetFleetId(TCHAR_TO_ANSI(*Fleet.Id.ToString()));
	}

	Pvt->TaskManager->RegisterTask(TaskWork);
}
