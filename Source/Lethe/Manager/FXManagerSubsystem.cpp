// Copyright JETBLU, Inc. All Rights Reserved.

#include "FXManagerSubsystem.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"

void UFXManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	StreamableManager = &UAssetManager::Get().GetStreamableManager();

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

void UFXManagerSubsystem::Deinitialize()
{
	FScopeLock Lock(&PendingRequestsLock);
	// 안전하게 종료하기 위해 현재 로드 중인 에셋을 모두 취소합니다.
	PendingSoundLoadRequests.Empty();
	Super::Deinitialize();
}

void UFXManagerSubsystem::AsyncPlaySoundAtLocation(const FGameplayTag& SoundTag, const FVector Location, const FRotator Rotation, const float VolumeMultiplier, const float PitchMultiplier)
{
	if (!SoundTag.IsValid() || !StreamableManager)
	{
		return;
	}
	
	FScopeLock Lock(&PendingRequestsLock);
	
	const TSoftObjectPtr<USoundBase> SoundToLoad = SoundMap.FindRef(SoundTag);
	if (SoundToLoad.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("SoundTag %s에 해당하는 사운드를 찾을 수 없습니다."), *SoundTag.ToString());
		return;
	}
	
	// 이미 에셋이 로드되어있는 경우 들어가는 분기입니다.
	if (SoundToLoad.IsValid())
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundToLoad.Get(), Location, Rotation, VolumeMultiplier, PitchMultiplier);
		return;
	}
	
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
	FStreamableDelegate StreamableCompleteDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnSoundAsyncLoadComplete, AssetPath);
	StreamableManager->RequestAsyncLoad(AssetPath, StreamableCompleteDelegate);

	PendingSoundLoadRequests.Add(AssetPath, NewRequest);
}

void UFXManagerSubsystem::AsyncGetSound(const FGameplayTag& SoundTag, const TFunction<void(USoundBase*)>& OnLoadedCallback)
{
	if (!SoundTag.IsValid() || !StreamableManager)
	{
		return;
	}
	
	FScopeLock Lock(&PendingRequestsLock);
	
	const TSoftObjectPtr<USoundBase> SoundToLoad = SoundMap.FindRef(SoundTag);
	if (SoundToLoad.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("SoundTag %s에 해당하는 사운드를 찾을 수 없습니다."), *SoundTag.ToString());
		return;
	}

	const FSoftObjectPath AssetPath = SoundToLoad.ToSoftObjectPath();

	// 이미 에셋이 로드되어있는 경우 즉시 콜백 함수를 호출합니다.
	if (SoundToLoad.IsValid())
	{
		OnLoadedCallback(SoundToLoad.Get());
		return;
	}

	// 이미 로드 중인 경우 콜백 함수만 등록합니다.
	if (PendingSoundLoadRequests.Contains(AssetPath))
	{
		PendingSoundLoadRequests[AssetPath].GetterCallbacks.Add(OnLoadedCallback);
		return;
	}

	// 처음 요청된 태그인 경우 콜백 리스트를 생성합니다.
	FSoundAsyncLoadRequest NewRequest;
	NewRequest.GetterCallbacks.Add(OnLoadedCallback);
	FStreamableDelegate StreamableCompleteDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnSoundAsyncLoadComplete, AssetPath);
	StreamableManager->RequestAsyncLoad(AssetPath, StreamableCompleteDelegate);
	
	PendingSoundLoadRequests.Add(AssetPath, NewRequest);
}

void UFXManagerSubsystem::OnSoundAsyncLoadComplete(FSoftObjectPath LoadedAssetPath)
{
	FScopeLock Lock(&PendingRequestsLock);
	
	// 로드 완료 후 사운드 재생을 시작합니다.
	USoundBase* LoadedSound = Cast<USoundBase>(StreamableManager->LoadSynchronous(LoadedAssetPath, false));

	if (FSoundAsyncLoadRequest* CompletedRequest = PendingSoundLoadRequests.Find(LoadedAssetPath))
	{
		if (LoadedSound && GetWorld())
		{
			for (const auto& PlayData : CompletedRequest->PlayRequests)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), LoadedSound, PlayData.Location, PlayData.Rotation, PlayData.VolumeMultiplier, PlayData.PitchMultiplier);
			}
			for (const auto& Callback : CompletedRequest->GetterCallbacks)
			{
				Callback(LoadedSound);
			}			
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("로딩 후, USoundBase가 유효하지 않거나 월드를 찾을 수 없습니다."));
		}

		PendingSoundLoadRequests.Remove(LoadedAssetPath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("로딩 후, 등록된 콜백 요청을 찾을 수 없습니다."));
	}
}

