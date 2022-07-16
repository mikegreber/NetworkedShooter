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
	class UVerticalBox* AnnouncementBox;

	UPROPERTY(EditAnywhere, Category="Announcement")
	float EliminationAnnouncementTime = 4.f;

	UPROPERTY(EditAnywhere, Category="Announcement")
	TSubclassOf<class USEliminationAnnouncementText> AnnouncementTextClass;
	
	void SetEliminationAnnouncementText(FString AttackerName, FString VictimName);
	
	void EliminationAnnouncementTimerFinished(USEliminationAnnouncementText* AnnouncementTextWidget);
};
