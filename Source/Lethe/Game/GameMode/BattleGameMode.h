// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheGameMode.h"
#include "Lethe/Data/Stage/StageData.h"
#include "BattleGameMode.generated.h"

class ALetheCharacterBase;
class UCharacterDefinitionData;
class UPrimaryDataAsset;
class URoomRoleAssignmentRuleData;
struct FRoomRolePlacementCandidate;
struct FRoomRoleSelectionContext;

UCLASS()
class LETHE_API ABattleGameMode : public ALetheGameMode
{
	GENERATED_BODY()

public:
	virtual void RestartPlayer(AController* NewPlayer) override;

	AController* GetController() const;

private:
	void OnCharacterDefinitionDataLoaded(const TArray<UPrimaryDataAsset*>& CharacterDefinitions) const;

	void InitRoomRoles(const TArray<UPrimaryDataAsset*>& CharacterDefinitions = {}) const;

	/** 추가적인 조건을 요구하는 Role에 대해 처리합니다. */
	const FRoomRolePlacementCandidate* SelectRoomRoleCandidate(const UObject* WorldContextObject, const URoomRoleAssignmentRuleData* RoomRoleAssignmentRuleData, const TArray<FRoomRolePlacementCandidate>& Candidates, const FRoomRoleSelectionContext& SelectionContext) const;

	const FRoomRolePlacementCandidate* SelectLargestRoom(const TArray<FRoomRolePlacementCandidate>& Candidates) const;
	const FRoomRolePlacementCandidate* SelectFarthestRoom(const UObject* WorldContextObject, const TArray<FRoomRolePlacementCandidate>& Candidates, const int32 StartRoomId) const;
	const FRoomRolePlacementCandidate* SelectRandomRoom(const TArray<FRoomRolePlacementCandidate>& Candidates) const;

protected:
	UPROPERTY(EditDefaultsOnly)
	EStageType StageType = EStageType::Forest;
	
	/** 테스트를 위해 시작하자마자 전투에 즉시 돌입하기 편리하도록 선언된 변수입니다. */
	UPROPERTY(EditDefaultsOnly)
	bool bSpawnEnemyNearly = false;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ALetheCharacterBase> TestEnemyClass;

private:
	TWeakObjectPtr<AController> Controller;
};
