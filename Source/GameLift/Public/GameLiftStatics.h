//Copyright 2016 davevillz, https://github.com/davevill



#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameLiftTypes.h"
#include "GameLiftStatics.generated.h"




UCLASS()
class GAMELIFT_API UGameLiftStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:






	UFUNCTION(Category = "GameLift", BlueprintCallable, meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static FString GetGameProperty(UObject* WorldContextObject, const FGameLiftGameSession& GameSession, FName Key);

	/** Formats the host to connect which is ip:port */
	UFUNCTION(Category = "GameLift", BlueprintPure, meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static FString GetConnectHost(UObject* WorldContextObject, const FGameLiftGameSession& GameSession);

	/** Formats the full connect command */
	UFUNCTION(Category = "GameLift", BlueprintPure, meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static FString GetConnectCommand(UObject* WorldContextObject, const FString& Host, const FString& PlayerSessionId);

	/** Connects to a GameLift Server, optionally appending extra options (must contain ?, ie: ?myparam=1?myparam2=0) */
	UFUNCTION(Category = "GameLift", BlueprintCallable, meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static void Connect(UObject* WorldContextObject, const FString& Host, const FString& PlayerSessionId, const FString& ExtraOptions = "");




};
