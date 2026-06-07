// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckBoxes.h"

#include "Lethe/Lethe.h"

ADeckBoxes::ADeckBoxes()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>("Root");
	SetRootComponent(Root);

	DeckBox0 = CreateDefaultSubobject<USkeletalMeshComponent>("DeckBox0");
	DeckBox0->SetupAttachment(Root);

	DeckBox1 = CreateDefaultSubobject<USkeletalMeshComponent>("DeckBox1");
	DeckBox1->SetupAttachment(Root);
	
	DeckBox2 = CreateDefaultSubobject<USkeletalMeshComponent>("DeckBox2");
	DeckBox2->SetupAttachment(Root);

	DeckBox3 = CreateDefaultSubobject<USkeletalMeshComponent>("DeckBox3");
	DeckBox3->SetupAttachment(Root);

	DeckBoxes.Reserve(PLAYER_CHARACTER_NUMBER);
	DeckBoxes.Add(DeckBox0);
	DeckBoxes.Add(DeckBox1);
	DeckBoxes.Add(DeckBox2);
	DeckBoxes.Add(DeckBox3);

	LeftCap = CreateDefaultSubobject<UStaticMeshComponent>("LeftCap");
	LeftCap->SetupAttachment(Root);

	Middle = CreateDefaultSubobject<UStaticMeshComponent>("Middle");
	Middle->SetupAttachment(Root);

	RightCap = CreateDefaultSubobject<UStaticMeshComponent>("RightCap");
	RightCap->SetupAttachment(Root);
}

void ADeckBoxes::UpdateLocations(const TArray<int32>& HandCounts)
{
	TArray<FVector> DeckBoxLocations;
	DeckBoxLocations.Init(FVector(DefaultDeckBoxXLocation, 0.f, 0.f), HandCounts.Num());

	int32 AllHandCount = 0;
	
	// 덱 박스는 (자신의 좌측 덱에서 뽑힌 핸드 수 * 8.f)만큼 우측으로 이동합니다.
	for (int32 HandCountIndex = 0; HandCountIndex < HandCounts.Num(); ++HandCountIndex)
	{
		AllHandCount += HandCounts[HandCountIndex];
		for (int32 DeckBoxIndex = HandCountIndex + 1; DeckBoxIndex < DeckBoxLocations.Num(); ++DeckBoxIndex)
		{
			DeckBoxLocations[DeckBoxIndex].X += HandCounts[HandCountIndex] * DeckBoxOffsetByHandCount;
		}
	}

	// 덱 박스 순서에 따라 10.f씩 추가로 이동합니다.
	for (int32 DeckBoxIndex = 0; DeckBoxIndex < DeckBoxLocations.Num(); ++DeckBoxIndex)
	{
		DeckBoxLocations[DeckBoxIndex].X += DeckBoxOffsetByDeckBox * DeckBoxIndex;
		if (DeckBoxes.IsValidIndex(DeckBoxIndex))
		{
			DeckBoxes[DeckBoxIndex]->SetRelativeLocation(DeckBoxLocations[DeckBoxIndex]);
		}
	}

	const FVector RightCapLocation = FVector(DefaultRightCapXLocation + AllHandCount * DeckBoxOffsetByHandCount, 0.f, 0.f);
	RightCap->SetRelativeLocation(RightCapLocation);

	const FVector MiddleCapLocation = FVector(AllHandCount * (DeckBoxOffsetByHandCount / 2.f), 0.f, 0.f);
	Middle->SetRelativeLocation(MiddleCapLocation);
}

void ADeckBoxes::GetDeckLocations(TArray<FVector>& DeckLocations) const
{
	for (const auto& DeckBox : DeckBoxes)
	{
		if (DeckBox)
		{
			DeckLocations.Add(DeckBox->GetComponentLocation());
		}
	}
}

FVector ADeckBoxes::GetDeckLocation(const int32 DeckIndex) const
{
	if (DeckBoxes.IsValidIndex(DeckIndex))
	{
		return DeckBoxes[DeckIndex]->GetRelativeLocation();
	}
	return FVector::ZeroVector;
}
