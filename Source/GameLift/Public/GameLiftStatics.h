//Copyright 2016 davevillz, https://github.com/davevill



#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameLiftTypes.h"
#include "GameliftStatics.generated.h"




UCLASS()
class GAMELIFT_API UGameLiftStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:






	UFUNCTION(Category = "GameLift", BlueprintCallable, meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static FString GetGameProperty(UObject* WorldContextObject, const FGameLiftGameSession& GameSession, FName Key);





};
