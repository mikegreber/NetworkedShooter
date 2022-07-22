// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameplayTagStack.h"
#include "GameFramework/PlayerController.h"
#include "SPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHUDCreated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHighPingUpdated, bool, bHighPing);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStateSet, class ASPlayerState*, NewPlayerState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameStateSet, class ASGameState*, NewGameState);

UCLASS()
class NETWORKEDSHOOTER_API ASPlayerController : public APlayerController
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Network")
	float TimeSyncFrequency = 5.f;
	
	UPROPERTY(EditAnywhere, Category = "Network")
	float HighPingDuration = 5.f;

	UPROPERTY(EditAnywhere, Category = "Network")
	float CheckPingFrequency = 20.f;

	UPROPERTY(EditAnywhere, Category = "Network")
	float HighPingThreshold = 50.f;

	UPROPERTY(EditAnywhere, Category = "Menu")
	TSubclassOf<class USReturnToMainMenuWidget> ReturnToMainMenuWidget;

	UPROPERTY()
	USReturnToMainMenuWidget* ReturnToMainMenuWidgetInstance;

	bool bReturnToMainMenuOpen = false;
	
public:

	FOnHighPingUpdated OnHighPingUpdated;
	FOnPlayerStateSet OnPlayerStateSet;
	FOnGameStateSet OnGameStateSet;

	ASPlayerController();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitPlayerState() override;
	virtual void OnRep_PlayerState() override;
	virtual void CleanupPlayerState() override;
	void SetPlayerState(APlayerState* NewPlayerState);
	virtual void OnRep_Pawn() override;
	virtual void ReceivedPlayer() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void AcknowledgePossession(APawn* P) override;

	bool HasLocalAuthority() const;
	bool HasLowPing() const;

	void OnMatchStateSet(FName State, bool bTeamsMatch = false);

	// synced with server world clock
	float GetServerTime() const;
	
	UFUNCTION() void SetHUDHealth(float Health, float MaxHealth);
	UFUNCTION() void SetHUDShield(float Shield, float MaxShield);
	UFUNCTION() void SetHUDKills(int32 Score);
	UFUNCTION() void SetHUDDeaths(int32 Deaths);
	UFUNCTION() void SetHUDWeaponAmmo(int32 Ammo);
	UFUNCTION() void SetHUDGrenades(int32 Grenades);
	UFUNCTION() void SetHUDRedTeamScore(int32 Score);
	UFUNCTION() void SetHUDBlueTeamScore(int32 Score);

	UFUNCTION() void SetHUDCarriedAmmo(const FGameplayTagStackContainer& AmmoContainer, FGameplayTag EquippedAmmoType);
	
	void HideTeamScores();
	void InitTeamScores();
	
	void BroadcastElimination(APlayerState* Attacker, APlayerState* Victim);

	FString GetInfoText(const TArray<ASPlayerState*>& Players);
	FString GetTeamsInfoText(ASGameState* ShooterGameState);

protected:

	UPROPERTY(ReplicatedUsing=OnRep_ShowTeamScores)
	bool bShowTeamScores;

	UFUNCTION()
	void OnRep_ShowTeamScores();
	
	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* NewPawn) override;
	virtual void OnUnPossess() override;

	void ShowReturnToMainMenu();

	void SyncServerTime();
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float ClientRequestTime, float ServerTime);
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float ClientRequestTime);

	void HandleMatchHasStarted(bool bIsTeamMatch = false);
	void HandleCooldown();

	
	UFUNCTION()
	void PlayerCharacterHUDInit();
	
	UFUNCTION()
	void PlayerStateHUDInit();

	void DisplayHighPingWarning();
	
	void StopHighPingWarning();

	UFUNCTION(Client, Reliable)
	void ClientEliminationAnnouncement(APlayerState* Attacker, APlayerState* Victim);
	
private:
	
	void InitHUD();

	void SetGameState(AGameStateBase* GetGameState);
	
	void SetHUDTime();

	UFUNCTION()
	void SetHUDMatchCountdown(float CountdownTime) const;
	
	void SetHUDAnnouncementCountdown(float CountdownTime);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(FName StateOfMatch, float Warmup, float Match, float StartingTime, float Cooldown);

	UFUNCTION()
	void OnRep_MatchState();
	
	void CheckPing();

	UFUNCTION(Server, Reliable)
	void ServerReportHighPing(bool bHighPing);
	
	UPROPERTY() class ASHUD* HUD;
	UPROPERTY() class ASGameState* GameState;
	UPROPERTY() class ASGameMode* GameMode;
	UPROPERTY() class ASPlayerState* ShooterPlayerState;
	UPROPERTY() class USAttributeSet* AttributeSet;

	UPROPERTY(ReplicatedUsing=OnRep_MatchState)
	FName MatchState;

	UPROPERTY(Replicated)
	bool bHasHighPing;
	
	float SingleTripTime = 0.f;
	float ClientServerDelta = 0.f;
	float LevelStartingTime = 0.f;
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt;
	FOnHUDCreated OnHUDInitialized;
	FDelegateHandle GameStateSetDelegateHandle;
	FDelegateHandle OnNewPawnDelegateHandle;

public:
	FORCEINLINE float GetSingleTripTime() const { return SingleTripTime; };
};


FORCEINLINE float ASPlayerController::GetServerTime() const
{
	return HasAuthority() ? GetWorld()->GetTimeSeconds() : GetWorld()->GetTimeSeconds() + ClientServerDelta;
}