// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/SGameMode.h"
#include "Character/SCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "GameState/SGameState.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerController/SPlayerController.h"
#include "PlayerState/SPlayerState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}


ASGameMode::ASGameMode()
{
	bDelayedStart = true;
}

void ASGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ASGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASPlayerController* PlayerController = Cast<ASPlayerController>(*It))
		{
			PlayerController->OnMatchStateSet(MatchState, bIsTeamsMatch);
		}
	}
}

void ASGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime + LevelStartingTime - GetWorld()->GetTimeSeconds();
		if (CountdownTime <= 0.f) StartMatch();
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime + LevelStartingTime - GetWorld()->GetTimeSeconds();
		if (CountdownTime <= 0.f) SetMatchState(MatchState::Cooldown);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = WarmupTime + MatchTime + CooldownTime +  LevelStartingTime - GetWorld()->GetTimeSeconds();
		if (CountdownTime <= 0.f) RestartGame();
	}
}

void ASGameMode::PlayerEliminated(ASCharacter* EliminatedCharacter, AController* VictimController, AController* KillerController)
{
	ASPlayerState* KillerPlayerState = KillerController ? Cast<ASPlayerState>(KillerController->PlayerState) : nullptr;
	ASPlayerState* VictimPlayerState = VictimController ? Cast<ASPlayerState>(VictimController->PlayerState) : nullptr;

	ASGameState* ShooterGameState = GetGameState<ASGameState>();

	if (KillerPlayerState && KillerPlayerState != VictimPlayerState && ShooterGameState)
	{
		TArray<ASPlayerState*> PlayersCurrentlyInTheLead;
		for (ASPlayerState* LeadPlayer : ShooterGameState->TopScoringPlayers)
		{
			PlayersCurrentlyInTheLead.Add(LeadPlayer);
		}
		
		KillerPlayerState->AddToKills(1);
		ShooterGameState->UpdateTopScore(KillerPlayerState);

		switch (KillerPlayerState->GetTeam())
		{
		case ETeam::ET_BlueTeam:
			{
				ShooterGameState->BlueTeamScores();
				break;
			}
		case ETeam::ET_RedTeam:
			{
				ShooterGameState->RedTeamScores();
				break;
			}
		default: break;
		}


		if (ShooterGameState->TopScoringPlayers.Contains(KillerPlayerState))
		{
			if (ASCharacter* Leader = KillerPlayerState->GetPawn<ASCharacter>())
			{
				Leader->MulticastGainedTheLead();
			}
		}
		
		for (ASPlayerState* PlayerCurrentlyInTheLead : PlayersCurrentlyInTheLead)
		{
			if (!ShooterGameState->TopScoringPlayers.Contains(PlayerCurrentlyInTheLead))
			{
				if (ASCharacter* Loser = PlayerCurrentlyInTheLead->GetPawn<ASCharacter>())
				{
					Loser->MulticastLostTheLead();
				}
			}
		}
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDeaths(1);
	}
	
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Eliminated(false);
	}
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ASPlayerController* Player = Cast<ASPlayerController>(*It);
		if (Player && KillerPlayerState && VictimPlayerState)
		{
			Player->BroadcastElimination(KillerPlayerState, VictimPlayerState);
		}
	}
}

void ASGameMode::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController)
{
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Reset();
		EliminatedCharacter->Destroy();
	}
	if (EliminatedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
		const int32 Selection = FMath::RandRange(0, PlayerStarts.Num()-1);
		RestartPlayerAtPlayerStart(EliminatedController, PlayerStarts[Selection]);
	}
}

void ASGameMode::PlayerLeftGame(ASPlayerState* PlayerLeaving)
{
	if (!PlayerLeaving) return;
	
	ASGameState* ShooterGameState = GetGameState<ASGameState>();
	if (ShooterGameState && ShooterGameState->TopScoringPlayers.Contains(PlayerLeaving))
	{
		ShooterGameState->TopScoringPlayers.Remove(PlayerLeaving);
	}

	if (ASCharacter* CharacterLeaving = PlayerLeaving->GetPawn<ASCharacter>())
	{
		CharacterLeaving->Eliminated(true);
	}
}

float ASGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	return BaseDamage;
}
