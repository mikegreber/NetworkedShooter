// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SGameState.generated.h"

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

private:
	float TopScore = 0.f;
};
