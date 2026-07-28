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

	OpenReasons.Reset();
	OpenReasons.Init(EDeckBoxOpenReason::None, PLAYER_CHARACTER_NUMBER);
	PreviousOpenReasons.Reset();
	PreviousOpenReasons.Init(EDeckBoxOpenReason::None, PLAYER_CHARACTER_NUMBER);
	
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

void ADeckBoxes::GetDeckLocations(TArray<FVector>& OutDeckLocations) const
{
	for (const auto& DeckBoxCollision : DeckBoxCollisions)
	{
		if (DeckBoxCollision)
		{
			OutDeckLocations.Add(DeckBoxCollision->GetComponentLocation());
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

int32 ADeckBoxes::GetDeckIndex(const UBoxComponent* InDeckBoxCollision) const
{
	return DeckBoxCollisions.IndexOfByKey(InDeckBoxCollision);
}

void ADeckBoxes::SetOpenReason(const UBoxComponent* InDeckBoxCollision, const EDeckBoxOpenReason InOpenReason, const bool bEnable)
{
	if (InOpenReason == EDeckBoxOpenReason::MouseHover)
	{
		// MouseHover는 유일하므로, 모든 Reason의 MouseHover 플래그를 0으로 변경합니다.
		SetAllOpenReason(EDeckBoxOpenReason::MouseHover, false, false);
	}
	if (InOpenReason == EDeckBoxOpenReason::Pinned)
	{
		// Pinned도 마찬가지입니다.
		SetAllOpenReason(EDeckBoxOpenReason::Pinned, false, false);
	}

	const int32 DeckIndex = GetDeckIndex(InDeckBoxCollision);
	if (DeckIndex != INDEX_NONE)
	{
		if (bEnable)
		{
			OpenReasons[DeckIndex] |= InOpenReason;
		}
		else
		{
			OpenReasons[DeckIndex] &= ~InOpenReason;
		}
	}
	ApplyDeckBoxesOpenState();
}

void ADeckBoxes::SetAllOpenReason(const EDeckBoxOpenReason InOpenReason, const bool bEnable, const bool bShouldApply)
{
	for (int32 DeckIndex = 0; DeckIndex < DeckBoxes.Num(); ++DeckIndex)
	{
		if (bEnable)
		{
			OpenReasons[DeckIndex] |= InOpenReason;
		}
		else
		{
			OpenReasons[DeckIndex] &= ~InOpenReason;
		}
	}

	if (bShouldApply)
	{
		ApplyDeckBoxesOpenState();
	}
}

void ADeckBoxes::ApplyDeckBoxesOpenState()
{
	bool bDirty = false;
	for (int32 ReasonIndex = 0; ReasonIndex < OpenReasons.Num(); ++ReasonIndex)
	{
		if (PreviousOpenReasons[ReasonIndex] != OpenReasons[ReasonIndex])
		{
			bDirty = true;
			break;
		}
	}

	if (!bDirty)
	{
		return;
	}
	
	PreviousOpenReasons = OpenReasons;
	
	for (int32 DeckIndex = 0; DeckIndex < DeckBoxes.Num(); ++DeckIndex)
	{
		const bool bShouldOpen = OpenReasons[DeckIndex] != EDeckBoxOpenReason::None;
		if (UAnimSingleNodeInstance* SingleNode = DeckBoxes[DeckIndex]->GetSingleNodeInstance())
		{
			SingleNode->SetReverse(!bShouldOpen);
			SingleNode->SetPlaying(true);
		}
	}
}
