// Copyright JETBLU, Inc. All Rights Reserved.

#include "PCGPointGenerator.h"

//PCG Point 및 Attribute 생성 관련
#include "Data/PCGPointArrayData.h"
#include "Metadata/PCGMetadata.h"
#include "UObject/SoftObjectPath.h"

//PCG 데이터 기반 DA 생성 관련
#if WITH_EDITOR
#include "PCGDataAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#endif

UPCGPointArrayData* PCGPointGenerator::GeneratePCGPoint(const TMap<FCubeCoord, FTileData>& TileDataMap, const TMap<FCubeCoord, TArray<FSoftObjectPath>>& TileMeshArray)
{
	UPCGPointArrayData* TilePoints = NewObject<UPCGPointArrayData>();
	
	//Point 개수 사전 정의
	TilePoints->SetNumPoints(TileDataMap.Num());
	
	//Point의 Transform 조작을 위한 배열
	TPCGValueRange<FTransform> TransformRange = TilePoints->GetTransformValueRange();
	
	//Attribute 컨트롤을 위한 포인터 선언
	UPCGMetadata* Metadata = TilePoints->MutableMetadata();
	TPCGValueRange<int64> MetadataEntryRange = TilePoints->GetMetadataEntryValueRange();
	
	FString AttrName[] = {TEXT("TileMainMesh"), TEXT("TileSide_LT"), TEXT("TileSide_LM"), TEXT("TileSide_LB"), TEXT("TileSide_RB"), TEXT("TileSide_RM"), TEXT("TileSide_RT")};
	FPCGMetadataAttribute<FSoftObjectPath>* AttrMesh[7];
	
	for (int32 i = 0; i < 7; ++i)
	{
		AttrMesh[i] = Metadata->FindOrCreateAttribute(AttrName[i], FSoftObjectPath(), false, true);
	}
	
	int32 Index = 0;
	
	for (auto& Pair : TileDataMap)
	{
		TransformRange[Index] = FTransform(FCubeCoord::CubeCoordToWorldCoord(Pair.Key, Pair.Value.Floor));
		const PCGMetadataEntryKey EntryKey = Metadata->AddEntry();
		MetadataEntryRange[Index] = EntryKey;
		
		const TArray<FSoftObjectPath>* MeshArray = TileMeshArray.Find(Pair.Key);
		
		for (int32 i = 0; i < 7; ++i)
		{
			AttrMesh[i]->SetValue(EntryKey, (*MeshArray)[i]);
		}
		++Index;
	}
	
	return TilePoints;
}

void PCGPointGenerator::BakePCGDataAsset(UPCGPointArrayData* TilePoints)
{
	#if WITH_EDITOR
	const FString AssetName = TEXT("DA_PCGTilePoints");
	const FString PackageName = FString(TEXT("/Game/Blueprint/PCG")) / AssetName;
	
	UPackage* Package = CreatePackage(*PackageName);
	UPCGDataAsset* DataAsset = NewObject<UPCGDataAsset>(Package, UPCGDataAsset::StaticClass(), *AssetName, RF_Public | RF_Standalone);
	
	//데이터 에셋에 Point 데이터 저장
	FPCGTaggedData Tagged;
	Tagged.Data = TilePoints;
	DataAsset->Data.TaggedData.Add(Tagged);
	DataAsset->bExposeToLibrary = true;
	TilePoints->Rename(nullptr, DataAsset); //Outer 대상을 DataAsset으로 재지정
	
	//콘텐츠 브라우저에 인지시키기
	FAssetRegistryModule::AssetCreated(DataAsset);
	DataAsset->MarkPackageDirty();
	
	//디스크에 데이터 저장
	const FString FilePath = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	//const bool bSaved = UPackage::SavePackage(Package, DataAsset, *FilePath, SaveArgs);
	//check(bSaved);
	#endif
}

