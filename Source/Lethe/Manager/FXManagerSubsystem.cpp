// Copyright JETBLU, Inc. All Rights Reserved.

#include "FXManagerSubsystem.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"
#include "World/LevelManagerSubsystem.h"

DEFINE_LOG_CATEGORY(LogFXManager);

void UFXManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<ULevelManagerSubsystem>();

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULevelManagerSubsystem* LevelManagerSubsystem = GameInstance->GetSubsystem<ULevelManagerSubsystem>())
		{
			OnLevelChangeStartedHandle = LevelManagerSubsystem->OnStartLevelChanged.AddUObject(this, &ThisClass::RemoveAllPending);
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
				SoundMap.Add(Row->SoundTag, Row->SoundAsset);
			}
		}
	}

	SoundDataTablePath.Reset();

	if (const UDataTable* NiagaraDataTable = NiagaraDataTablePath.LoadSynchronous())
	{
		const FString Context(TEXT("FXManagerSubsystem - Niagara"));
		TArray<FTaggedNiagaraRow*> Rows;
		NiagaraDataTable->GetAllRows<FTaggedNiagaraRow>(Context, Rows);

		for (const FTaggedNiagaraRow* Row : Rows)
		{
			if (Row && Row->NiagaraTag.IsValid() && !Row->NiagaraAsset.IsNull())
			{
				NiagaraMap.Add(Row->NiagaraTag, Row->NiagaraAsset);
			}
		}
	}
	
	NiagaraDataTablePath.Reset();
}

void UFXManagerSubsystem::RemoveAllPending()
{
	// 안전하게 종료하기 위해 모든 반환 요청 콜백에 대해 nullptr을 반환한 후, 재생 요청과 반환 요청을 모두 제거합니다.
	TArray<TFunction<void(USoundBase*)>> SoundGetterCallbacks;
	TArray<TFunction<void(UNiagaraSystem*)>> NiagaraGetterCallbacks;
	
	{
		FScopeLock Lock(&PendingRequestsLock);
		for (auto& Requests : PendingSoundLoadRequests)
		{
			SoundGetterCallbacks.Append(MoveTemp(Requests.Value.GetterCallbacks));
		}
		for (auto& Requests : PendingNiagaraLoadRequests)
		{
			NiagaraGetterCallbacks.Append(MoveTemp(Requests.Value.GetterCallbacks));
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
			LevelManagerSubsystem->OnStartLevelChanged.Remove(OnLevelChangeStartedHandle);
		}
	}
	
	Super::Deinitialize();
}

void UFXManagerSubsystem::AsyncPlaySoundAtLocation(const FGameplayTag& SoundTag, const FVector Location, const FRotator Rotation, const float VolumeMultiplier, const float PitchMultiplier)
{
	FStreamableManager& StreamableManager = GetStreamableManager();
	if (!SoundTag.IsValid())
	{
		ensureMsgf(false, TEXT("비동기 로드를 요청한 SoundTag가 유효하지 않습니다."));
		return;
	}
	
	const TSoftObjectPtr<USoundBase> SoundToLoad = SoundMap.FindRef(SoundTag);
	if (SoundToLoad.IsNull())
	{
		UE_LOG(LogFXManager, Warning, TEXT("SoundTag %s에 해당하는 사운드를 찾을 수 없습니다."), *SoundTag.ToString());
		return;
	}
	
	// 이미 에셋이 로드되어있는 경우 들어가는 분기입니다.
	if (SoundToLoad.IsValid())
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundToLoad.Get(), Location, Rotation, VolumeMultiplier, PitchMultiplier);
		return;
	}
	
	FScopeLock Lock(&PendingRequestsLock);
	
	FSoftObjectPath AssetPath = SoundToLoad.ToSoftObjectPath();
	
	// 함수를 바인드하기 위한 변수를 선언 및 초기화합니다.
	FSoundAsyncPlayData NewPlayData;
	NewPlayData.Location = Location;
	NewPlayData.Rotation = Rotation;
	NewPlayData.VolumeMultiplier = VolumeMultiplier;
	NewPlayData.PitchMultiplier = PitchMultiplier;
	
	// 이미 로드 중인 경우 들어가는 분기입니다.
	if (FSoundAsyncLoadRequest* ExistingRequest = PendingSoundLoadRequests.Find(AssetPath))
	{
		// 로드 중인 에셋이 로딩 완료 시점에 이 요청에 대해서도 함께 처리하기 위해 배열에 추가합니다.
		ExistingRequest->PlayRequests.Add(NewPlayData);
		return;
	}

	// 새로 로드를 시작해야 하는 경우 여기로 내려옵니다.
	// 에셋 로드가 끝난 뒤 호출되는 델리게이트에 함수를 바인드합니다.
	FSoundAsyncLoadRequest NewRequest;
	NewRequest.PlayRequests.Add(NewPlayData);
	PendingSoundLoadRequests.Add(AssetPath, NewRequest);
	
	FStreamableDelegate StreamableCompleteDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnSoundAsyncLoadComplete, AssetPath);
	StreamableManager.RequestAsyncLoad(AssetPath, StreamableCompleteDelegate);
}

