// Copyright JETBLU, Inc. All Rights Reserved.

#include "AbilityTask_WaitMoveArrived.h"

#include "Lethe/LetheLog.h"
#include "Lethe/Character/LetheCharacterBase.h"
#include "Misc/App.h"

UAbilityTask_WaitMoveArrived::UAbilityTask_WaitMoveArrived(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = true;
}

UAbilityTask_WaitMoveArrived* UAbilityTask_WaitMoveArrived::WaitMoveArrived(UGameplayAbility* OwningAbility, const TArray<ALetheCharacterBase*>& MovingCharacters, const float TimeoutSeconds)
{
	UAbilityTask_WaitMoveArrived* Task = NewAbilityTask<UAbilityTask_WaitMoveArrived>(OwningAbility);
	for (ALetheCharacterBase* MovingCharacter : MovingCharacters)
	{
		if (MovingCharacter)
		{
			Task->PendingCharacters.AddUnique(MovingCharacter);
		}
	}
	Task->TimeoutSeconds = TimeoutSeconds;
	return Task;
}

void UAbilityTask_WaitMoveArrived::Activate()
{
	Super::Activate();

	// 이미 이동을 끝낸 캐릭터(순간이동, 빈 경로)는 신호를 다시 보내지 않으므로 여기서 걸러냅니다.
	PendingCharacters.RemoveAll([](const TWeakObjectPtr<ALetheCharacterBase>& Character)
	{
		return !Character.IsValid() || !Character->IsMoving();
	});

	for (const TWeakObjectPtr<ALetheCharacterBase>& Character : PendingCharacters)
	{
		const FDelegateHandle Handle = Character->OnMoveFinished.AddUObject(this, &ThisClass::HandleMoveFinished, Character);
		BoundHandles.Emplace(Character, Handle);
	}

	CompleteIfAllArrived();
}

void UAbilityTask_WaitMoveArrived::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (bCompleted)
	{
		return;
	}

	// 이동 중 파괴된 캐릭터는 더 이상 신호를 보내지 않으므로 도착한 것으로 처리합니다.
	PendingCharacters.RemoveAll([](const TWeakObjectPtr<ALetheCharacterBase>& Character)
	{
		return !Character.IsValid();
	});
	CompleteIfAllArrived();
	if (bCompleted || TimeoutSeconds <= 0.f)
	{
		return;
	}

	// DeltaTime은 TimeDilation이 적용된 값이므로, 실제 프레임 시간을 따로 누적합니다.
	ElapsedRealSeconds += static_cast<float>(FApp::GetDeltaTime());
	if (ElapsedRealSeconds >= TimeoutSeconds)
	{
		LETHE_LOG(LogAbility, Warning, "%.1f초 동안 도착 신호가 오지 않아 이동 대기를 강제로 종료합니다. 남은 캐릭터 수: %d", TimeoutSeconds, PendingCharacters.Num());
		Complete(true);
	}
}

void UAbilityTask_WaitMoveArrived::OnDestroy(bool bInOwnerFinished)
{
	UnbindAll();
	Super::OnDestroy(bInOwnerFinished);
}

void UAbilityTask_WaitMoveArrived::HandleMoveFinished(TWeakObjectPtr<ALetheCharacterBase> MovedCharacter)
{
	PendingCharacters.Remove(MovedCharacter);
	CompleteIfAllArrived();
}

void UAbilityTask_WaitMoveArrived::CompleteIfAllArrived()
{
	if (!bCompleted && PendingCharacters.IsEmpty())
	{
		Complete(false);
	}
}

void UAbilityTask_WaitMoveArrived::Complete(const bool bTimedOut)
{
	bCompleted = true;
	UnbindAll();

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnArrived.Broadcast(bTimedOut);
	}
	EndTask();
}

void UAbilityTask_WaitMoveArrived::UnbindAll()
{
	for (const TPair<TWeakObjectPtr<ALetheCharacterBase>, FDelegateHandle>& Pair : BoundHandles)
	{
		if (Pair.Key.IsValid())
		{
			Pair.Key->OnMoveFinished.Remove(Pair.Value);
		}
	}
	BoundHandles.Reset();
}
