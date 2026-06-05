// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckBoxes.h"

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

	LeftCap = CreateDefaultSubobject<UStaticMeshComponent>("LeftCap");
	LeftCap->SetupAttachment(Root);

	Middle = CreateDefaultSubobject<UStaticMeshComponent>("Middle");
	Middle->SetupAttachment(Root);

	RightCap = CreateDefaultSubobject<UStaticMeshComponent>("RightCap");
	RightCap->SetupAttachment(Root);
}

