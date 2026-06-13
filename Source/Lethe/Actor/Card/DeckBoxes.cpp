// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckBoxes.h"

#include "Animation/AnimSingleNodeInstance.h"
#include "Components/BoxComponent.h"
#include "Lethe/Lethe.h"

ADeckBoxes::ADeckBoxes()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>("Root");
	SetRootComponent(Root);
	
	DeckBoxCollisions.Reset();
	DeckBoxCollisions.Reserve(PLAYER_CHARACTER_NUMBER);
	DeckBoxes.Reset();
	DeckBoxes.Reserve(PLAYER_CHARACTER_NUMBER);

	OpenedStates.Reset();
	OpenedStates.Init(false, PLAYER_CHARACTER_NUMBER);
	
	DeckBoxCollision0 = CreateDefaultSubobject<UBoxComponent>("DeckBoxCollision0");
	DeckBox0 = CreateDefaultSubobject<USkeletalMeshComponent>("DeckBox0");
	InitBox(DeckBoxCollision0, DeckBox0);

	DeckBoxCollision1 = CreateDefaultSubobject<UBoxComponent>("DeckBoxCollision1");
	DeckBox1 = CreateDefaultSubobject<USkeletalMeshComponent>("DeckBox1");
	InitBox(DeckBoxCollision1, DeckBox1);

	DeckBoxCollision2 = CreateDefaultSubobject<UBoxComponent>("DeckBoxCollision2");
	DeckBox2 = CreateDefaultSubobject<USkeletalMeshComponent>("DeckBox2");
	InitBox(DeckBoxCollision2, DeckBox2);

	DeckBoxCollision3 = CreateDefaultSubobject<UBoxComponent>("DeckBoxCollision3");
	DeckBox3 = CreateDefaultSubobject<USkeletalMeshComponent>("DeckBox3");
	InitBox(DeckBoxCollision3, DeckBox3);

	LeftCap = CreateDefaultSubobject<UStaticMeshComponent>("LeftCap");
	LeftCap->SetupAttachment(Root);

	Middle = CreateDefaultSubobject<UStaticMeshComponent>("Middle");
	Middle->SetupAttachment(Root);

	RightCap = CreateDefaultSubobject<UStaticMeshComponent>("RightCap");
	RightCap->SetupAttachment(Root);
}

void ADeckBoxes::InitBox(UBoxComponent* BoxCollision, USkeletalMeshComponent* DeckBox)
{
	BoxCollision->SetupAttachment(Root);
	BoxCollision->SetBoxExtent(FVector(5.f, 5.f, 6.f), false);
	BoxCollision->SetGenerateOverlapEvents(false);
	BoxCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxCollision->SetCollisionResponseToChannel(ECC_Card, ECR_Block);
	DeckBox->SetupAttachment(BoxCollision);
	
	DeckBoxCollisions.Add(BoxCollision);
	DeckBoxes.Add(DeckBox);
}

void ADeckBoxes::BeginPlay()
{
	Super::BeginPlay();

	for (USkeletalMeshComponent* DeckBox : DeckBoxes)
	{
		DeckBox->SetPosition(0.f, false);
	}
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
		if (DeckBoxCollisions.IsValidIndex(DeckBoxIndex))
		{
			DeckBoxCollisions[DeckBoxIndex]->SetRelativeLocation(DeckBoxLocations[DeckBoxIndex]);
		}
	}

	const FVector RightCapLocation = FVector(DefaultRightCapXLocation + AllHandCount * DeckBoxOffsetByHandCount, 0.f, 0.f);
	RightCap->SetRelativeLocation(RightCapLocation);

	const FVector MiddleCapLocation = FVector(AllHandCount * (DeckBoxOffsetByHandCount / 2.f), 0.f, 0.f);
	Middle->SetRelativeLocation(MiddleCapLocation);
}

void ADeckBoxes::GetDeckLocations(TArray<FVector>& DeckLocations) const
{
	for (const auto& DeckBoxCollision : DeckBoxCollisions)
	{
		if (DeckBoxCollision)
		{
			DeckLocations.Add(DeckBoxCollision->GetComponentLocation());
		}
	}
}

FVector ADeckBoxes::GetDeckLocation(const int32 DeckIndex) const
{
	if (DeckBoxCollisions.IsValidIndex(DeckIndex))
	{
		return DeckBoxCollisions[DeckIndex]->GetRelativeLocation();
	}
	return FVector::ZeroVector;
}

void ADeckBoxes::OpenDeckBox(UBoxComponent* InDeckBoxCollision)
{
	const int32 DeckIndex = DeckBoxCollisions.IndexOfByKey(InDeckBoxCollision);
	OpenDeckBox(DeckIndex);
}

void ADeckBoxes::CloseDeckBox(UBoxComponent* InDeckBoxCollision)
{
	const int32 DeckIndex = DeckBoxCollisions.IndexOfByKey(InDeckBoxCollision);
	CloseDeckBox(DeckIndex);
}

void ADeckBoxes::OpenDeckBox(const int32 DeckIndex)
{
	if (DeckBoxes.IsValidIndex(DeckIndex) && OpenedStates.IsValidIndex(DeckIndex))
	{
		if (!OpenedStates[DeckIndex])
		{
			if (UAnimSingleNodeInstance* SingleNode = DeckBoxes[DeckIndex]->GetSingleNodeInstance())
			{
				SingleNode->SetReverse(false);
				SingleNode->SetPlaying(true);
			}
			OpenedStates[DeckIndex] = true;
		}
	}
}

void ADeckBoxes::CloseDeckBox(const int32 DeckIndex)
{
	if (DeckBoxes.IsValidIndex(DeckIndex) && OpenedStates.IsValidIndex(DeckIndex))
	{
		if (OpenedStates[DeckIndex])
		{
			if (UAnimSingleNodeInstance* SingleNode = DeckBoxes[DeckIndex]->GetSingleNodeInstance())
			{
				SingleNode->SetReverse(true);
				SingleNode->SetPlaying(true);
			}
			OpenedStates[DeckIndex] = false;
		}
	}
}

void ADeckBoxes::OpenAllBoxes()
{
	for (int32 DeckIndex = 0; DeckIndex < DeckBoxes.Num(); ++DeckIndex)
	{
		OpenDeckBox(DeckIndex);
	}
}

void ADeckBoxes::CloseAllBoxes()
{
	for (int32 DeckIndex = 0; DeckIndex < DeckBoxes.Num(); ++DeckIndex)
	{
		CloseDeckBox(DeckIndex);
	}
}
