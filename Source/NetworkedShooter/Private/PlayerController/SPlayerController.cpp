// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/SPlayerController.h"

#include "Character/SCharacter.h"
#include "Components/Image.h"
#include "Components/SCombatComponent.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/GameStateBase.h"
#include "GameMode/SGameMode.h"
#include "GameState/SGameState.h"
#include "HUD/SAnnouncementWidget.h"
#include "HUD/SCharacterOverlay.h"
#include "HUD/SHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "PlayerState/SPlayerState.h"

#define LOG_WARNING() UE_LOG(LogTemp, Warning, TEXT("Called %s on non-local controller (%s)"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR);

ASPlayerController::ASPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = 0;
	bAllowTickBeforeBeginPlay = false;
}

void ASPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASPlayerController, MatchState);
}

void ASPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsLocalController())
	{
		InitGameStateAndTick();
	}

	ServerCheckMatchState();

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &ASPlayerController::CheckPing, CheckPingFrequency, true);
}

void ASPlayerController::InitGameStateAndTick()
{
	HUD = Cast<ASHUD>(GetHUD());
	HUD->Initialize();
	OnHUDInitialized.Broadcast();
	
	if (GetWorld()->GetGameState())
	{
		GameState = GetWorld()->GetGameState();
		HUD->ShowAnnouncement(true);
		SetActorTickEnabled(true);
	}
	else
	{
		GameStateSetDelegateHandle = GetWorld()->GameStateSetEvent.AddLambda([&](AGameStateBase* GS)
		{
			GameState = GS;
			HUD->ShowAnnouncement(true);
			SetActorTickEnabled(true);
			GetWorld()->GameStateSetEvent.Remove(GameStateSetDelegateHandle);
		});
	}
}

void ASPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SetHUDTime();
}

void ASPlayerController::CheckPing()
{
	if (PlayerState && PlayerState->GetPingInMilliseconds() > HighPingThreshold)
	{
		HighPingWarning();
	}
}

void ASPlayerController::InitPlayerState()
{
	Super::InitPlayerState();
	OnRep_PlayerState();
}

void ASPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (PlayerState && IsLocalPlayerController())
	{
		PlayerStateHUDInit();
	}
}

void ASPlayerController::PlayerStateHUDInit()
{
	if (ASPlayerState* PS = GetPlayerState<ASPlayerState>())
	{
		if (HUD && HUD->CharacterOverlay)
		{
			PS->OnKillsUpdated.AddUniqueDynamic(this, &ASPlayerController::SetHUDKills);
			PS->OnKillsUpdated.Broadcast(PS->GetKills());
			
			PS->OnDeathsUpdated.AddUniqueDynamic(this, &ASPlayerController::SetHUDDeaths);
			PS->OnDeathsUpdated.Broadcast(PS->GetDeaths());
		}
		else
		{
			OnHUDInitialized.AddUniqueDynamic(this, &ASPlayerController::PlayerStateHUDInit);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s called without valid PlayerState. %s"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR);
	}
}

void ASPlayerController::CleanupPlayerState()
{
	if (IsLocalPlayerController())
	{
		if (ASPlayerState* PS = GetPlayerState<ASPlayerState>())
		{
			PS->OnKillsUpdated.RemoveDynamic(this, &ASPlayerController::SetHUDKills);
			PS->OnDeathsUpdated.RemoveDynamic(this, &ASPlayerController::SetHUDDeaths);
		}
	}
	
	Super::CleanupPlayerState();
}

void ASPlayerController::OnPossess(APawn* NewPawn)
{
	Super::OnPossess(NewPawn);
	if (HasLocalAuthority()) OnRep_Pawn();
}

void ASPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	if (GetPawn() && IsLocalPlayerController())
	{
		PlayerCharacterHUDInit();
	}
}

void ASPlayerController::PlayerCharacterHUDInit()
{
	if (ASCharacter* SCharacter = Cast<ASCharacter>(GetPawn()))
	{
		if (HUD && HUD->CharacterOverlay)
		{
			SCharacter->OnHealthChanged.AddUniqueDynamic(this, &ASPlayerController::SetHUDHealth);
			SCharacter->OnHealthChanged.Broadcast(SCharacter->GetHealth(), SCharacter->GetMaxHealth());

			SCharacter->OnShieldChanged.AddUniqueDynamic(this, &ASPlayerController::SetHUDShield);
			SCharacter->OnShieldChanged.Broadcast(SCharacter->GetShield(), SCharacter->GetMaxShield());

			SCharacter->GetCombatComponent()->OnCarriedAmmoUpdated.AddUniqueDynamic(this, &ASPlayerController::SetHUDCarriedAmmo);
			SCharacter->GetCombatComponent()->OnCarriedAmmoUpdated.Broadcast(SCharacter->GetCombatComponent()->GetCarriedAmmo());

			SCharacter->GetCombatComponent()->OnGrenadesUpdated.AddUniqueDynamic(this, &ASPlayerController::SetHUDGrenades);
			SCharacter->GetCombatComponent()->OnGrenadesUpdated.Broadcast(SCharacter->GetCombatComponent()->GetGrenades());
		}
		else
		{
			OnHUDInitialized.AddUniqueDynamic(this, &ASPlayerController::PlayerCharacterHUDInit);
		}
	}
	else
	{
		if (!OnNewPawnDelegateHandle.IsValid())
		{
			OnNewPawnDelegateHandle = OnNewPawn.AddLambda([&](APawn* NewPawn)
			{
				this->PlayerCharacterHUDInit();
				OnNewPawn.Remove(OnNewPawnDelegateHandle);
			});
		}
	}
}

