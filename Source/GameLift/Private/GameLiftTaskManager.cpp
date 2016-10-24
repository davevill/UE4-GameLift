//Copyright 2016 davevillz, https://github.com/davevill

#include "GameLiftPrivatePCH.h"
#include "GameLiftTaskManager.h"



void FGameLiftTaskManager::RegisterTask(TSharedPtr<IGameLiftTaskWork> TaskWorkPtr)
{
	FGameLiftTaskPtr Task = MakeShareable(new FAsyncTask<FGameLiftTask>(TaskWorkPtr));
	Task->StartBackgroundTask();

	Tasks.Push(Task);
}

void FGameLiftTaskManager::RegisterTask(IGameLiftTaskWork* TaskWork)
{
	RegisterTask(MakeShareable(TaskWork));
}

void FGameLiftTaskManager::Tick(class UObject* ObjectContext)
{
	for (int32 i = 0; i < Tasks.Num(); i++)
	{
		FGameLiftTaskPtr Task = Tasks[i];

		if (Task->IsDone())
		{
			ensure(ObjectContext);
			if (ObjectContext)
			{
				Task->GetTask().Work->Publish(ObjectContext);
			}

			Tasks.RemoveAt(i, 1, false);
			break;
		}
	}
}

void FGameLiftTaskManager::AbandonAllTasks()
{
	for (int32 i = 0; i < Tasks.Num(); i++)
	{
		Tasks[i]->EnsureCompletion(true);
	}

	Tasks.Empty();
}