void UFXManagerSubsystem::AsyncGetSound(const FGameplayTag& SoundTag, const TFunction<void(USoundBase*)>& OnLoadedCallback)
{
	FStreamableManager& StreamableManager = GetStreamableManager();
	if (!SoundTag.IsValid())
	{
		ensureMsgf(false, TEXT("비동기 로드를 요청한 SoundTag가 유효하지 않습니다."));
		OnLoadedCallback(nullptr);
		return;
	}
	
	const TSoftObjectPtr<USoundBase> SoundToLoad = SoundMap.FindRef(SoundTag);
	if (SoundToLoad.IsNull())
	{
		UE_LOG(LogFXManager, Warning, TEXT("SoundTag %s에 해당하는 사운드를 찾을 수 없습니다."), *SoundTag.ToString());
		OnLoadedCallback(nullptr);
		return;
	}

	const FSoftObjectPath AssetPath = SoundToLoad.ToSoftObjectPath();

	// 이미 에셋이 로드되어있는 경우 즉시 콜백 함수를 호출합니다.
	if (SoundToLoad.IsValid())
	{
		OnLoadedCallback(SoundToLoad.Get());
		return;
	}
	
	FScopeLock Lock(&PendingRequestsLock);

	// 이미 로드 중인 경우 콜백 함수만 등록합니다.
	if (PendingSoundLoadRequests.Contains(AssetPath))
	{
		PendingSoundLoadRequests[AssetPath].GetterCallbacks.Add(OnLoadedCallback);
		return;
	}

	// 처음 요청된 태그인 경우 콜백 리스트를 생성합니다.
	FSoundAsyncLoadRequest NewRequest;
	NewRequest.GetterCallbacks.Add(OnLoadedCallback);
	PendingSoundLoadRequests.Add(AssetPath, NewRequest);
	
	FStreamableDelegate StreamableCompleteDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnSoundAsyncLoadComplete, AssetPath);
	StreamableManager.RequestAsyncLoad(AssetPath, StreamableCompleteDelegate);
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

	// 로드 완료 후 사운드 재생을 시작합니다.
	USoundBase* LoadedSound = Cast<USoundBase>(LoadedAssetPath.ResolveObject());

	if (LoadedSound)
	{
		for (const auto& PlayData : CompletedRequest.PlayRequests)
		{
			UGameplayStatics::PlaySoundAtLocation(this, LoadedSound, PlayData.Location, PlayData.Rotation, PlayData.VolumeMultiplier, PlayData.PitchMultiplier);
		}
		for (const auto& Callback : CompletedRequest.GetterCallbacks)
		{
			Callback(LoadedSound);
		}
	}
	else
	{
		UE_LOG(LogFXManager, Warning, TEXT("로딩 후, USoundBase가 유효하지 않습니다."));
	}
}

