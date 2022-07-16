// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamUpdateScore, int32, NewScore);
/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API ASGameState : public AGameState
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void UpdateTopScore(class ASPlayerState* ScoringPlayer);
	
	UPROPERTY(Replicated)
	TArray<ASPlayerState*> TopScoringPlayers;

	void RedTeamScores();
	void BlueTeamScores();
	
	TArray<ASPlayerState*> RedTeam;
	TArray<ASPlayerState*> BlueTeam;

	UPROPERTY(ReplicatedUsing=OnRep_RedTeamScore)
	int32 RedTeamScore = 0;
	
	UPROPERTY(ReplicatedUsing=OnRep_BlueTeamScore)
	int32 BlueTeamScore = 0;

	FOnTeamUpdateScore OnRedTeamUpdateScore;
	FOnTeamUpdateScore OnBlueTeamUpdateScore;

	UFUNCTION()
	void OnRep_RedTeamScore();

	UFUNCTION()
	void OnRep_BlueTeamScore();

private:
	float TopScore = 0.f;
};
