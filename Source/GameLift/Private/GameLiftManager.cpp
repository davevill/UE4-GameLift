//Copyright 2016 davevillz, https://github.com/davevill

#include "GameLiftPrivatePCH.h"
#include "GameLiftManager.h"
#include "GameLiftStatics.h"
#include "Async.h"


//#define _HAS_EXCEPTIONS 0
#pragma warning( push )
#pragma warning( disable : 4530)
#pragma warning( disable : 4996)


#if WITH_GAMELIFT

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif

#include "aws/gamelift/common/Outcome.h"
#include "aws/gamelift/server/GameLiftServerAPI.h"
#include <aws/gamelift/server/model/GameSession.h>
#include <aws/gamelift/server/model/GameProperty.h>
#include <map>
#include <vector>
#include <string>

#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif


#pragma warning( pop ) 




#include "GameLiftTaskManager.h"


void OnActivateFunctionInternal(Aws::GameLift::Server::Model::GameSession GameSession, void* State)
{
	UGameLiftManager* Manager = (UGameLiftManager*)State;
	check(Manager);

	int NumProperties = 0;
	auto Properties = GameSession.GetGameProperties(NumProperties);

	FGameLiftGameSession UnrealGameSession;

	UnrealGameSession.SessionId = GameSession.GetGameSessionId();
	UnrealGameSession.MaxPlayers = GameSession.GetMaximumPlayerSessionCount();



	for (int32 i = 0; i < NumProperties; i++)
	{
		const Aws::GameLift::Server::Model::GameProperty& Property = Properties[i];

		FGameLiftProperty UnrealProperty;
		UnrealProperty.Key = Property.GetKey();
		UnrealProperty.Value = Property.GetValue();

		UnrealGameSession.Properties.Add(UnrealProperty);
	}

	AsyncTask(ENamedThreads::GameThread, [=]()
	{
		Manager->OnStartGameSession(UnrealGameSession);
	});
}

void OnTerminateFunctionInternal(void* State)
{
	UGameLiftManager* Manager = (UGameLiftManager*)State;
	check(Manager);

	AsyncTask(ENamedThreads::GameThread, [=]()
	{
		Manager->OnProcessTerminate();
	});
}

bool OnHealthCheckInternal(void* State)
{
	return true;
}


#endif //WITH_GAMELIFT





UGameLiftManager::UGameLiftManager()
{
	bInitialized = false;
	bGameSessionActive = false;
}

void UGameLiftManager::Tick(float DeltaTime)
{
}

bool UGameLiftManager::IsTickable() const
{
	return false;
}

void UGameLiftManager::Init()
{
	GameInstance = Cast<UGameInstance>(GetOuter());

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
#if WITH_GAMELIFT
		auto InitOutcome = Aws::GameLift::Server::InitSDK();

		UE_LOG(GameLiftLog, Log, TEXT("Gamelift Server initialized successfully"));
		bInitialized = true;

		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, this, &UGameLiftManager::ProcessReady, 5.f, false);
#endif
	}

	if (bInitialized == false)
	{
		UE_LOG(GameLiftLog, Log, TEXT("Gamelift Server didn't initialized or its disabled"));
	}
}

void UGameLiftManager::ProcessReady()
{
	ensure(bInitialized);
	ensure(bGameSessionActive == false);

	if (bInitialized && bGameSessionActive == false)
	{
#if WITH_GAMELIFT
		const int32 Port = FURL::UrlConfig.DefaultPort;

		TArray<const char*> LogPaths;
		LogPaths.Add(TCHAR_TO_ANSI(*FPaths::GameLogDir()));

		auto ProcessReadyParameter = Aws::GameLift::Server::ProcessParameters(
			OnActivateFunctionInternal, this,
			OnTerminateFunctionInternal, this,
			OnHealthCheckInternal, this,
			Port,
			Aws::GameLift::Server::LogParameters(LogPaths.GetData(), LogPaths.Num())
		);

		UE_LOG(GameLiftLog, Log, TEXT("GameLift::Server::ProcessReady with port %d"), Port);

		auto Outcome = Aws::GameLift::Server::ProcessReady(ProcessReadyParameter);

		if (!Outcome.IsSuccess())
		{
			FString Error = Outcome.GetError().GetErrorMessage();

			UE_LOG(GameLiftLog, Warning, TEXT("GameLift::Server::ProcessReady failed: %s"), *Error);
		}
#endif
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
#if WITH_GAMELIFT
		Aws::GameLift::Server::Destroy();
#endif
	}	
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

	FString Url;

	Url += TEXT("?listen");
	Url += FString::Printf(TEXT("?maxPlayers=%d"), GameSession.MaxPlayers);

	//TODO add an interface to build the open URL
	for (int32 i = 0; i < GameSession.Properties.Num(); i++)
	{
		const FGameLiftProperty& Property = GameSession.Properties[i];

		if (Property.Key == GAMELIFT_PROPERTY_MAP) continue;

		if (Property.Key == NAME_None || Property.Value.IsEmpty()) continue;

		Url += "?";
		Url += Property.Key.ToString();
		Url += "=";
		Url += Property.Value;
	}

	UE_LOG(GameLiftLog, Log, TEXT("GameLift::Server Starting Game Session with URL = %s"), *Url);

	//Clear the player session id map
	PlayerSessions.Empty();

	bGameSessionActive = false;
	UGameplayStatics::OpenLevel(this, *MapName, true, Url);


	ActivateGameSession();
}

