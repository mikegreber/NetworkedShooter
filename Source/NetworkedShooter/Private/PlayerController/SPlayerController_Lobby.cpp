// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/SPlayerController_Lobby.h"

#include "Character/SCharacter_Lobby.h"
#include "Components/Button.h"
#include "Components/ComboBoxKey.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "GameMode/SGameMode.h"
#include "GameMode/SGameMode_Lobby.h"
#include "GameState/SGameState_Lobby.h"
#include "HUD/SHUD_Lobby.h"
#include "Net/UnrealNetwork.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "PlayerState/SPlayerState_Lobby.h"


void ASPlayerController_Lobby::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ASPlayerController_Lobby::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	SetLobbyPlayerState(PlayerState);
}

void ASPlayerController_Lobby::InitPlayerState()
{
	Super::InitPlayerState();
	SetLobbyPlayerState(PlayerState);
}

void ASPlayerController_Lobby::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	LobbyPlayerState = nullptr;
}

void ASPlayerController_Lobby::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	SetLobbyGameState(GetWorld()->GetGameState());
	InitializeHUD();
}

void ASPlayerController_Lobby::ClientNotifyGameModeSelectionChanged_Implementation(int32 SelectedIndex)
{
	if (IsLocalController())
	{
		HUD->GameModeComboBox->SetSelectedIndex(SelectedIndex);
		HUD->GameModeComboBox->SetSelectedOption(HUD->GameModeOptions[SelectedIndex]->GetDefaultObject<ASGameMode>()->GetMenuName());

		if (HasAuthority())
		{
			if (ASGameMode_Lobby* LobbyGameMode = GetWorld()->GetAuthGameMode<ASGameMode_Lobby>())
			{
				LobbyGameMode->SelectedGameMode = HUD->GameModeOptions[SelectedIndex];
			}
		}
	}
}

void ASPlayerController_Lobby::ClientNotifyMapSelectionChanged_Implementation(int32 SelectedIndex)
{
	if (IsLocalController())
	{
		HUD->MapComboBox->SetSelectedIndex(SelectedIndex);
		HUD->MapComboBox->SetSelectedOption(HUD->MapOptions[SelectedIndex].PrimaryAssetName.ToString());

		if (HasAuthority())
		{
			if (ASGameMode_Lobby* LobbyGameMode = GetWorld()->GetAuthGameMode<ASGameMode_Lobby>())
			{
				LobbyGameMode->SelectedMap = HUD->MapOptions[SelectedIndex];
			}
		}
	}
}

void ASPlayerController_Lobby::SetLobbyPlayerState(APlayerState* NewPlayerState)
{
	ASPlayerState_Lobby* NewLobbyPlayerState = Cast<ASPlayerState_Lobby>(NewPlayerState);
	if (NewLobbyPlayerState && LobbyPlayerState != NewLobbyPlayerState)
	{
		LobbyPlayerState = NewLobbyPlayerState;
		InitializeHUD();
		if (ASCharacter_Lobby* LobbyCharacter = LobbyPlayerState->GetPawn<ASCharacter_Lobby>())
		{
			LobbyCharacter->SetTopOverheadText(LobbyPlayerState->GetPlayerName());
		}
	}
}

void ASPlayerController_Lobby::OnCountdownTimeUpdated(int32 CountdownTime)
{
	if (CountdownTime >= 0)
	{
		HUD->CountdownText->SetText(FText::FromString(FString::FromInt(CountdownTime)));
	}
	else
	{
		HUD->CountdownText->SetText(FText());
	}
	
}

void ASPlayerController_Lobby::SetLobbyGameState(AGameStateBase* NewGameState)
{
	LobbyGameState = Cast<ASGameState_Lobby>(NewGameState);
	UE_LOG(LogTemp, Warning, TEXT("%s %s %d"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR, LobbyGameState != nullptr);

	if (LobbyGameState)
	{
		InitializeHUD();
	}
	else
	{
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(
			Handle,
			FTimerDelegate::CreateUObject(this, &ASPlayerController_Lobby::SetLobbyGameState, GetWorld()->GetGameState()),
			0.5f,
			false
		);
	}
}

void ASPlayerController_Lobby::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		SetLobbyGameState(GetWorld()->GetGameState());
		const FInputModeUIOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
}

void ASPlayerController_Lobby::InitializeHUD()
{
	if (!HUD && LobbyGameState && LobbyPlayerState)
	{
		HUD = GetHUD<ASHUD_Lobby>();
		if (HUD && HUD->Initialize())
		{
			LobbyGameState->OnLobbyPlayersChanged.AddDynamic(this, &ASPlayerController_Lobby::OnLobbyPlayersChanged);
			OnLobbyPlayersChanged(LobbyGameState->GetLobbyPlayers());

			LobbyGameState->OnCountdownTimeUpdated.AddDynamic(this, &ASPlayerController_Lobby::OnCountdownTimeUpdated);

			LobbyPlayerState->OnIsReadyChanged.AddDynamic(this, &ASPlayerController_Lobby::OnIsReadyChanged);
			OnIsReadyChanged(LobbyPlayerState->IsReady());
			
			HUD->ReadyButton->OnClicked.AddDynamic(this, &ASPlayerController_Lobby::OnReadyButtonClicked);

			if (HasAuthority() && IsLocalController())
			{
				HUD->GameModeComboBox->OnSelectionChanged.AddDynamic(this, &ASPlayerController_Lobby::OnGameModeSelectionChanged);
				HUD->MapComboBox->OnSelectionChanged.AddDynamic(this, &ASPlayerController_Lobby::OnMapSelectionChanged);
			}
			else
			{
				HUD->GameModeComboBox->SetIsEnabled(false);
				HUD->MapComboBox->SetIsEnabled(false);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s %s Failed: %d %d"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR, LobbyGameState != nullptr, LobbyPlayerState != nullptr);
	}
}

void ASPlayerController_Lobby::OnLobbyPlayersChanged(const TArray<FLobbyPlayer>& LobbyPlayers)
{
	if (HUD) HUD->PlayersReadyText->SetText(FText::FromString(
		FString::Printf(TEXT("%d / %d"),
			ASGameState_Lobby::CountReadyPlayers(LobbyPlayers),
			LobbyPlayers.Num())
		)
	);
}

void ASPlayerController_Lobby::OnIsReadyChanged(bool bReady)
{
	HUD->ReadyText->SetText(FText::FromString(bReady ? "Cancel" : "Ready"));
}

void ASPlayerController_Lobby::OnGameModeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (ASGameMode_Lobby* LobbyGameMode = GetWorld()->GetAuthGameMode<ASGameMode_Lobby>())
	{
		LobbyGameMode->NotifyGameModeSelectionChanged(HUD->GameModeComboBox->GetSelectedIndex());	
	}
}

void ASPlayerController_Lobby::OnMapSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (ASGameMode_Lobby* LobbyGameMode = GetWorld()->GetAuthGameMode<ASGameMode_Lobby>())
	{
		LobbyGameMode->NotifyMapSelectionChanged(HUD->MapComboBox->GetSelectedIndex());	
	}
}

void ASPlayerController_Lobby::OnReadyButtonClicked()
{
	if (LobbyPlayerState)
	{
		LobbyPlayerState->ToggleReady();
	}
}


