// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DeckBoxes.generated.h"

class UBoxComponent;

enum class EDeckBoxOpenReason : uint8
{
	None = 0,

	/** 덱 박스에 마우스를 올린 경우입니다. */
	MouseHover = 1 << 0,

	/** 비전투 상황에 덱 박스를 클릭한 경우입니다. */
	Pinned = 1 << 1,

	/** 전투 상황이 되어 모두 열어야 하는 경우입니다. */
	Battle = 1 << 2,
};
ENUM_CLASS_FLAGS(EDeckBoxOpenReason)

UCLASS()
class LETHE_API ADeckBoxes : public AActor
{
	GENERATED_BODY()

public:
	ADeckBoxes();

	void UpdateLocations(const TArray<int32>& HandCounts);
	void GetDeckLocations(TArray<FVector>& OutDeckLocations) const;
	FVector GetDeckLocation(const int32 DeckIndex) const;
	int32 GetDeckIndex(const UBoxComponent* InDeckBoxCollision) const;

	void SetOpenReason(const UBoxComponent* InDeckBoxCollision, const EDeckBoxOpenReason InOpenReason, const bool bEnable);
	void SetAllOpenReason(const EDeckBoxOpenReason InOpenReason, const bool bEnable, const bool bShouldApply = true);

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End of AActor Interface

private:
	void InitBox(UBoxComponent* BoxCollision, USkeletalMeshComponent* DeckBox);

	void ApplyDeckBoxesOpenState();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> DeckBoxCollision0;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> DeckBoxCollision1;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> DeckBoxCollision2;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> DeckBoxCollision3;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> DeckBox0;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> DeckBox1;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> DeckBox2;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> DeckBox3;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> LeftCap;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Middle;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> RightCap;

private:
	UPROPERTY()
	TArray<TObjectPtr<UBoxComponent>> DeckBoxCollisions;
	
	UPROPERTY()
	TArray<TObjectPtr<USkeletalMeshComponent>> DeckBoxes;

	TArray<EDeckBoxOpenReason> PreviousOpenReasons;
	TArray<EDeckBoxOpenReason> OpenReasons;

	float DefaultDeckBoxXLocation = 6.f;
	float DeckBoxOffsetByDeckBox = 10.f;
	float DeckBoxOffsetByHandCount = 8.f;
	float DefaultRightCapXLocation = 42.f;
};
