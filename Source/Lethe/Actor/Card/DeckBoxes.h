// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DeckBoxes.generated.h"

UCLASS()
class LETHE_API ADeckBoxes : public AActor
{
	GENERATED_BODY()

public:
	ADeckBoxes();

	void UpdateLocations(const TArray<int32>& HandCounts);
	void GetDeckLocations(TArray<FVector>& DeckLocations) const;
	FVector GetDeckLocation(const int32 DeckIndex) const;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;
	
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
	TArray<TObjectPtr<USkeletalMeshComponent>> DeckBoxes;

	float DefaultDeckBoxXLocation = 6.f;
	float DeckBoxOffsetByDeckBox = 10.f;
	float DeckBoxOffsetByHandCount = 8.f;
	float DefaultRightCapXLocation = 42.f;
};
