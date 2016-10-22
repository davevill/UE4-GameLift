//Copyright 2016 davevillz, https://github.com/davevill



#pragma once

#include "Object.h"
#include "Tickable.h"
#include "GameliftServerManager.generated.h"



/* Must be created from within a GameInstance object otherwise it wont initialize */
UCLASS(Config=Game)
class GAMELIFTSERVER_API UGameLiftServerManager : public UObject, public FTickableGameObject
{
	GENERATED_BODY()


	bool bInitialized;


	class FGameLiftServerCallbacks* Callbacks;


	UPROPERTY()
	class UGameInstance* GameInstance;


public:

	UGameLiftServerManager();


	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return bInitialized; }
	virtual TStatId GetStatId() const override { return UObject::GetStatID(); }


	virtual class UWorld* GetWorld() const override;


	/** Must be called within the GameInstance::Init method */
	virtual void Init();

	/** Must be called within the GameInstance::Shutdown method */
	virtual void Shutdown();


	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get GameLiftServer", WorldContext = "WorldContextObject"), Category = "GameLift")
	static UGameLiftServerManager* Get(UObject* WorldContextObject);

};
