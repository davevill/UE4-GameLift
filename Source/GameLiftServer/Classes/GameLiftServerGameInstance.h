//Copyright 2016 davevillz, https://github.com/davevill



#pragma once

#include "Engine/GameInstance.h"
#include "GameliftServerGameInstance.generated.h"




/** This class is optional, the ideal way is to create, initialize and shutdown the GameLiftServerManager object within your own game instance */
UCLASS()
class GAMELIFTSERVER_API UGameLiftServerGameInstance : public UGameInstance
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class UGameLiftServerManager* GameLiftServerManager;

public:

	virtual void Init() override;
	virtual void Shutdown() override;

};
