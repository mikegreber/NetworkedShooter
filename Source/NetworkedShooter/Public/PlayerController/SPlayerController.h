// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHUDCreated);

UCLASS()
class NETWORKEDSHOOTER_API ASPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	ASPlayerController();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void InitPlayerState() override;
	virtual void OnRep_PlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_Pawn() override;

	virtual void Tick(float DeltaSeconds) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	bool HasLocalAuthority() const;

	void OnMatchStateSet(FName State);

	UFUNCTION()
	void SetHUDHealth(float Health, float MaxHealth);

	UFUNCTION()
	void SetHUDShield(float Shield, float MaxShield);
	
	UFUNCTION()
	void SetHUDKills(int32 Score);

	UFUNCTION()
	void SetHUDDeaths(int32 Deaths);

	UFUNCTION()
	void SetHUDWeaponAmmo(int32 Ammo);

	UFUNCTION()
	void SetHUDCarriedAmmo(int32 Ammo);

	UFUNCTION()
	void SetHUDGrenades(int32 Grenades);

protected:
	
	virtual void BeginPlay() override;

	virtual void OnPossess(APawn* NewPawn) override;
	virtual void OnUnPossess() override;

	void HandleMatchHasStarted();
	void HandleCooldown();

	UFUNCTION()
	void PlayerCharacterHUDInit();
	
	UFUNCTION()
	void PlayerStateHUDInit();
	
	// synced with server world clock
	float GetServerTime() const;

	void HighPingWarning();
	void StopHighPingWarning();
	
private:
	void InitGameStateAndTick();

	void SetHUDTime();

	UFUNCTION()
	void SetHUDMatchCountdown(float CountdownTime) const;
	void SetHUDAnnouncementCountdown(float CountdownTime);
	
	UPROPERTY()
	class ASHUD* HUD;

	UPROPERTY()
	AGameStateBase* GameState;

	UPROPERTY()
	class ASGameMode* GameMode;

	float LevelStartingTime = 0.f;
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt;
	FOnHUDCreated OnHUDInitialized;
	FDelegateHandle GameStateSetDelegateHandle;
	FDelegateHandle OnNewPawnDelegateHandle;

	UPROPERTY(ReplicatedUsing=OnRep_MatchState)
	FName MatchState;

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(FName StateOfMatch, float Warmup, float Match, float StartingTime, float Cooldown);

	UFUNCTION()
	void OnRep_MatchState();
	
	UPROPERTY(EditAnywhere, Category = "Network")
	float HighPingDuration = 5.f;

	UPROPERTY(EditAnywhere, Category = "Network")
	float CheckPingFrequency = 20.f;

	UPROPERTY(EditAnywhere, Category = "Network")
	float HighPingThreshold = 50.f;
	
	void CheckPing();

};


