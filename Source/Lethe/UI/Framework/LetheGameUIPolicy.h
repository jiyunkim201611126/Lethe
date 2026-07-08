// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "LetheGameUIPolicy.generated.h"

class ULethePrimaryGameLayout;
class ULetheUIManagerSubsystem;

/**
 * 
 */
UCLASS(Blueprintable, Within = LetheUIManagerSubsystem)
class LETHE_API ULetheGameUIPolicy : public UObject
{
	GENERATED_BODY()

public:
	static ULetheGameUIPolicy* GetGameUIPolicy(const UObject* WorldContextObject);

	virtual UWorld* GetWorld() const override;
	ULetheUIManagerSubsystem* GetOwningUIManager() const;
	ULethePrimaryGameLayout* GetOrCreateRootLayout(APlayerController* PlayerController);
	ULethePrimaryGameLayout* GetRootLayout() const { return RootLayout; }

protected:
	void AddLayoutToViewport(APlayerController* PlayerController, ULethePrimaryGameLayout* Layout);

	virtual void OnRootLayoutAddedToViewport(APlayerController* PlayerController, ULethePrimaryGameLayout* Layout);

	void CreateLayoutWidget(APlayerController* PlayerController);
	TSubclassOf<ULethePrimaryGameLayout> GetLayoutWidgetClass() const;

private:
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<ULethePrimaryGameLayout> LayoutClass;

	UPROPERTY(Transient)
	TObjectPtr<ULethePrimaryGameLayout> RootLayout;
};
