//Copyright 2016 davevillz, https://github.com/davevill

#pragma once

#include "Engine/GameInstance.h"
#include "GameLiftGameInstance.generated.h"


/** This class is optional, the ideal way to do this is to create, initialize and shutdown the GameLiftManager object within your own game instance */
UCLASS()
class GAMELIFT_API UGameLiftGameInstance : public UGameInstance
{
	GENERATED_UCLASS_BODY()
	UPROPERTY()
	class UGameLiftManager* GameLiftManager;

public:

	virtual void Init() override;
	virtual void Shutdown() override;

};
