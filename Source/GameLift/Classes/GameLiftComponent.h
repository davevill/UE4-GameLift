//Copyright 2016 davevillz, https://github.com/davevill



#pragma once

#include "Components/ActorComponent.h"
#include "GameLiftTypes.h"
#include "GameliftComponent.generated.h"




UENUM(BlueprintType)
enum class EGameLiftRegion : uint8
{
	USEast1,
	USWest2,
	AsiaPacific1, //ap-northeast-1
	EUWest1,
};


USTRUCT(BlueprintType)
struct FGameLiftFleet
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite)
	FName Id;

	/** Is this an alias? */
	UPROPERTY(BlueprintReadWrite)
	bool bAlias;

	UPROPERTY(BlueprintReadWrite)
	EGameLiftRegion Region;

};





DECLARE_DYNAMIC_DELEGATE_TwoParams(FGameLiftOnCreateGameSessionDelegate, bool, bSucceed, const FGameLiftGameSession&, GameSession);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FGameLiftOnCreatePlayerSessionsDelegate, bool, bSucceed, const TArray<FGameLiftPlayerSession>&, PlayerSessions);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FGameLiftOnSearchGameSessionsDelegate, bool, bSucceed, const TArray<FGameLiftGameSession>&, GameSessions);

/** This component exposes functions and events to comunicate to the GameLift Service 
  * Is intended to be used in an actor that bundles UI creation and event binding */
UCLASS(ClassGroup=(GameLift), meta=(BlueprintSpawnableComponent), Config=Game)
class GAMELIFT_API UGameLiftComponent : public UActorComponent
{
	GENERATED_BODY()

	// we dont expose any direct aws stuff
	class TSharedPtr<class FGameLiftPrivate> Pvt;


	UPROPERTY(Config)
	FString AccessKeyId;

	UPROPERTY(Config)
	FString SecretAccessKey;

	UPROPERTY(Config)
	bool bCognitoEnabled;

public:



	UPROPERTY(BlueprintReadWrite, Category="GameLift")
	EGameLiftRegion Region;




	UGameLiftComponent();


	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	/** Creates a game session  */
	UFUNCTION(BlueprintCallable, Category="GameLift")
	void CreateGameSession(int32 MaxPlayers, TArray<FGameLiftProperty> GameProperties, FGameLiftFleet Fleet, const FGameLiftOnCreateGameSessionDelegate& Callback);


	/** Creates player sessions, can be used for a single one */
	UFUNCTION(BlueprintCallable, Category="GameLift")
	void CreatePlayerSessions(const FString& GameSessionId, const TArray<FString>& PlayerIds, const FGameLiftOnCreatePlayerSessionsDelegate& Callback);

	/** Search for game sessions */
	UFUNCTION(BlueprintCallable, Category="GameLift")
	void SearchGameSessions(const FString& FilterExpression, const FString& SortExpression, FGameLiftFleet Fleet, const FGameLiftOnSearchGameSessionsDelegate& Callback);



};