void UFXManagerSubsystem::AsyncSpawnNiagaraAtLocation(const FGameplayTag& NiagaraTag, const FVector Location, const FRotator Rotation, const FVector Scale, bool bAutoDestroy, bool bAutoActivate)
{
	FStreamableManager& StreamableManager = GetStreamableManager();
	if (!NiagaraTag.IsValid())
	{
		ensureMsgf(false, TEXT("비동기 로드를 요청한 NiagaraTag가 유효하지 않습니다."));
		return;
	}
	
	const TSoftObjectPtr<UNiagaraSystem> NiagaraToLoad = NiagaraMap.FindRef(NiagaraTag);
	if (NiagaraToLoad.IsNull())
	{
		UE_LOG(LogFXManager, Warning, TEXT("NiagaraTag %s에 해당하는 나이아가라를 찾을 수 없습니다."), *NiagaraTag.ToString());
		return;
	}

	// 이미 에셋이 로드되어있는 경우 들어가는 분기입니다.
	if (NiagaraToLoad.IsValid())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, NiagaraToLoad.Get(), Location, Rotation, Scale, bAutoDestroy, bAutoActivate);
		return;
	}
	
	FScopeLock Lock(&PendingRequestsLock);
	
	FSoftObjectPath AssetPath = NiagaraToLoad.ToSoftObjectPath();
	
	FNiagaraAsyncSpawnData NewPlayData;
	NewPlayData.Location = Location;
	NewPlayData.Rotation = Rotation;
	NewPlayData.Scale = Scale;
	NewPlayData.bAutoDestroy = bAutoDestroy;
	NewPlayData.bAutoActivate = bAutoActivate;
	
	// 이미 로드 중인 경우 들어가는 분기입니다.
	if (FNiagaraAsyncLoadRequest* ExistingRequest = PendingNiagaraLoadRequests.Find(AssetPath))
	{
		ExistingRequest->SpawnRequests.Add(NewPlayData);
		return;
	}
	
	// 새로 로드를 시작해야 하는 경우 여기로 내려옵니다.
	// 에셋 로딩이 완료되면 위에서 초기화한 정보들을 참조할 수 있도록 배열에 추가합니다.
	FNiagaraAsyncLoadRequest NewRequest;
	NewRequest.SpawnRequests.Add(NewPlayData);
	PendingNiagaraLoadRequests.Add(AssetPath, NewRequest);
	
	FStreamableDelegate StreamableCompleteDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnNiagaraAsyncLoadComplete, AssetPath);
	StreamableManager.RequestAsyncLoad(AssetPath, StreamableCompleteDelegate);
}

void UFXManagerSubsystem::AsyncGetNiagara(const FGameplayTag& NiagaraTag, const TFunction<void(UNiagaraSystem*)>& OnLoadedCallback)
{
	FStreamableManager& StreamableManager = GetStreamableManager();
	if (!NiagaraTag.IsValid())
	{
		ensureMsgf(false, TEXT("비동기 로드를 요청한 NiagaraTag가 유효하지 않습니다."));
		OnLoadedCallback(nullptr);
		return;
	}
	
	const TSoftObjectPtr<UNiagaraSystem> NiagaraToLoad = NiagaraMap.FindRef(NiagaraTag);
	if (NiagaraToLoad.IsNull())
	{
		UE_LOG(LogFXManager, Warning, TEXT("NiagaraTag %s에 해당하는 나이아가라를 찾을 수 없습니다."), *NiagaraTag.ToString());
		OnLoadedCallback(nullptr);
		return;
	}

	const FSoftObjectPath AssetPath = NiagaraToLoad.ToSoftObjectPath();

	// 이미 에셋이 로드되어있는 경우 들어가는 분기입니다.
	if (NiagaraToLoad.IsValid())
	{
		OnLoadedCallback(NiagaraToLoad.Get());
		return;
	}
	
	FScopeLock Lock(&PendingRequestsLock);

	// 이미 로드 중인 경우 콜백 함수만 등록하고 리턴합니다.
	if (PendingNiagaraLoadRequests.Contains(AssetPath))
	{
		PendingNiagaraLoadRequests[AssetPath].GetterCallbacks.Add(OnLoadedCallback);
		return;
	}

	// 처음 요청된 태그인 경우 콜백 리스트를 생성합니다.
	FNiagaraAsyncLoadRequest NewRequest;
	NewRequest.GetterCallbacks.Add(OnLoadedCallback);
	PendingNiagaraLoadRequests.Add(AssetPath, NewRequest);
	
	FStreamableDelegate StreamableCompleteDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnNiagaraAsyncLoadComplete, AssetPath);
	StreamableManager.RequestAsyncLoad(AssetPath, StreamableCompleteDelegate);
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
		for (const auto& PlayData : CompletedRequest.SpawnRequests)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, LoadedNiagara, PlayData.Location, PlayData.Rotation, PlayData.Scale, PlayData.bAutoDestroy, PlayData.bAutoActivate);
		}
		for (const auto& Callback : CompletedRequest.GetterCallbacks)
		{
			Callback(LoadedNiagara);
		}
	}
	else
	{
		UE_LOG(LogFXManager, Warning, TEXT("로딩 후, UNiagaraSystem이 유효하지 않습니다."));
	}
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
		UE_LOG(LogFXManager, Warning, TEXT("SoundTag %s에 해당하는 사운드를 찾을 수 없습니다."), *SoundTag.ToString());
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
		UE_LOG(LogFXManager, Warning, TEXT("NiagaraTag %s에 해당하는 나이아가라를 찾을 수 없습니다."), *NiagaraTag.ToString());
		return nullptr;
	}

	return NiagaraToLoad.LoadSynchronous();
}

FStreamableManager& UFXManagerSubsystem::GetStreamableManager() const
{
	return UAssetManager::Get().GetStreamableManager();
}
