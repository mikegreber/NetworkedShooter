// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SGameMode.generated.h"

namespace MatchState
{
	extern NETWORKEDSHOOTER_API const FName Cooldown; // Match duration has been reached, display winner and begin cooldown timer.
}


UCLASS()
class NETWORKEDSHOOTER_API ASGameMode : public AGameMode
{
	GENERATED_BODY()

public:

	ASGameMode();

	virtual void Tick(float DeltaSeconds) override;

	virtual void PlayerEliminated(class ASCharacter* EliminatedCharacter, AController* VictimController, AController* KillerController);

	virtual void RequestRespawn(class ACharacter* EliminatedCharacter, AController* EliminatedController);

	void PlayerLeftGame(class ASPlayerState* PlayerLeaving);
	

public:
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
    float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;
    	
	float LevelStartingTime = 0.f;

protected:
	virtual void BeginPlay() override;
	
	virtual void OnMatchStateSet() override;

private:
	float CountdownTime = 0.f;

public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }

};