bool UGameLiftManager::AcceptPlayerSession(const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	if (bInitialized == false) return true;

	if (bGameSessionActive)
	{
#if WITH_GAMELIFT
		const FString PlayerSessionId = UGameplayStatics::ParseOption(Options, GAMELIFT_URL_PLAYER_SESSION_ID);

		UE_LOG(GameLiftLog, Log, TEXT("GameLift::Server::AcceptPlayerSession with player session id %s"), *PlayerSessionId);

		Aws::GameLift::GenericOutcome ConnectOutcome = Aws::GameLift::Server::AcceptPlayerSession(TCHAR_TO_ANSI(*PlayerSessionId));

		if (!ConnectOutcome.IsSuccess())
		{
			//We dont want to send the gamelift error messages to users, just log it in the backend
			ErrorMessage = TEXT("Internal error");

			const FString LogErrorMessage = ConnectOutcome.GetError().GetErrorMessage();

			UE_LOG(GameLiftLog, Warning, TEXT("GameLift::Server::AcceptPlayerSession failed with error %s"), *LogErrorMessage);
			return false;
		}

		const FString UniqueIdStr = UniqueId.ToString();

		if (PlayerSessions.Find(UniqueIdStr))
		{
			UE_LOG(GameLiftLog, Warning, TEXT("GameLift AcceptPlayerSession UniqueId %s has multiple sessions ids, checking for duplicates"), *UniqueIdStr);
		}

		
		TArray<FString>& SavedSessions = PlayerSessions.Add(UniqueIdStr);

		if (SavedSessions.Find(PlayerSessionId) == INDEX_NONE)
		{
			//Remove all other player sessions, that could left over due to a crash
			for (const FString& SessionId : SavedSessions)
			{
				Aws::GameLift::GenericOutcome ConnectOutcome = Aws::GameLift::Server::RemovePlayerSession(TCHAR_TO_ANSI(*SessionId));
			}

			SavedSessions.Add(PlayerSessionId);
		}
		else
		{
			UE_LOG(GameLiftLog, Warning, TEXT("GameLift AcceptPlayerSession UniqueId %s has a duplicate player session, dropping connection"), *UniqueIdStr);
			return false;
		}

		return true;
#endif
	}

	return false;
}

void UGameLiftManager::RemovePlayerSession(const FUniqueNetIdRepl& UniqueId)
{
	const FString UniqueIdStr = UniqueId.ToString();

	const TArray<FString>* pSavedSessions = PlayerSessions.Find(UniqueId.ToString());

	//if a player joined with an unique id, it gotta leave with the same id
	ensure(pSavedSessions != nullptr);

	if (!pSavedSessions)
	{
		UE_LOG(GameLiftLog, Warning, TEXT("GameLift::Server::RemovePlayerSession UniqueId not found %s"), *UniqueIdStr);
	}
	else
	{
		ensure((*pSavedSessions).Num() == 1);
		for (const FString& SessionId : (*pSavedSessions))
		{
#if WITH_GAMELIFT
			Aws::GameLift::GenericOutcome ConnectOutcome = Aws::GameLift::Server::RemovePlayerSession(TCHAR_TO_ANSI(*SessionId));
			PlayerSessions.Remove(UniqueIdStr);
#endif
		}
	}
}

void UGameLiftManager::ActivateGameSession()
{
#if WITH_GAMELIFT
	ensure(bGameSessionActive == false);
	if (bGameSessionActive) return;

	Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::ActivateGameSession();
	UE_LOG(GameLiftLog, Log, TEXT("GameLift Activating Game Session"));

	bGameSessionActive = Outcome.IsSuccess();
	GameSessionDuration = 0.f;
#endif
}

void UGameLiftManager::OnProcessTerminate()
{
#if WITH_GAMELIFT
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


	ProcessEnding();
#endif
}

void UGameLiftManager::TerminateGameSession()
{
	if (bGameSessionActive && bInitialized)
	{
#if WITH_GAMELIFT
		UE_LOG(GameLiftLog, Log, TEXT("GameLift Terminating Game Session "));
		Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::TerminateGameSession();
		bGameSessionActive = false;


		AGameModeBase* GameMode = UGameplayStatics::GetGameMode(GameInstance);

		ensure(GameMode);

		//Kick everyone before switching to idle map
		if (GameMode)
		{
			for (TActorIterator<APlayerController> Itr(GameInstance->GetWorld()); Itr; ++Itr)
			{
				GameMode->GameSession->KickPlayer(*Itr, FText::FromString(TEXT("Game session ended")));
			}
		}

		ProcessReady();
#endif
	}
}

void UGameLiftManager::ProcessEnding()
{
#if WITH_GAMELIFT
	if (bInitialized)
	{
		Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::ProcessEnding();

		UGameLiftManager::RequestExit();
	}
#endif
}

void UGameLiftManager::UpdatePlayerSessionCreationPolicy(EGameLiftPlayerSessionCreationPolicy Policy)
{
#if WITH_GAMELIFT

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
#endif
}


