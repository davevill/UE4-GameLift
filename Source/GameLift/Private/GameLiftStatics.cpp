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
