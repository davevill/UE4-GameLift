//Copyright 2016 davevillz, https://github.com/davevill

#include "GameLiftPrivatePCH.h"
#include "GameLiftStatics.h"




FString UGameLiftStatics::GetGameProperty(UObject* WorldContextObject, const FGameLiftGameSession& GameSession, FName Key)
{
	for (int32 i = 0; i < GameSession.Properties.Num(); i++)
	{
		if (GameSession.Properties[i].Key == Key)
		{
			return GameSession.Properties[i].Value;
			break;
		}
	}

	return FString();
}

FString UGameLiftStatics::GetConnectHost(UObject* WorldContextObject, const FGameLiftGameSession& GameSession)
{
	return FString::Printf(TEXT("%s:%d"), *GameSession.IpAddress, GameSession.Port);
}

FString UGameLiftStatics::GetConnectCommand(UObject* WorldContextObject, const FString& Host, const FString& PlayerSessionId)
{
	return FString::Printf(TEXT("open %s?%s=%s"), *Host, GAMELIFT_URL_PLAYER_SESSION_ID, *PlayerSessionId);
}

void UGameLiftStatics::Connect(UObject* WorldContextObject, const FString& Host, const FString& PlayerSessionId, const FString& ExtraOptions)
{
	WorldContextObject->GetWorld()->GetFirstPlayerController()->ConsoleCommand(GetConnectCommand(WorldContextObject, Host, PlayerSessionId) + ExtraOptions);	
}

