// Fill out your copyright notice in the Description page of Project Settings.


#include "GameState/SGameState_Lobby.h"
#include "GameMode/SGameMode_Lobby.h"
#include "Net/UnrealNetwork.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "PlayerState/SPlayerState_Lobby.h"

int32 ASGameState_Lobby::CountReadyPlayers(const TArray<FLobbyPlayer>& Players)
{	
	int32 ReadyCount = 0;
	for (const FLobbyPlayer LobbyPlayer : Players)
	{
		if (LobbyPlayer.bReady) ++ReadyCount;
	}
	return ReadyCount;
}

void ASGameState_Lobby::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ASGameState_Lobby, LobbyPlayers, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME(ASGameState_Lobby, CountdownTime);
}

void ASGameState_Lobby::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	if (HasAuthority())
	{
		const ASPlayerState_Lobby* LobbyPlayerState = Cast<ASPlayerState_Lobby>(PlayerState);
		LobbyPlayers.Add(LobbyPlayerState);
		OnRep_LobbyPlayers();
	}
}

void ASGameState_Lobby::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	if (HasAuthority())
	{
		for (int32 i = 0; i < LobbyPlayers.Num(); ++i)
		{
			if (LobbyPlayers[i].PlayerState == PlayerState)
			{
				LobbyPlayers.RemoveAt(i);
				break;
			}
		}
		OnRep_LobbyPlayers();
	}
}

void ASGameState_Lobby::UpdatePlayerState_Implementation(ASPlayerState_Lobby* PlayerState)
{
	int32 ReadyCount = 0;
	for (FLobbyPlayer& LobbyPlayer : LobbyPlayers)
	{
		if (LobbyPlayer.PlayerState == PlayerState)
		{
			LobbyPlayer.bReady = PlayerState->IsReady();
		}
		if (LobbyPlayer.bReady) ++ReadyCount;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("%s %s ReadyCount: %d"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR, ReadyCount)
	
	OnRep_LobbyPlayers();

	if (ReadyCount == LobbyPlayers.Num())
	{
		StartGame();
	}
	else if (CountdownHandle.IsValid())
	{
		CancelStartGame();
	}
}

void ASGameState_Lobby::StartGame()
{
	UE_LOG(LogTemp, Warning, TEXT("%s %s Starting Countdown"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR)
	CountdownTime = CountdownDuration;
	OnRep_CountdownTime();
	
	GetWorldTimerManager().SetTimer(
		CountdownHandle,
		this,
		&ASGameState_Lobby::OnCountDown,
		1.f,
		true
	);
}

void ASGameState_Lobby::OnCountDown()
{
	CountdownTime -= 1;
	OnRep_CountdownTime();
	
	if (CountdownTime == 0)
	{
		OnCountdownFinished();
	}
}

void ASGameState_Lobby::OnCountdownFinished()
{
	GetWorldTimerManager().ClearTimer(CountdownHandle);
	if (ASGameMode_Lobby* LobbyGameMode = GetWorld()->GetAuthGameMode<ASGameMode_Lobby>())
	{
		LobbyGameMode->StartGame();
	}
}

void ASGameState_Lobby::CancelStartGame()
{
	CountdownTime = -1.f;
	OnRep_CountdownTime();
	
	GetWorldTimerManager().ClearTimer(CountdownHandle);
}

void ASGameState_Lobby::OnRep_CountdownTime() const
{
	OnCountdownTimeUpdated.Broadcast(CountdownTime);
}

void ASGameState_Lobby::OnRep_LobbyPlayers() const
{
	OnLobbyPlayersChanged.Broadcast(LobbyPlayers);
}
