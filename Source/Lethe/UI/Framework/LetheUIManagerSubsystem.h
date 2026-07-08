// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/SoftObjectPtr.h"
#include "LetheUIManagerSubsystem.generated.h"

class ULetheGameUIPolicy;

/**
 * 게임 시작 시 정해진 GameUIPolicy 객체를 생성, 관리하는 매니저 클래스입니다.
 */
UCLASS(Config = Game)
class LETHE_API ULetheUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	const ULetheGameUIPolicy* GetCurrentUIPolicy() const { return CurrentPolicy; }
	ULetheGameUIPolicy* GetCurrentUIPolicy() { return CurrentPolicy; }

protected:
	void SwitchToPolicy(ULetheGameUIPolicy* InPolicy);

private:
	UPROPERTY(Transient)
	TObjectPtr<ULetheGameUIPolicy> CurrentPolicy;

	UPROPERTY(Config, EditAnywhere)
	TSoftClassPtr<ULetheGameUIPolicy> DefaultUIPolicyClass;
};
