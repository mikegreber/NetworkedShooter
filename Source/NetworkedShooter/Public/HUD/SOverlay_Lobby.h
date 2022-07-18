// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOverlay_Lobby.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API USOverlay_Lobby : public UUserWidget
{
	GENERATED_BODY()

	friend class ASPlayerController_Lobby;
	friend class ASHUD_Lobby;
	
protected:
	
	UPROPERTY(meta = (BindWidget))
	class UButton* ReadyButton;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ReadyText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayersReadyText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CountdownText;
	
	UPROPERTY(meta = (BindWidget))
	class UComboBoxString* GameModeComboBox;
	
	UPROPERTY(meta = (BindWidget))
	UComboBoxString* MapComboBox;

	UPROPERTY(meta = (BindWidget))
	class UImage* MapThumbnail;
	
};