void ASPlayerController::OnUnPossess()
{
	if (IsLocalController())
	{
		if (ASCharacter* SCharacter = Cast<ASCharacter>(GetPawn()))
		{
			SCharacter->OnHealthChanged.RemoveDynamic(this, &ASPlayerController::SetHUDHealth);
			SCharacter->GetCombatComponent()->OnCarriedAmmoUpdated.RemoveDynamic(this, &ASPlayerController::SetHUDCarriedAmmo);
			SCharacter->GetCombatComponent()->OnGrenadesUpdated.RemoveDynamic(this, &ASPlayerController::SetHUDGrenades);
		}
	}
	
	Super::OnUnPossess();
}

bool ASPlayerController::HasLocalAuthority() const
{
	return GetRemoteRole() != ROLE_AutonomousProxy && GetLocalRole() == ROLE_Authority;
}

void ASPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;
	OnRep_MatchState();
}

void ASPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}



void ASPlayerController::HandleMatchHasStarted()
{
	if (IsLocalController())
	{
		if (HUD->Announcement)
		{
			HUD->ShowAnnouncement(false);
		}
		if (HUD->CharacterOverlay)
		{
			HUD->ShowOverlay(true);
		}
	}
}

void ASPlayerController::HandleCooldown()
{
	if (IsLocalController())
	{
		HUD->ShowAnnouncement(true);
		HUD->ShowOverlay(false);

		const FString AnnouncementText("New Match Starts In: ");
		HUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
		
		
		if (ASGameState* GS = Cast<ASGameState>(UGameplayStatics::GetGameState(this)))
		{
			ASPlayerState* PS = GetPlayerState<ASPlayerState>();
			TArray<ASPlayerState*> TopPlayers = GS->TopScoringPlayers;
			FString InfoTextString;
			if (TopPlayers.IsEmpty())
			{
				InfoTextString = "There is no winner.";
			}
			else if (TopPlayers.Num() == 1 && TopPlayers[0] == PS)
			{
				InfoTextString = "You are the winner!";
			}
			else if (TopPlayers.Num() == 1)
			{
				InfoTextString = FString::Printf(TEXT("Winner:\n%s"), *TopPlayers[0]->GetPlayerName());
			}
			else
			{
				InfoTextString = "Players tied for the win:\n";
				for (ASPlayerState* TiedPlayer : TopPlayers)
				{
					InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
				}
			}
			
			HUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
		}
	}

	if (ASCharacter* SCharacter = Cast<ASCharacter>(GetPawn()))
	{
		SCharacter->bDisableGameplay = true;
		SCharacter->GetCombatComponent()->FireButtonPressed(false);
	}
}

void ASPlayerController::ServerCheckMatchState_Implementation()
{
	if (ASGameMode* ServerGameMode = Cast<ASGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GameMode = ServerGameMode;
		MatchState = ServerGameMode->GetMatchState();
		WarmupTime = ServerGameMode->WarmupTime;
		MatchTime = ServerGameMode->MatchTime;
		CooldownTime = ServerGameMode->CooldownTime;
		LevelStartingTime = ServerGameMode->LevelStartingTime;
		
		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, LevelStartingTime, CooldownTime);

		if (HUD)
		{
			if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
			{
				HUD->ShowAnnouncement(true);
				HUD->ShowOverlay(false);
			}
			else if (MatchState == MatchState::InProgress)
			{
				HUD->ShowAnnouncement(false);
				HUD->ShowOverlay(true);
			}
		}
	}
}

void ASPlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float Warmup, float Match, float StartingTime, float Cooldown)
{
	MatchState = StateOfMatch;
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	OnMatchStateSet(MatchState);

	if (HUD)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			HUD->ShowAnnouncement(true);
			HUD->ShowOverlay(false);
		}
		else if (MatchState == MatchState::InProgress)
		{
			HUD->ShowAnnouncement(false);
			HUD->ShowOverlay(true);
		}
	}
}



void ASPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	if (IsLocalController())
	{
		HUD->CharacterOverlay->HealthBar->SetPercent(Health/MaxHealth);
		const FText HealthText = FText::FromString(FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth)));
		HUD->CharacterOverlay->HealthText->SetText(HealthText);
	}
	else
	{
		LOG_WARNING();
	}
}

void ASPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	if (IsLocalController())
	{
		HUD->CharacterOverlay->ShieldBar->SetPercent(Shield/MaxShield);
		const FText ShieldText = FText::FromString(FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield)));
		HUD->CharacterOverlay->ShieldText->SetText(ShieldText);
	}
	else
	{
		LOG_WARNING();
	}
}

void ASPlayerController::SetHUDKills(int32 Kills)
{
	if (IsLocalController())
	{
		const FText KillsText = FText::FromString(FString::Printf(TEXT("%d"), Kills));
		HUD->CharacterOverlay->KillsAmount->SetText(KillsText);
	}
	else
	{
		LOG_WARNING();
	}
}

void ASPlayerController::SetHUDDeaths(int32 Deaths)
{
	if (IsLocalController())
	{
		const FText DeathsText = FText::FromString(FString::Printf(TEXT("%d"), Deaths));
		HUD->CharacterOverlay->DeathsAmount->SetText(DeathsText);
	}
	else
	{
		LOG_WARNING();
	}
}

void ASPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	if (IsLocalController())
	{
		const FText AmmoText = FText::FromString(FString::Printf(TEXT("%d"), Ammo));
		HUD->CharacterOverlay->WeaponAmmoAmount->SetText(AmmoText);
	}
	else
	{
		LOG_WARNING();
	}
}

void ASPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	if (IsLocalController())
	{
		const FText AmmoText = FText::FromString(FString::Printf(TEXT("%d"), Ammo));
		HUD->CharacterOverlay->CarriedAmmoAmount->SetText(AmmoText);
	}
	else
	{
		LOG_WARNING();
	}
}

void ASPlayerController::SetHUDGrenades(int32 Grenades)
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), __FUNCTIONW__)
	if (IsLocalController())
	{
		const FText AmmoText = FText::FromString(FString::Printf(TEXT("%d"), Grenades));
		HUD->CharacterOverlay->GrenadesText->SetText(AmmoText);
	}
	else
	{
		LOG_WARNING();
	}
}

void ASPlayerController::HighPingWarning()
{
	if (IsLocalController())
	{
		HUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		HUD->CharacterOverlay->PlayAnimation(
			HUD->CharacterOverlay->HighPingAnimation,
			0.f,
			5
		);
		
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, this, &ASPlayerController::StopHighPingWarning, HighPingDuration);
	}
	else
	{
		LOG_WARNING();
	}
}

void ASPlayerController::StopHighPingWarning()
{
	if (IsLocalController())
	{
		HUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (HUD->CharacterOverlay->IsAnimationPlaying(HUD->CharacterOverlay->HighPingAnimation))
		{
			HUD->CharacterOverlay->StopAnimation(HUD->CharacterOverlay->HighPingAnimation);
		}
	}
	else
	{
		LOG_WARNING();
	}
}


void ASPlayerController::SetHUDMatchCountdown(float CountdownTime) const
{
	if (IsLocalController())
	{
		if (CountdownTime < 0.f)
		{
			HUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		
		const int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		const int32 Seconds = FMath::FloorToInt(CountdownTime - Minutes * 60.f);
		const FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		HUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
	else
	{
		LOG_WARNING();
	}
}

void ASPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	if (IsLocalController())
	{
		if (CountdownTime < 0.f)
		{
			HUD->Announcement->WarmupTimeText->SetText(FText());
			return;
		}

		
		const int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		const int32 Seconds = FMath::FloorToInt(CountdownTime - Minutes * 60.f);
		const FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		HUD->Announcement->WarmupTimeText->SetText(FText::FromString(CountdownText));
	}
	else
	{
		LOG_WARNING();
	}
}

void ASPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = WarmupTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	
	if (HasAuthority() && GameMode)
	{
		SecondsLeft = FMath::CeilToInt(GameMode->GetCountdownTime() + LevelStartingTime);
	}
	
	if (CountdownInt != SecondsLeft)
	{
		CountdownInt = SecondsLeft;

		if (MatchState == MatchState::WaitingToStart)
		{
			SetHUDAnnouncementCountdown(SecondsLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(SecondsLeft);
		}
		else if (MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(SecondsLeft);
		}
	}
}

float ASPlayerController::GetServerTime() const
{
	return GameState->GetServerWorldTimeSeconds();
}



#undef LOG_WARNING
