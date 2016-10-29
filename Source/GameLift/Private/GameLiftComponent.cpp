//Copyright 2016 davevillz, https://github.com/davevill

#include "GameLiftPrivatePCH.h"
#include "GameLiftComponent.h"
#define _HAS_EXCEPTIONS 0
#pragma warning( push )
#pragma warning( disable : 4530)

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
		UGameLiftComponent* Component = Cast<UGameLiftComponent>(Context);
		check(Component);

		Publish(Component);
	}

	virtual void Publish(UGameLiftComponent* Component) = 0;
};







UGameLiftComponent::UGameLiftComponent()
{
	bWantsInitializeComponent = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;

	Pvt = MakeShareable(new FGameLiftPrivate);
	Pvt->TaskManager = MakeShareable(new FGameLiftTaskManager);
}

void UGameLiftComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Pvt->TaskManager->Tick(this);

	if (Pvt->TaskManager->GetTaskNum() == 0)
	{
		PrimaryComponentTick.SetTickFunctionEnable(false);
	}
}

void UGameLiftComponent::InitializeComponent()
{
	Super::InitializeComponent();

	Pvt->Credentials.SetAWSAccessKeyId(TCHAR_TO_ANSI(*AccessKeyId));
	Pvt->Credentials.SetAWSSecretKey(TCHAR_TO_ANSI(*SecretAccessKey));

	PrimaryComponentTick.SetTickFunctionEnable(false);
}

void UGameLiftComponent::UninitializeComponent()
{
	Super::UninitializeComponent();
	Pvt->TaskManager->AbandonAllTasks();
}




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

	void Publish(UGameLiftComponent* Component)
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

void UGameLiftComponent::CreateGameSession(int32 MaxPlayers, TArray<FGameLiftProperty> GameProperties, FGameLiftFleet Fleet, const FGameLiftOnCreateGameSessionDelegate& Callback)
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
	PrimaryComponentTick.SetTickFunctionEnable(true);
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

	void Publish(UGameLiftComponent* Component)
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

void UGameLiftComponent::CreatePlayerSessions(const FString& GameSessionId, const TArray<FString>& PlayerIds, const FGameLiftOnCreatePlayerSessionsDelegate& Callback)
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
	PrimaryComponentTick.SetTickFunctionEnable(true);
}







//CreateGameSession
class FGameLiftSearchGameSessionsTaskWork : public FGameLiftAPITaskWork
{
public:

	Aws::GameLift::Model::SearchGameSessionsRequest Request;
	Aws::GameLift::Model::SearchGameSessionsOutcome Outcome;

	FGameLiftOnSearchGameSessionsDelegate Callback;

	void DoWork()
	{
		Outcome = Client.SearchGameSessions(Request);
	}

	void Publish(UGameLiftComponent* Component)
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

void UGameLiftComponent::SearchGameSessions(const FString& FilterExpression, const FString& SortExpression, FGameLiftFleet Fleet, const FGameLiftOnSearchGameSessionsDelegate& Callback)
{
	FGameLiftSearchGameSessionsTaskWork* TaskWork = new FGameLiftSearchGameSessionsTaskWork();
	TaskWork->Callback = Callback;

	Aws::Client::ClientConfiguration ClientConfiguration = Pvt->SetupClientConfiguration(Region);

	TaskWork->Client = Aws::GameLift::GameLiftClient(Pvt->Credentials, ClientConfiguration);
	TaskWork->Request.SetFilterExpression(TCHAR_TO_ANSI(*FilterExpression));
	TaskWork->Request.SetSortExpression(TCHAR_TO_ANSI(*SortExpression));

	if (Fleet.bAlias)
	{
		TaskWork->Request.SetAliasId(TCHAR_TO_ANSI(*Fleet.Id.ToString()));
	}
	else
	{
		TaskWork->Request.SetFleetId(TCHAR_TO_ANSI(*Fleet.Id.ToString()));
	}

	Pvt->TaskManager->RegisterTask(TaskWork);
	PrimaryComponentTick.SetTickFunctionEnable(true);
}
