// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/SHUD_Lobby.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameMode/SGameMode.h"
#include "GameMode/SGameMode_Lobby.h"
#include "Library/ShooterGameplayStatics.h"

bool ASHUD_Lobby::Initialize()
{
	if (LobbyOverlayClass)
	{
		Overlay = CreateWidget<USOverlay_Lobby>(GetOwningPlayerController(), LobbyOverlayClass);
		Overlay->AddToViewport();
		ReadyButton = Overlay->ReadyButton;
		ReadyText = Overlay->ReadyText;
		PlayersReadyText = Overlay->PlayersReadyText;
		GameModeComboBox = Overlay->GameModeComboBox;
		MapComboBox = Overlay->MapComboBox;
		MapThumbnail = Overlay->MapThumbnail;
		
		CountdownText = Overlay->CountdownText;
		CountdownText->SetText(FText());
		
		if (!GameModeOptions.IsEmpty())
		{
			for (const TSubclassOf<ASGameMode>& Mode : GameModeOptions)
			{
				GameModeComboBox->AddOption(Mode->GetDefaultObject<ASGameMode>()->GetMenuName());
				UE_LOG(LogTemp, Warning, TEXT("%s"), *Mode->GetPathName())
				
			}
				
			GameModeComboBox->SetSelectedIndex(0);
			GameModeComboBox->SetSelectedOption(GameModeOptions[0]->GetDefaultObject<ASGameMode>()->GetMenuName());

			if (ASGameMode_Lobby* LobbyGameMode = GetWorld()->GetAuthGameMode<ASGameMode_Lobby>())
			{
				LobbyGameMode->SelectedGameMode = GameModeOptions[0];
			}
		}

		if (!MapOptions.IsEmpty())
		{
			for (const FMapInfo& MapInfo : MapOptions)
			{
				MapComboBox->AddOption(MapInfo.DisplayName);
			}
				
			MapComboBox->SetSelectedIndex(0);
			MapComboBox->SetSelectedOption(MapOptions[0].DisplayName);
			MapThumbnail->SetBrushFromTexture(MapOptions[0].Thumbnail);

			// will succeed on server only
			if (ASGameMode_Lobby* LobbyGameMode = GetWorld()->GetAuthGameMode<ASGameMode_Lobby>())
			{
				LobbyGameMode->SelectedMap = MapOptions[0].Map;
			}
		}
		
		return true;
	}

	return false;
}

void ASHUD_Lobby::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (Overlay) Overlay->RemoveFromParent();
}
