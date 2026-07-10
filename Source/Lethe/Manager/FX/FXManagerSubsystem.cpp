// Copyright JETBLU, Inc. All Rights Reserved.

#include "FXManagerSubsystem.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/LetheLog.h"
#include "Lethe/Manager/World/LevelManagerSubsystem.h"

void UFXManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<ULevelManagerSubsystem>();

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULevelManagerSubsystem* LevelManagerSubsystem = GameInstance->GetSubsystem<ULevelManagerSubsystem>())
		{
			OnLevelChangeStartedHandle = LevelManagerSubsystem->OnStartLevelChange.AddUObject(this, &ThisClass::RemoveAllPending);
		}
	}

	// 모든 데이터 테이블을 동기 로드합니다.
	if (const UDataTable* SoundDataTable = SoundDataTablePath.LoadSynchronous())
	{
		const FString Context(TEXT("FXManagerSubsystem - Sound"));
		TArray<FTaggedSoundRow*> Rows;
		SoundDataTable->GetAllRows<FTaggedSoundRow>(Context, Rows);

		for (const FTaggedSoundRow* Row : Rows)
		{
			if (Row && Row->SoundTag.IsValid() && !Row->SoundAsset.IsNull())
			{
				if (!ensureMsgf(!SoundMap.Contains(Row->SoundTag), TEXT("GameplayTag: %s가 중복인 사운드 에셋이 존재합니다."), *Row->SoundTag.ToString()))
				{
					continue;
				}
				SoundMap.Add(Row->SoundTag, Row->SoundAsset);
			}
		}
	}

	if (const UDataTable* NiagaraDataTable = NiagaraDataTablePath.LoadSynchronous())
	{
		const FString Context(TEXT("FXManagerSubsystem - Niagara"));
		TArray<FTaggedNiagaraRow*> Rows;
		NiagaraDataTable->GetAllRows<FTaggedNiagaraRow>(Context, Rows);

		for (const FTaggedNiagaraRow* Row : Rows)
		{
			if (Row && Row->NiagaraTag.IsValid() && !Row->NiagaraAsset.IsNull())
			{
				if (!ensureMsgf(!NiagaraMap.Contains(Row->NiagaraTag), TEXT("GameplayTag: %s가 중복인 나이아가라 에셋이 존재합니다."), *Row->NiagaraTag.ToString()))
				{
					continue;
				}
				NiagaraMap.Add(Row->NiagaraTag, Row->NiagaraAsset);
			}
		}
	}
}

void UFXManagerSubsystem::RemoveAllPending()
{
	// 안전하게 종료하기 위해 모든 반환 요청 콜백에 대해 nullptr을 반환한 후, 재생 요청과 반환 요청을 모두 제거합니다.
	TArray<TFunction<void(USoundBase*)>> SoundGetterCallbacks;
	TArray<TFunction<void(UNiagaraSystem*)>> NiagaraGetterCallbacks;
	
	{
		FScopeLock Lock(&PendingRequestsLock);
		for (auto& Request : PendingSoundLoadRequests)
		{
			if (Request.Value.StreamableHandle.IsValid())
			{
				Request.Value.StreamableHandle->CancelHandle();
			}
			SoundGetterCallbacks.Append(MoveTemp(Request.Value.GetterCallbacks));
		}
		for (auto& Request : PendingNiagaraLoadRequests)
		{
			if (Request.Value.StreamableHandle.IsValid())
			{
				Request.Value.StreamableHandle->CancelHandle();
			}
			NiagaraGetterCallbacks.Append(MoveTemp(Request.Value.GetterCallbacks));
		}
		PendingSoundLoadRequests.Empty();
		PendingNiagaraLoadRequests.Empty();
	}
	
	for (const auto& Callback : SoundGetterCallbacks)
	{
		Callback(nullptr);
	}
	for (const auto& Callback : NiagaraGetterCallbacks)
	{
		Callback(nullptr);
	}
}

void UFXManagerSubsystem::Deinitialize()
{
	RemoveAllPending();

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULevelManagerSubsystem* LevelManagerSubsystem = GameInstance->GetSubsystem<ULevelManagerSubsystem>())
		{
			LevelManagerSubsystem->OnStartLevelChange.Remove(OnLevelChangeStartedHandle);
		}
	}
	
	Super::Deinitialize();
}

