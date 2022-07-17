// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameState/SGameState_Lobby.h"
#include "SPlayerController_Lobby.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API ASPlayerController_Lobby : public APlayerController
{
	GENERATED_BODY()

	UPROPERTY() class ASHUD_Lobby* HUD;
	UPROPERTY() class ASPlayerState_Lobby* LobbyPlayerState;
	UPROPERTY() class ASGameState_Lobby* LobbyGameState;

	// UPROPERTY(EditAnywhere, Category = "")
	// TArray<TSubclassOf<class ASGameMode>> GameModeOptions;
	
	bool bCountDownStarted;
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRep_PlayerState() override;
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;

	virtual void ReceivedPlayer() override;
	
	UFUNCTION(Client, Reliable)
	void ClientNotifyGameModeSelectionChanged(int32 SelectedIndex);

	UFUNCTION(Client, Reliable)
	void ClientNotifyMapSelectionChanged(int32 SelectedIndex);

protected:
	
	virtual void BeginPlay() override;

private:
	
	void InitializeHUD();
	
	void SetLobbyGameState(AGameStateBase* NewGameState);
	
	void SetLobbyPlayerState(APlayerState* NewPlayerState);

	UFUNCTION()
	void OnCountdownTimeUpdated(int32 CountdownTime);
	
	UFUNCTION()
	void OnLobbyPlayersChanged(const TArray<FLobbyPlayer>& LobbyPlayers);

	UFUNCTION()
	void OnIsReadyChanged(bool bBReady);
	
	UFUNCTION()
	void OnGameModeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnMapSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	
	UFUNCTION()
	void OnReadyButtonClicked();
	
};



