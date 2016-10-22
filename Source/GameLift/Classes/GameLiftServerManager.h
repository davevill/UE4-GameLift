//Copyright 2016 davevillz, https://github.com/davevill



#pragma once

#include "Object.h"
#include "Tickable.h"
#include "GameLiftTypes.h"
#include "GameliftServerManager.generated.h"








/* Must be created from within a GameInstance object otherwise it wont initialize */
UCLASS(Config=Game)
class GAMELIFT_API UGameLiftServerManager : public UObject, public FTickableGameObject
{
	GENERATED_BODY()


	bool bInitialized;
	bool bGameSessionActive;

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

	/** Called when the GameLift service receives request to start a new game session, Loading a map game mode and other settings
	  * should happend here. When the server is ready for players to connect, ActivateGameSession should be called. 
	  * 
	  * The default implementations loads the map defined in the "map" property, with max players and game mode */
	virtual void OnStartGameSession(const FGameLiftGameSession& GameSession);

	virtual void OnProcessTerminate();

	virtual bool OnHealthCheck();


	/** Global getter */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get GameLiftServer", WorldContext = "WorldContextObject"), Category = "GameLift")
	static UGameLiftServerManager* Get(UObject* WorldContextObject);


	/** This should be called within the GameMode::PreLogin method passing both Options and ErrorMessage.
	  * it functions in the same way, if ErrorMessage is set, it means player was rejected or/and if it returns false.*/
	UFUNCTION(BlueprintCallable, Category="GameLift")
	bool AcceptPlayerSession(const FString& Options, FString& ErrorMessage);

	/** Activates the game session */
	UFUNCTION(BlueprintCallable, Category="GameLift")
	bool ActivateGameSession();

	/** Termintes the game session, ProcessEnding should be called if the server needs to be restarted, 
	  * otherwise it will inmediatly be available to host new game sessions */
	UFUNCTION(BlueprintCallable, Category="GameLift")
	void TerminateGameSession();

	/** Notifies GameLift Service that the server is imediatetly ending/exiting, this calls QuitGame internally */
	UFUNCTION(BlueprintCallable, Category="GameLift")
	void ProcessEnding();

};
