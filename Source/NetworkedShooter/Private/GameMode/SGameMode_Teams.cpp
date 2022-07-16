// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/SGameMode_Teams.h"

#include "GameState/SGameState.h"
#include "PlayerController/SPlayerController.h"
#include "PlayerState/SPlayerState.h"

ASGameMode_Teams::ASGameMode_Teams()
{
	bIsTeamsMatch = true;
}


void ASGameMode_Teams::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (ASGameState* ShooterGameState = GetGameState<ASGameState>())
	{
		if (ASPlayerState* PlayerState = NewPlayer->GetPlayerState<ASPlayerState>())
		{
			SetPlayerTeam(ShooterGameState, PlayerState);
		}
	}
}

void ASGameMode_Teams::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (ASGameState* ShooterGameState = GetGameState<ASGameState>())
	{
		if (ASPlayerState* PlayerState = Exiting->GetPlayerState<ASPlayerState>())
		{
			if (ShooterGameState->RedTeam.Contains(PlayerState))
			{
				ShooterGameState->RedTeam.Remove(PlayerState);
			}
			if (ShooterGameState->BlueTeam.Contains(PlayerState))
			{
				ShooterGameState->BlueTeam.Remove(PlayerState);
			}
		}
	}
}

float ASGameMode_Teams::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	const ASPlayerState* AttackerPlayerState = Attacker->GetPlayerState<ASPlayerState>();
	const ASPlayerState* VictimPlayerState = Victim->GetPlayerState<ASPlayerState>();
	
	if (!AttackerPlayerState || !VictimPlayerState) return BaseDamage;

	if (AttackerPlayerState == VictimPlayerState) return BaseDamage;

	if (AttackerPlayerState->GetTeam() == VictimPlayerState->GetTeam()) return 0.f;

	return BaseDamage;
}

void ASGameMode_Teams::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	if (ASGameState* ShooterGameState = GetGameState<ASGameState>())
	{
		for (TObjectPtr<APlayerState> PS : ShooterGameState->PlayerArray)
		{
			if (ASPlayerState* PlayerState = Cast<ASPlayerState>(PS.Get()))
			{
				SetPlayerTeam(ShooterGameState, PlayerState);
			}
		}
	}
}


void ASGameMode_Teams::SetPlayerTeam(ASGameState* ShooterGameState, ASPlayerState* PlayerState)
{
	if (PlayerState->GetTeam() == ETeam::ET_NoTeam)
	{
		if (ShooterGameState->BlueTeam.Num() >= ShooterGameState->RedTeam.Num())
		{
			ShooterGameState->RedTeam.AddUnique(PlayerState);
			PlayerState->SetTeam(ETeam::ET_RedTeam);
		}
		else
		{
			ShooterGameState->BlueTeam.AddUnique(PlayerState);
			PlayerState->SetTeam(ETeam::ET_BlueTeam);
		}
	}
}
