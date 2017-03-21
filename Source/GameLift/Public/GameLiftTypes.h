//Copyright 2016 davevillz, https://github.com/davevill



#pragma once

#include "GameLiftTypes.generated.h"



#define GAMELIFT_PROPERTY_MAP "map"
#define GAMELIFT_PROPERTY_GAMEMODE "gamemode"



USTRUCT(BlueprintType)
struct FGameLiftProperty
{
	GENERATED_USTRUCT_BODY();
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName Key;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Value;


};

USTRUCT(BlueprintType)
struct FGameLiftPlayerSession
{
	GENERATED_USTRUCT_BODY();
public:

	UPROPERTY(BlueprintReadOnly)
	FString PlayerSessionId;

	UPROPERTY(BlueprintReadOnly)
	FString GameSessionId;

	UPROPERTY(BlueprintReadOnly)
	FString PlayerId;

};

UENUM(BlueprintType)
enum class EGameLiftPlayerSessionCreationPolicy : uint8
{
	AcceptAll,
	DenyAll
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

	UPROPERTY(BlueprintReadOnly)
	int32 Players;

	UPROPERTY(BlueprintReadOnly)
	FString IpAddress;

	UPROPERTY(BlueprintReadOnly)
	int32 Port;

	UPROPERTY(BlueprintReadOnly)
	EGameLiftPlayerSessionCreationPolicy PlayerSessionCreationPolicy;

};

UENUM(BlueprintType)
enum class EGameLiftRegion : uint8
{
	USEast1,
	USWest2,
	AsiaPacific1, //ap-northeast-1
	EUWest1,
	EUCentral1,
};


USTRUCT(BlueprintType)
struct FGameLiftFleet
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName Id;

	/** Is this an alias? */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAlias;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EGameLiftRegion Region;

};