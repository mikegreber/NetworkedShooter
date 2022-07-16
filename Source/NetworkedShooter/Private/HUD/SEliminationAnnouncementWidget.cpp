// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/SEliminationAnnouncementWidget.h"

#include "Components/TextBlock.h"

void USEliminationAnnouncementWidget::SetEliminationAnnouncementText(FString AttackerName, FString VictimName)
{
	const FString EliminationAnnouncementText = FString::Printf(TEXT("%s Eliminated %s"), *AttackerName, *VictimName);
	if (AnnouncementText)
	{
		AnnouncementText->SetText(FText::FromString(EliminationAnnouncementText));
	}
}
