// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SEliminationAnnouncementWidget.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API USEliminationAnnouncementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* AnnouncementBox;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* AnnouncementText;
	
	void SetEliminationAnnouncementText(FString AttackerName, FString VictimName);
	

};