void UFXManagerSubsystem::AsyncSpawnNiagaraAtLocation(const FGameplayTag& NiagaraTag, const FVector Location, const FRotator Rotation, const FVector Scale, bool bAutoDestroy, bool bAutoActivate)
{
	if (!NiagaraTag.IsValid() || !StreamableManager)
	{
		return;
	}
	
	FScopeLock Lock(&PendingRequestsLock);
	
	const TSoftObjectPtr<UNiagaraSystem> NiagaraToLoad = NiagaraMap.FindRef(NiagaraTag);
	if (NiagaraToLoad.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("NiagaraTag %s에 해당하는 나이아가라를 찾을 수 없습니다."), *NiagaraTag.ToString());
		return;
	}

	// 이미 에셋이 로드되어있는 경우 들어가는 분기입니다.
	if (NiagaraToLoad.IsValid())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NiagaraToLoad.Get(), Location, Rotation, Scale, bAutoDestroy, bAutoActivate);
		return;
	}
	
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
	FStreamableDelegate StreamableCompleteDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnNiagaraAsyncLoadComplete, AssetPath);
	StreamableManager->RequestAsyncLoad(AssetPath, StreamableCompleteDelegate);

	PendingNiagaraLoadRequests.Add(AssetPath, NewRequest);
}

void UFXManagerSubsystem::AsyncGetNiagara(const FGameplayTag& NiagaraTag, const TFunction<void(UNiagaraSystem*)>& OnLoadedCallback)
{
	if (!NiagaraTag.IsValid() || !StreamableManager)
	{
		return;
	}
	
	FScopeLock Lock(&PendingRequestsLock);
	
	const TSoftObjectPtr<UNiagaraSystem> NiagaraToLoad = NiagaraMap.FindRef(NiagaraTag);
	if (NiagaraToLoad.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("NiagaraTag %s에 해당하는 나이아가라를 찾을 수 없습니다."), *NiagaraTag.ToString());
		return;
	}

	const FSoftObjectPath AssetPath = NiagaraToLoad.ToSoftObjectPath();

	// 이미 에셋이 로드되어있는 경우 들어가는 분기입니다.
	if (NiagaraToLoad.IsValid())
	{
		OnLoadedCallback(NiagaraToLoad.Get());
		return;
	}

	// 이미 로드 중인 경우 콜백 함수만 등록하고 리턴합니다.
	if (PendingNiagaraLoadRequests.Contains(AssetPath))
	{
		PendingNiagaraLoadRequests[AssetPath].GetterCallbacks.Add(OnLoadedCallback);
		return;
	}

	// 처음 요청된 태그인 경우 콜백 리스트를 생성합니다.
	FNiagaraAsyncLoadRequest NewRequest;
	NewRequest.GetterCallbacks.Add(OnLoadedCallback);
	FStreamableDelegate StreamableCompleteDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnNiagaraAsyncLoadComplete, AssetPath);
	StreamableManager->RequestAsyncLoad(AssetPath, StreamableCompleteDelegate);
	
	PendingNiagaraLoadRequests.Add(AssetPath, NewRequest);
}

void UFXManagerSubsystem::OnNiagaraAsyncLoadComplete(FSoftObjectPath LoadedAssetPath)
{
	FScopeLock Lock(&PendingRequestsLock);

	UNiagaraSystem* LoadedNiagara = Cast<UNiagaraSystem>(StreamableManager->LoadSynchronous(LoadedAssetPath, false));

	if (FNiagaraAsyncLoadRequest* CompletedRequest = PendingNiagaraLoadRequests.Find(LoadedAssetPath))
	{
		if (LoadedNiagara && GetWorld())
		{
			for (const auto& PlayData : CompletedRequest->SpawnRequests)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), LoadedNiagara, PlayData.Location, PlayData.Rotation, PlayData.Scale, PlayData.bAutoDestroy, PlayData.bAutoActivate);
			}
			for (const auto& Callback : CompletedRequest->GetterCallbacks)
			{
				Callback(LoadedNiagara);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("로딩 후, UNiagaraSystem이 유효하지 않거나 월드를 찾을 수 없습니다."));
		}

		PendingNiagaraLoadRequests.Remove(LoadedAssetPath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("로딩 후, 등록된 콜백 요청을 찾을 수 없습니다."));
	}
}

USoundBase* UFXManagerSubsystem::GetSound(const FGameplayTag& SoundTag) const
{
	if (!SoundTag.IsValid())
	{
		return nullptr;
	}
	
	const TSoftObjectPtr<USoundBase> SoundToLoad = SoundMap.FindRef(SoundTag);
	if (SoundToLoad.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("SoundTag %s에 해당하는 사운드를 찾을 수 없습니다."), *SoundTag.ToString());
		return nullptr;
	}

	return SoundToLoad.LoadSynchronous();
}

UNiagaraSystem* UFXManagerSubsystem::GetNiagara(const FGameplayTag& NiagaraTag) const
{
	if (!NiagaraTag.IsValid())
	{
		return nullptr;
	}
	
	const TSoftObjectPtr<UNiagaraSystem> NiagaraToLoad = NiagaraMap.FindRef(NiagaraTag);
	if (NiagaraToLoad.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("NiagaraTag %s에 해당하는 나이아가라를 찾을 수 없습니다."), *NiagaraTag.ToString());
		return nullptr;
	}

	return NiagaraToLoad.LoadSynchronous();
}
