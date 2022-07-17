// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SGameState_Lobby.generated.h"

USTRUCT(BlueprintType)
struct FLobbyPlayer
{
	GENERATED_BODY()

	FLobbyPlayer() = default;
	FLobbyPlayer(const class ASPlayerState_Lobby* Player) : PlayerState(Player), bReady(false) {}
	
	UPROPERTY()
	const ASPlayerState_Lobby* PlayerState;

	UPROPERTY()
	bool bReady;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyPlayersChanged, const TArray<FLobbyPlayer>&, LobbyPlayers);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCountdownTimeUpdated, int32, CountdownTime);

UCLASS()
class NETWORKEDSHOOTER_API ASGameState_Lobby : public AGameStateBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Settings")
	float CountdownDuration = 10.f;
	
	UPROPERTY(ReplicatedUsing=OnRep_LobbyPlayers)
	TArray<FLobbyPlayer> LobbyPlayers;
	
	UPROPERTY(ReplicatedUsing=OnRep_CountdownTime)
	int32 CountdownTime = -1.f;

	FTimerHandle CountdownHandle;

public:
	
	FOnLobbyPlayersChanged OnLobbyPlayersChanged;
	FOnCountdownTimeUpdated OnCountdownTimeUpdated;

public:
	
	static int32 CountReadyPlayers(const TArray<FLobbyPlayer>& Players);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	UFUNCTION(Server, Reliable)
	void UpdatePlayerState(ASPlayerState_Lobby* PlayerState);

protected:

	// starts countdown
	void StartGame();

	// cancels countdown
	void CancelStartGame();

	// ticks down the countdown timer
	void OnCountDown();

	// calls GameMode->StartGame()
	void OnCountdownFinished();
	
	UFUNCTION()
	void OnRep_LobbyPlayers() const;
	
	UFUNCTION()
	void OnRep_CountdownTime() const;

public:
	
	FORCEINLINE const TArray<FLobbyPlayer>& GetLobbyPlayers() { return LobbyPlayers; };

};

