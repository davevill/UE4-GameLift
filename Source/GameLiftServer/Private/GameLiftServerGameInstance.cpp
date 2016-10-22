//Copyright 2016 davevillz, https://github.com/davevill

#include "GameLiftServerPrivatePCH.h"
#include "GameLiftServerGameInstance.h"
#include "GameLiftServerManager.h"




UGameLiftServerGameInstance::UGameLiftServerGameInstance(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	GameLiftServerManager = ObjectInitializer.CreateDefaultSubobject<UGameLiftServerManager>(this, "GameLiftServerManager");
}

void UGameLiftServerGameInstance::Init()
{
	Super::Init();

	check(GameLiftServerManager);
	GameLiftServerManager->Init();
}

void UGameLiftServerGameInstance::Shutdown()
{
	Super::Shutdown();

	check(GameLiftServerManager);
	GameLiftServerManager->Shutdown();
}