void UFXManagerSubsystem::AsyncGetSound(const FGameplayTag& SoundTag, const TFunction<void(USoundBase*)>& OnLoadedCallback)
{
	if (!SoundTag.IsValid())
	{
		ensureMsgf(false, TEXT("비동기 로드를 요청한 SoundTag가 유효하지 않습니다."));
		OnLoadedCallback(nullptr);
		return;
	}
	
	const TSoftObjectPtr<USoundBase> SoundToLoad = SoundMap.FindRef(SoundTag);
	if (SoundToLoad.IsNull())
	{
		LETHE_LOG(LogFXManager, Warning, "SoundTag %s에 해당하는 사운드를 찾을 수 없습니다.", *SoundTag.ToString());
		OnLoadedCallback(nullptr);
		return;
	}
	
	// 이미 에셋이 로드되어있는 경우 즉시 콜백 함수를 호출합니다.
	if (SoundToLoad.IsValid())
	{
		OnLoadedCallback(SoundToLoad.Get());
		return;
	}
	
	FScopeLock Lock(&PendingRequestsLock);

	const FSoftObjectPath AssetPath = SoundToLoad.ToSoftObjectPath();
	
	// 이미 로드 중인 경우 콜백 함수만 등록합니다.
	if (PendingSoundLoadRequests.Contains(AssetPath))
	{
		PendingSoundLoadRequests[AssetPath].GetterCallbacks.Add(OnLoadedCallback);
		return;
	}

	// 처음 요청된 태그인 경우 콜백 리스트를 생성합니다.
	FSoundAsyncLoadRequest& NewRequest = PendingSoundLoadRequests.Add(AssetPath);
	NewRequest.GetterCallbacks.Add(OnLoadedCallback);
	
	FStreamableManager& StreamableManager = GetStreamableManager();
	FStreamableDelegate StreamableCompleteDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnSoundAsyncLoadComplete, AssetPath);
	NewRequest.StreamableHandle = StreamableManager.RequestAsyncLoad(AssetPath, StreamableCompleteDelegate);
}

void UFXManagerSubsystem::OnSoundAsyncLoadComplete(FSoftObjectPath LoadedAssetPath)
{
	FSoundAsyncLoadRequest CompletedRequest;
	{
		FScopeLock Lock(&PendingRequestsLock);
		if (FSoundAsyncLoadRequest* FoundRequest = PendingSoundLoadRequests.Find(LoadedAssetPath))
		{
			CompletedRequest = MoveTemp(*FoundRequest);
			PendingSoundLoadRequests.Remove(LoadedAssetPath);
		}
		else
		{
			return;
		}
	}

	USoundBase* LoadedSound = Cast<USoundBase>(LoadedAssetPath.ResolveObject());

	if (LoadedSound)
	{
		for (const auto& Callback : CompletedRequest.GetterCallbacks)
		{
			Callback(LoadedSound);
		}
	}
	else
	{
		LETHE_LOG(LogFXManager, Warning, "로드 후, USoundBase가 유효하지 않습니다.");
		for (const auto& Callback : CompletedRequest.GetterCallbacks)
		{
			Callback(nullptr);
		}
	}
}

void UFXManagerSubsystem::AsyncPlaySoundAtLocation(const FGameplayTag& SoundTag, const FVector Location, const FRotator Rotation, const float VolumeMultiplier, const float PitchMultiplier)
{
	TWeakObjectPtr<UFXManagerSubsystem> WeakThis = MakeWeakObjectPtr(this);
	AsyncGetSound(SoundTag, [WeakThis, Location, Rotation, VolumeMultiplier, PitchMultiplier](USoundBase* LoadedSound)
	{
		if (WeakThis.IsValid() && LoadedSound)
		{
			UGameplayStatics::PlaySoundAtLocation(WeakThis.Get(), LoadedSound, Location, Rotation, VolumeMultiplier, PitchMultiplier);
		}
	});
}

void UFXManagerSubsystem::AsyncPlaySound2D(const FGameplayTag& SoundTag, const float VolumeMultiplier, const float PitchMultiplier)
{
	TWeakObjectPtr<UFXManagerSubsystem> WeakThis = MakeWeakObjectPtr(this);
	AsyncGetSound(SoundTag, [WeakThis, VolumeMultiplier, PitchMultiplier](USoundBase* LoadedSound)
	{
		if (WeakThis.IsValid() && LoadedSound)
		{
			UGameplayStatics::PlaySound2D(WeakThis.Get(), LoadedSound, VolumeMultiplier, PitchMultiplier);
		}
	});
}

