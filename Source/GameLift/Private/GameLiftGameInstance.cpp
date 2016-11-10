//Copyright 2016 davevillz, https://github.com/davevill

#include "GameLiftPrivatePCH.h"
#include "GameLiftGameInstance.h"
#include "GameLiftManager.h"




UGameLiftGameInstance::UGameLiftGameInstance(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	GameLiftManager = ObjectInitializer.CreateDefaultSubobject<UGameLiftManager>(this, "GameLiftManager");
}

void UGameLiftGameInstance::Init()
{
	Super::Init();

	check(GameLiftManager);
	GameLiftManager->Init();
}

void UGameLiftGameInstance::Shutdown()
{
	Super::Shutdown();

	check(GameLiftManager);
	GameLiftManager->Shutdown();
}

