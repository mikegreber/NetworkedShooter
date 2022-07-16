// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/SGameMode.h"
#include "GameState/SGameState.h"
#include "SGameMode_Teams.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API ASGameMode_Teams : public ASGameMode
{
	GENERATED_BODY()

public:
	ASGameMode_Teams();

	void SetPlayerTeam(ASGameState* ShooterGameState, ASPlayerState* PlayerState);
	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void Logout(AController* Exiting) override;

	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) override;
protected:
	
	virtual void HandleMatchHasStarted() override;

	
};
