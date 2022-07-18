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

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float CooldownTime = 10.f;
	
protected:
	
	UPROPERTY(EditAnywhere, Category = "Settings")
	FString MenuName;

public:
    	
	float LevelStartingTime = 0.f;

	bool bIsTeamsMatch = false;

private:
	
	float CountdownTime = 0.f;
	
public:

	ASGameMode();

	virtual void StartPlay() override;
	
	virtual void Tick(float DeltaSeconds) override;

	virtual void PlayerEliminated(class ASCharacter* EliminatedCharacter, AController* VictimController, AController* KillerController);

	virtual void RequestRespawn(class ACharacter* EliminatedCharacter, AController* EliminatedController);

	void PlayerLeftGame(class ASPlayerState* PlayerLeaving);

	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage);

protected:
	
	virtual void BeginPlay() override;
	
	virtual void OnMatchStateSet() override;

public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
	FORCEINLINE const FString& GetMenuName() const { return MenuName; };

};

