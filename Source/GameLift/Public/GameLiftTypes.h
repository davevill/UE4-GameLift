//Copyright 2016 davevillz, https://github.com/davevill



#pragma once

#include "GameliftTypes.generated.h"



#define GAMELIFT_PROPERTY_MAP "map"
#define GAMELIFT_PROPERTY_GAMEMODE "gamemode"



USTRUCT(BlueprintType)
struct FGameLiftProperty
{
	GENERATED_USTRUCT_BODY();
public:

	UPROPERTY(BlueprintReadOnly)
	FName Key;

	UPROPERTY(BlueprintReadOnly)
	FString Value;


};




USTRUCT(BlueprintType)
struct FGameLiftGameSession
{
	GENERATED_USTRUCT_BODY();
public:

	UPROPERTY(BlueprintReadOnly)
	FString SessionId;

	UPROPERTY(BlueprintReadOnly)
	TArray<FGameLiftProperty> Properties;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers;

};