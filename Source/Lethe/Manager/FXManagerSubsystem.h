// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NiagaraSystem.h"
#include "Engine/StreamableManager.h"
#include "FXManagerSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FTaggedSoundRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag SoundTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<USoundBase> SoundAsset;
};

USTRUCT(BlueprintType)
struct FTaggedNiagaraRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag NiagaraTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UNiagaraSystem> NiagaraAsset;
};

USTRUCT()
struct FSoundAsyncPlayData
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY()
	float VolumeMultiplier = 1.f;

	UPROPERTY()
	float PitchMultiplier = 1.f;
};

USTRUCT()
struct FSoundAsyncLoadRequest
{
	GENERATED_BODY()

	TSharedPtr<FStreamableHandle> StreamableHandle;
	
	UPROPERTY()
	TArray<FSoundAsyncPlayData> PlayRequests;

	TArray<TFunction<void(USoundBase*)>> GetterCallbacks;
};

USTRUCT()
struct FNiagaraAsyncSpawnData
{
	GENERATED_BODY()
	
	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY()
	FVector Scale = FVector(1.f);

	UPROPERTY()
	bool bAutoDestroy = true;

	UPROPERTY()
	bool bAutoActivate = true;
};

USTRUCT()
struct FNiagaraAsyncLoadRequest
{
	GENERATED_BODY()

	TSharedPtr<FStreamableHandle> StreamableHandle;
	
	UPROPERTY()
	TArray<FNiagaraAsyncSpawnData> SpawnRequests;

	TArray<TFunction<void(UNiagaraSystem*)>> GetterCallbacks;
};

DECLARE_LOG_CATEGORY_EXTERN(LogFXManager, Log, All);

/**
 * 나이아가라와 사운드를 비동기/동기 로드해 재생하거나 반환해주는 매니저 클래스입니다.
 * Tag와 1:1 매핑되는 에셋을 TSoftObjectPtr로 들고 있으며, 요청이 들어올 경우 [즉시 반환/비동기 로드 시작/콜백에 등록] 중 하나의 동작을 선택합니다.
 * 로드 도중 Level 이동 시 모든 Pending을 취소하므로, 이 점에 유의해 사용해야 합니다.
 */
UCLASS(Config = Game)
class LETHE_API UFXManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin Subsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End Subsystem Interface

	/** 사운드를 비동기 로드할 때 사용하는 함수입니다. */
	UFUNCTION(BlueprintCallable, Category = "FX")
	void AsyncPlaySoundAtLocation(const FGameplayTag& SoundTag, const FVector Location, const FRotator Rotation = FRotator::ZeroRotator, const float VolumeMultiplier = 1.f, const float PitchMultiplier = 1.f);
	void AsyncGetSound(const FGameplayTag& SoundTag, const TFunction<void(USoundBase*)>& OnLoadedCallback);

	/** 나이아가라를 비동기 로드할 때 사용하는 함수입니다. */
	UFUNCTION(BlueprintCallable, Category = "FX")
	void AsyncSpawnNiagaraAtLocation(const FGameplayTag& NiagaraTag, const FVector Location, const FRotator Rotation = FRotator::ZeroRotator, const FVector Scale = FVector(1.f), bool bAutoDestroy = true, bool bAutoActivate = true);
	void AsyncGetNiagara(const FGameplayTag& NiagaraTag, const TFunction<void(UNiagaraSystem*)>& OnLoadedCallback);

	/** 동기 로드를 원하는 경우 사용하는 함수입니다. */
	USoundBase* GetSound(const FGameplayTag& SoundTag) const;
	UNiagaraSystem* GetNiagara(const FGameplayTag& NiagaraTag) const;

private:
	void OnSoundAsyncLoadComplete(FSoftObjectPath LoadedAssetPath);
	void OnNiagaraAsyncLoadComplete(FSoftObjectPath LoadedAssetPath);
	
	void RemoveAllPending();
	FStreamableManager& GetStreamableManager() const;

private:
	FDelegateHandle OnLevelChangeStartedHandle;
	
	/** DataTable에 매핑되어있는 Tag와 에셋들은 탐색 효율을 위해 TMap으로 재구성되므로, 메모리 효율을 위해 Soft로 선언합니다. */
	UPROPERTY(Config)
	TSoftObjectPtr<UDataTable> SoundDataTablePath;
	
	UPROPERTY(Config)
	TSoftObjectPtr<UDataTable> NiagaraDataTablePath;

	UPROPERTY()
	TMap<FGameplayTag, TSoftObjectPtr<USoundBase>> SoundMap;
	
	UPROPERTY()
	TMap<FGameplayTag, TSoftObjectPtr<UNiagaraSystem>> NiagaraMap;
	
	/** 동일한 에셋에 대한 요청이 여러 개 동시에 들어올 경우, 로드가 끝날 때 모두 처리할 수 있도록 콜백 함수나 재생 관련 변수를 캐싱해 대기시키는 TMap입니다. */
	UPROPERTY()
	TMap<FSoftObjectPath, FSoundAsyncLoadRequest> PendingSoundLoadRequests;
	
	UPROPERTY()
	TMap<FSoftObjectPath, FNiagaraAsyncLoadRequest> PendingNiagaraLoadRequests;

	/** 멀티스레딩 안전성 확보를 위한 변수입니다. */
	FCriticalSection PendingRequestsLock;
};
