// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LetheWidgetController.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class ULetheAbilitySystemComponent;
class ULetheAttributeSet;

// Widget Controller의 멤버 변수 초기화를 간편화하는 구조체입니다.
USTRUCT()
struct FWidgetControllerParams
{
	GENERATED_BODY()

	FWidgetControllerParams() {}
	FWidgetControllerParams(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
	: PlayerController(PC), PlayerState(PS), AbilitySystemComponent(ASC), AttributeSet(AS) {}

	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;

	UPROPERTY()
	TObjectPtr<APlayerState> PlayerState;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;
};

// 프로젝트 특성상 플레이어와 캐릭터의 수가 1:4 매칭이기 때문에, WidgetController에서 ASC와 AS를 여러 개 관리해야 합니다.
// 따라서 배열 선언을 위해 구조체로 두 객체를 감싸 사용합니다.
USTRUCT(BlueprintType)
struct FAbilitySystemReference
{
	GENERATED_BODY()

	FAbilitySystemReference() {}
	FAbilitySystemReference(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
	: AbilitySystemComponent(ASC), AttributeSet(AS) {}

	UPROPERTY()
	TObjectPtr<ULetheAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<ULetheAttributeSet> AttributeSet;
};


/**
 * MVVM 패턴의 VM을 담당하는 Widget Controller입니다.
 * MVVM 패턴은 UI를 관리하는 패턴 중 GAS 기반의 프로젝트와 가장 잘 어울리는 패턴이라는 게 정설입니다.
 * HUD를 포함해 UI와 관련된 모든 객체(ASC, AttributeSet, PlayerController 등)이 모두 생성되었다고 판단되는 순간, HUD에 의해 생성되는 객체입니다.
 * 생성 이후 즉시 관련 객체들을 할당받으며, 그 객체들에게 콜백 함수를 바인드합니다.
 * 바인드를 마치면 모든 관련 Widget들에게 뿌려지게 되며, Widget들은 이를 변수로 할당하고 마찬가지로 콜백 함수를 바인드합니다.
 */
UCLASS(Abstract, NotBlueprintable)
class LETHE_API ULetheWidgetController : public UObject
{
	GENERATED_BODY()

public:
	void SetWidgetControllerParams(const FWidgetControllerParams& WidgetControllerParams);
	virtual void BindCallbacksToDependencies();

	// 객체 생성 직후 값 초기화와 함께 View에 표시해야 한다면 아래 함수를 활용할 수 있습니다.
	UFUNCTION(BlueprintCallable)
	virtual void BroadcastInitialValue();

	APlayerController* GetPC();
	APlayerState* GetPS();
	TArray<FAbilitySystemReference>* GetAbilitySystemReferences();
	
protected:
	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<APlayerController> PlayerController;

	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<APlayerState> PlayerState;

	// 해당 배열의 Element는 Character의 BeginPlay가 호출된 순서대로 추가됩니다.
	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TArray<FAbilitySystemReference> AbilitySystemReferences;
};
