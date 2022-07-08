// Fill out your copyright notice in the Description page of Project Settings.


#include "GameState/SGameState.h"

#include "Net/UnrealNetwork.h"
#include "PlayerState/SPlayerState.h"

void ASGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASGameState, TopScoringPlayers);
}

void ASGameState::UpdateTopScore(ASPlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.IsEmpty())
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetKills();
	}
	else if (ScoringPlayer->GetKills() == TopScore)
	{
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetKills() > TopScore)
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetKills();
	}
}
