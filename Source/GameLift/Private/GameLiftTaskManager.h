//Copyright 2016 davevillz, https://github.com/davevill



#pragma once

#include "AsyncWork.h"



/** Defines the actual work */
class IGameLiftTaskWork
{
public:

	virtual ~IGameLiftTaskWork() {}

	/** Called after the task is completed on the game thread */
	virtual void Publish(UObject* Context) = 0;

	/* Do the async work here, store the result in a member */
	virtual void DoWork() = 0;

};

/** Task proxy, must pass a valid Work pointer */
class FGameLiftTask : public FNonAbandonableTask
{
public:
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FGameLiftTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork()
	{
		check(Work.IsValid());

		Work->DoWork();
	}

	FGameLiftTask(TSharedPtr<IGameLiftTaskWork> NewWork) : Work(NewWork) {}

	TSharedPtr<IGameLiftTaskWork> Work;

};

typedef TSharedPtr<FAsyncTask<FGameLiftTask>> FGameLiftTaskPtr;










/** Very simple task manager to allow async calls to aws */
class FGameLiftTaskManager
{
protected:

	TArray<FGameLiftTaskPtr> Tasks;

public:


	/** Registers a new task with the provided work */
	void RegisterTask(TSharedPtr<IGameLiftTaskWork> TaskWorkPtr);

	/** Another flavor, taking ownership of the pointer */
	void RegisterTask(IGameLiftTaskWork* TaskWork);

	/** Should be ticked in the game thread, it's best called once a second
	  * The Object context is passed to the task work */
	void Tick(class UObject* ObjectContext);

	/** Abandons all tasks, wont check for completion or publish any result, however 
	  * all tasks will still run until completion */
	void AbandonAllTasks();


	int32 GetTaskNum() const { return Tasks.Num(); }
};

