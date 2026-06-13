// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DeckBoxes.generated.h"

class UBoxComponent;

UCLASS()
class LETHE_API ADeckBoxes : public AActor
{
	GENERATED_BODY()

public:
	ADeckBoxes();

	void UpdateLocations(const TArray<int32>& HandCounts);
	void GetDeckLocations(TArray<FVector>& DeckLocations) const;
	FVector GetDeckLocation(const int32 DeckIndex) const;

	void OpenDeckBox(UBoxComponent* InDeckBoxCollision);
	void CloseDeckBox(UBoxComponent* InDeckBoxCollision);
	void OpenAllBoxes();
	void CloseAllBoxes();

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End of AActor Interface

private:
	void InitBox(UBoxComponent* BoxCollision, USkeletalMeshComponent* DeckBox);

	void OpenDeckBox(const int32 DeckIndex);
	void CloseDeckBox(const int32 DeckIndex);

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

	TArray<bool> OpenedStates;

	float DefaultDeckBoxXLocation = 6.f;
	float DeckBoxOffsetByDeckBox = 10.f;
	float DeckBoxOffsetByHandCount = 8.f;
	float DefaultRightCapXLocation = 42.f;
};
