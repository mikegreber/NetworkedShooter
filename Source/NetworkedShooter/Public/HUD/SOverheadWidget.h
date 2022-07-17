// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOverheadWidget.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API USOverheadWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* DisplayText;

	void SetDisplayText(FString TextToDisplay, FColor Color = FColor(255,255,255));

	UFUNCTION(BlueprintCallable)
	void ShowPlayerNetRole(APawn* InPawn);

protected:
	
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld) override;
};
