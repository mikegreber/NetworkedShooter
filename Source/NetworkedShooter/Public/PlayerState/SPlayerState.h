// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatUpdated, int32, NewValue);

UCLASS()
class NETWORKEDSHOOTER_API ASPlayerState : public APlayerState
{
	GENERATED_BODY()
	
	UPROPERTY(ReplicatedUsing=OnRep_Kills)
	int32 Kills;
	
	UPROPERTY(ReplicatedUsing=OnRep_Deaths)
	int32 Deaths;

	UPROPERTY()
	class ASPlayerController* Controller;
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION()
	void OnPlayerHUDCreated(class ASHUD* HUD);
	virtual void SetOwner(AActor* NewOwner) override;
	
	// void UpdateHUD() const;
	
	void AddToKills(int32 KillsAmount);
	void UpdateKills() const;
	void AddToDeaths(int32 DefeatsAmount);
	void UpdateDeaths();
	FOnStatUpdated OnKillsUpdated;
	FOnStatUpdated OnDeathsUpdated;

	UFUNCTION()
	virtual void OnRep_Kills();

	UFUNCTION()
	virtual void OnRep_Deaths();

	FORCEINLINE int32 GetKills() const { return Kills; }
	FORCEINLINE int32 GetDeaths() const { return Deaths; }
protected:
	virtual void BeginPlay() override;
private:
	
};
