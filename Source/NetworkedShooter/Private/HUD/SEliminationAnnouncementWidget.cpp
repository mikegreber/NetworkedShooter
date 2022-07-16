// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/SEliminationAnnouncementWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "HUD/SEliminationAnnouncementText.h"

void USEliminationAnnouncementWidget::SetEliminationAnnouncementText(FString AttackerName, FString VictimName)
{
	const FString EliminationAnnouncementString= FString::Printf(TEXT("%s Eliminated %s!"), *AttackerName, *VictimName);
	if (AnnouncementTextClass)
	{
		USEliminationAnnouncementText* AnnouncementTextWidget = CreateWidget<USEliminationAnnouncementText>(this, AnnouncementTextClass);
		AnnouncementTextWidget->TextBlock->SetText(FText::FromString(EliminationAnnouncementString));
		AnnouncementBox->InsertChildAt(AnnouncementBox->GetChildrenCount(), AnnouncementTextWidget);

		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(
			Handle,
			FTimerDelegate::CreateUObject(this, &USEliminationAnnouncementWidget::EliminationAnnouncementTimerFinished, AnnouncementTextWidget),
			EliminationAnnouncementTime,
			false
		);
	}
}

void USEliminationAnnouncementWidget::EliminationAnnouncementTimerFinished(USEliminationAnnouncementText* AnnouncementTextWidget)
{
	if (AnnouncementTextWidget)
	{
		AnnouncementTextWidget->RemoveFromParent();
	}
	
}
