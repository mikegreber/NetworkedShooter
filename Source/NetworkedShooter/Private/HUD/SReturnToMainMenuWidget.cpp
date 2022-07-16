// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/SReturnToMainMenuWidget.h"

#include "MultiplayerSessionsSubsystem.h"
#include "Character/SCharacter.h"
#include "Components/Button.h"
#include "GameFramework/GameModeBase.h"

void USReturnToMainMenuWidget::MenuSetup()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		FInputModeGameAndUI InputModeData;
		InputModeData.SetWidgetToFocus(TakeWidget());
		PlayerController->SetInputMode(InputModeData);
		PlayerController->SetShowMouseCursor(true);
	}

	if (ReturnButton && !ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.AddDynamic(this, &USReturnToMainMenuWidget::ReturnButtonClicked);
	}

	MultiplayerSessionsSubsystem = GetGameInstance()->GetSubsystem<UMultiplayerSessionsSubsystem>();
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->SubsystemOnDestroySessionComplete.AddUniqueDynamic(this, &USReturnToMainMenuWidget::OnDestroySession);
	}
}

bool USReturnToMainMenuWidget::Initialize()
{
	return Super::Initialize();
}

void USReturnToMainMenuWidget::OnDestroySession(bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		ReturnButton->SetIsEnabled(true);
		return;
	}
	
	if (AGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AGameModeBase>())
	{
		GameMode->ReturnToMainMenuHost();
	}
	else
	{
		if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		{
			PlayerController->ClientReturnToMainMenuWithTextReason(FText());
		}
	}
}




void USReturnToMainMenuWidget::MenuTeardown()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		const FInputModeGameOnly InputModeData;
		PlayerController->SetInputMode(InputModeData);
		PlayerController->SetShowMouseCursor(false);
	}

	if (ReturnButton && ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.RemoveDynamic(this, &USReturnToMainMenuWidget::ReturnButtonClicked);
	}

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->SubsystemOnDestroySessionComplete.RemoveDynamic(this, &USReturnToMainMenuWidget::OnDestroySession);
	}
}



void USReturnToMainMenuWidget::ReturnButtonClicked()
{
	ReturnButton->SetIsEnabled(false);

	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* FirstPlayerController = World->GetFirstPlayerController())
		{
			if (ASCharacter* Character = FirstPlayerController->GetPawn<ASCharacter>())
			{
				Character->ServerLeaveGame();
				Character->OnLeftGame.AddDynamic(this, &USReturnToMainMenuWidget::OnPlayerLeftGame);
			}
			else
			{
				ReturnButton->SetIsEnabled(true);
			}
		}
	}
	
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->DestroySession();
	}
}


void USReturnToMainMenuWidget::OnPlayerLeftGame()
{	
	
}