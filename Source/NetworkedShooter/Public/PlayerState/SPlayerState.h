// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Types/Team.h"
#include "SPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatUpdated, int32, NewValue);

UCLASS()
class NETWORKEDSHOOTER_API ASPlayerState : public APlayerState
{
	GENERATED_BODY()

	friend class ASPlayerController;
	
	UPROPERTY(ReplicatedUsing=OnRep_Kills)
	int32 Kills;
	
	UPROPERTY(ReplicatedUsing=OnRep_Deaths)
	int32 Deaths;

	UPROPERTY()
	class ASPlayerController* PlayerController;

	UPROPERTY(Replicated)
	bool bHasHighPing;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetOwner(AActor* NewOwner) override;
	
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
	FORCEINLINE bool HasHighPing() const { return bHasHighPing; }
	FORCEINLINE ETeam GetTeam() const { return Team; }
	void SetTeam(ETeam NewTeam);
	
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_Team();
	
private:

	UPROPERTY(ReplicatedUsing=OnRep_Team)
	ETeam Team = ETeam::ET_NoTeam;
	
};