void UFXManagerSubsystem::AsyncGetNiagara(const FGameplayTag& NiagaraTag, const TFunction<void(UNiagaraSystem*)>& OnLoadedCallback)
{
	if (!NiagaraTag.IsValid())
	{
		ensureMsgf(false, TEXT("비동기 로드를 요청한 NiagaraTag가 유효하지 않습니다."));
		OnLoadedCallback(nullptr);
		return;
	}
	
	const TSoftObjectPtr<UNiagaraSystem> NiagaraToLoad = NiagaraMap.FindRef(NiagaraTag);
	if (NiagaraToLoad.IsNull())
	{
		LETHE_LOG(LogFXManager, Warning, "NiagaraTag %s에 해당하는 나이아가라를 찾을 수 없습니다.", *NiagaraTag.ToString());
		OnLoadedCallback(nullptr);
		return;
	}

	// 이미 에셋이 로드되어 있는 경우 들어가는 분기입니다.
	if (NiagaraToLoad.IsValid())
	{
		OnLoadedCallback(NiagaraToLoad.Get());
		return;
	}
	
	FScopeLock Lock(&PendingRequestsLock);
	
	const FSoftObjectPath AssetPath = NiagaraToLoad.ToSoftObjectPath();
	
	// 이미 로드 중인 경우 콜백 함수만 등록하고 리턴합니다.
	if (PendingNiagaraLoadRequests.Contains(AssetPath))
	{
		PendingNiagaraLoadRequests[AssetPath].GetterCallbacks.Add(OnLoadedCallback);
		return;
	}

	// 처음 요청된 태그인 경우 콜백 리스트를 생성합니다.
	FNiagaraAsyncLoadRequest& NewRequest = PendingNiagaraLoadRequests.Add(AssetPath);
	NewRequest.GetterCallbacks.Add(OnLoadedCallback);
	
	FStreamableManager& StreamableManager = GetStreamableManager();
	FStreamableDelegate StreamableCompleteDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnNiagaraAsyncLoadComplete, AssetPath);
	NewRequest.StreamableHandle = StreamableManager.RequestAsyncLoad(AssetPath, StreamableCompleteDelegate);
}

void UFXManagerSubsystem::OnNiagaraAsyncLoadComplete(FSoftObjectPath LoadedAssetPath)
{
	FNiagaraAsyncLoadRequest CompletedRequest;
	{
		FScopeLock Lock(&PendingRequestsLock);
		if (FNiagaraAsyncLoadRequest* FoundRequest = PendingNiagaraLoadRequests.Find(LoadedAssetPath))
		{
			CompletedRequest = MoveTemp(*FoundRequest);
			PendingNiagaraLoadRequests.Remove(LoadedAssetPath);
		}
		else
		{
			return;
		}
	}

	UNiagaraSystem* LoadedNiagara = Cast<UNiagaraSystem>(LoadedAssetPath.ResolveObject());

	if (LoadedNiagara)
	{
		for (const auto& Callback : CompletedRequest.GetterCallbacks)
		{
			Callback(LoadedNiagara);
		}
	}
	else
	{
		LETHE_LOG(LogFXManager, Warning, "로드 후, UNiagaraSystem이 유효하지 않습니다.");
		for (const auto& Callback : CompletedRequest.GetterCallbacks)
		{
			Callback(nullptr);
		}
	}
}

void UFXManagerSubsystem::AsyncSpawnNiagaraAtLocation(const FGameplayTag& NiagaraTag, const FVector Location, const FRotator Rotation, const FVector Scale, bool bAutoDestroy, bool bAutoActivate)
{
	TWeakObjectPtr<UFXManagerSubsystem> WeakThis = MakeWeakObjectPtr(this);
	AsyncGetNiagara(NiagaraTag, [WeakThis, Location, Rotation, Scale, bAutoDestroy, bAutoActivate](UNiagaraSystem* LoadedNiagara)
	{
		if (WeakThis.IsValid() && LoadedNiagara)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(WeakThis.Get(), LoadedNiagara, Location, Rotation, Scale, bAutoDestroy, bAutoActivate);
		}
	});
}

USoundBase* UFXManagerSubsystem::GetSound(const FGameplayTag& SoundTag) const
{
	if (!SoundTag.IsValid())
	{
		ensureMsgf(false, TEXT("동기 로드를 요청한 SoundTag가 유효하지 않습니다."));
		return nullptr;
	}
	
	const TSoftObjectPtr<USoundBase> SoundToLoad = SoundMap.FindRef(SoundTag);
	if (SoundToLoad.IsNull())
	{
		LETHE_LOG(LogFXManager, Warning, "SoundTag %s에 해당하는 사운드를 찾을 수 없습니다.", *SoundTag.ToString());
		return nullptr;
	}

	return SoundToLoad.LoadSynchronous();
}

UNiagaraSystem* UFXManagerSubsystem::GetNiagara(const FGameplayTag& NiagaraTag) const
{
	if (!NiagaraTag.IsValid())
	{
		ensureMsgf(false, TEXT("동기 로드를 요청한 NiagaraTag가 유효하지 않습니다."));
		return nullptr;
	}
	
	const TSoftObjectPtr<UNiagaraSystem> NiagaraToLoad = NiagaraMap.FindRef(NiagaraTag);
	if (NiagaraToLoad.IsNull())
	{
		LETHE_LOG(LogFXManager, Warning, "NiagaraTag %s에 해당하는 나이아가라를 찾을 수 없습니다.", *NiagaraTag.ToString());
		return nullptr;
	}

	return NiagaraToLoad.LoadSynchronous();
}

FStreamableManager& UFXManagerSubsystem::GetStreamableManager() const
{
	return UAssetManager::Get().GetStreamableManager();
}
