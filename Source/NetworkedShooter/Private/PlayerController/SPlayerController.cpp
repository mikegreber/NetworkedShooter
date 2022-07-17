// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/SPlayerController.h"

#include "Character/SCharacter.h"
#include "Character/SCharacter_Lobby.h"
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
#include "HUD/SReturnToMainMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "PlayerState/SPlayerState.h"
#include "Types/Announcement.h"

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
	DOREPLIFETIME(ASPlayerController, bShowTeamScores);
	DOREPLIFETIME_CONDITION(ASPlayerController, bHasHighPing, COND_OwnerOnly);
}

void ASPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		
		InitHUD();
		SetGameState(GetWorld()->GetGameState());

		if (!HasAuthority())
		{
			FTimerHandle PingHandle;
			GetWorldTimerManager().SetTimer(PingHandle, this, &ASPlayerController::CheckPing, CheckPingFrequency, true);

			FTimerHandle SyncHandle;
			GetWorldTimerManager().SetTimer(SyncHandle, this, &ASPlayerController::SyncServerTime, TimeSyncFrequency, true);
		}
	}

	ServerCheckMatchState();
}

void ASPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	
	Super::EndPlay(EndPlayReason);
}

void ASPlayerController::InitHUD()
{
	HUD = Cast<ASHUD>(GetHUD());
	HUD->Initialize();
	OnHUDInitialized.Broadcast();
}

void ASPlayerController::SetGameState(AGameStateBase* NewGameState)
{
	GameState = Cast<ASGameState>(NewGameState);
	if (GameState)
	{
		HUD->ShowAnnouncement(true);
		GameState->OnRedTeamUpdateScore.AddUniqueDynamic(this, &ASPlayerController::SetHUDRedTeamScore);
		GameState->OnBlueTeamUpdateScore.AddUniqueDynamic(this, &ASPlayerController::SetHUDBlueTeamScore);
		SetActorTickEnabled(true);
		GetWorld()->GameStateSetEvent.RemoveAll(this);
	}
	else
	{
		GetWorld()->GameStateSetEvent.AddUObject(this, &ASPlayerController::SetGameState);
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
		DisplayHighPingWarning();
		ServerReportHighPing(true);
	}
	else
	{
		ServerReportHighPing(false);
	}
}

void ASPlayerController::ServerReportHighPing_Implementation(bool bHighPing)
{
	bHasHighPing = bHighPing;
	ShooterPlayerState->bHasHighPing = bHighPing;
}

void ASPlayerController::InitPlayerState()
{
	Super::InitPlayerState();
	OnRep_PlayerState();
}

void ASPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	SetPlayerState(PlayerState);
	
	
}

void ASPlayerController::PlayerStateHUDInit()
{
	if (ShooterPlayerState)
	{
		if (HUD && HUD->CharacterOverlay)
		{
			ShooterPlayerState->OnKillsUpdated.AddUniqueDynamic(this, &ASPlayerController::SetHUDKills);
			ShooterPlayerState->OnKillsUpdated.Broadcast(ShooterPlayerState->GetKills());
			
			ShooterPlayerState->OnDeathsUpdated.AddUniqueDynamic(this, &ASPlayerController::SetHUDDeaths);
			ShooterPlayerState->OnDeathsUpdated.Broadcast(ShooterPlayerState->GetDeaths());
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
		if (ShooterPlayerState)
		{
			ShooterPlayerState->OnKillsUpdated.RemoveDynamic(this, &ASPlayerController::SetHUDKills);
			ShooterPlayerState->OnDeathsUpdated.RemoveDynamic(this, &ASPlayerController::SetHUDDeaths);
		}
	}
	
	Super::CleanupPlayerState();
}

void ASPlayerController::SetPlayerState(APlayerState* NewPlayerState)
{
	ShooterPlayerState = Cast<ASPlayerState>(NewPlayerState);
	OnPlayerStateSet.Broadcast(ShooterPlayerState);
	
	if (ShooterPlayerState && IsLocalPlayerController()) PlayerStateHUDInit();
	if (ASCharacter_Lobby* LobbyCharacter = ShooterPlayerState->GetPawn<ASCharacter_Lobby>())
	{
		LobbyCharacter->SetTopOverheadText(ShooterPlayerState->GetPlayerName());
	}
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

void ASPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ASPlayerController::SyncServerTime()
{
	ServerRequestServerTime(GetWorld()->GetTimeSeconds());
}

void ASPlayerController::ServerRequestServerTime_Implementation(float ClientRequestTime)
{
	ClientReportServerTime(ClientRequestTime, GetWorld()->GetTimeSeconds());
}

void ASPlayerController::ClientReportServerTime_Implementation(float ClientRequestTime, float ServerTime)
{
	const float RoundTripTime = GetWorld()->GetTimeSeconds() - ClientRequestTime;
	SingleTripTime = RoundTripTime * 0.5f;
	const float CurrentServerTime = ServerTime + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
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
	else if (!OnNewPawnDelegateHandle.IsValid())
	{
		OnNewPawnDelegateHandle = OnNewPawn.AddLambda([&](APawn* NewPawn)
		{
			this->PlayerCharacterHUDInit();
			OnNewPawn.Remove(OnNewPawnDelegateHandle);
		});
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

void ASPlayerController::ShowReturnToMainMenu()
{
	if (ReturnToMainMenuWidget)
	{
		if (!ReturnToMainMenuWidgetInstance)
		{
			ReturnToMainMenuWidgetInstance = CreateWidget<USReturnToMainMenuWidget>(this, ReturnToMainMenuWidget);
		}
		
		if (ReturnToMainMenuWidgetInstance)
		{
			bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
			if (bReturnToMainMenuOpen)
			{
				ReturnToMainMenuWidgetInstance->MenuSetup();
			}
			else
			{
				ReturnToMainMenuWidgetInstance->MenuTeardown();
			}
		}
	}
}

bool ASPlayerController::HasLocalAuthority() const
{
	return GetRemoteRole() != ROLE_AutonomousProxy && GetLocalRole() == ROLE_Authority;
}

bool ASPlayerController::HasLowPing() const
{
	return !bHasHighPing;
}

void ASPlayerController::OnMatchStateSet(FName State, bool bTeamsMatch)
{
	MatchState = State;
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted(bTeamsMatch);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
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



void ASPlayerController::HandleMatchHasStarted(bool bIsTeamMatch)
{
	UE_LOG(LogTemp, Warning, TEXT("%s %s %d"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR, bIsTeamMatch);
	if (HasAuthority())
	{
		bShowTeamScores = bIsTeamMatch;
		OnRep_ShowTeamScores();
	}
	
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

FString ASPlayerController::GetInfoText(const TArray<ASPlayerState*>& TopPlayers)
{
	FString InfoTextString;
	if (TopPlayers.IsEmpty())
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (TopPlayers.Num() == 1 && TopPlayers[0] == ShooterPlayerState)
	{
		InfoTextString = Announcement::YouAreTheWinner;
	}
	else if (TopPlayers.Num() == 1)
	{
		InfoTextString = FString::Printf(TEXT("Winner:\n%s"), *TopPlayers[0]->GetPlayerName());
	}
	else
	{
		InfoTextString = Announcement::PlayersTiedForTheWin;
		for (const ASPlayerState* TiedPlayer : TopPlayers)
		{
			InfoTextString.Append(FString::Printf(TEXT("\n%s"), *TiedPlayer->GetPlayerName()));
		}
	}
	return InfoTextString;
}

FString ASPlayerController::GetTeamsInfoText(ASGameState* ShooterGameState)
{
	FString InfoTextString;
	if (ShooterGameState)
	{
		const int32 RedTeamScore = ShooterGameState->RedTeamScore;
		const int32 BlueTeamScore = ShooterGameState->BlueTeamScore;

		if (RedTeamScore == 0 && BlueTeamScore == 0)
		{
			InfoTextString = Announcement::ThereIsNoWinner;
		}
		else if (RedTeamScore == BlueTeamScore)
		{
			InfoTextString = Announcement::TeamsTiedForTheWin;
			InfoTextString.Append(FString::Printf(TEXT("\n%s"), *Announcement::RedTeam));
			InfoTextString.Append(FString::Printf(TEXT("\n%s"), *Announcement::BlueTeam));
		}
		else if (RedTeamScore > BlueTeamScore)
		{
			InfoTextString = Announcement::RedTeamWins;
			InfoTextString.Append(FString::Printf(TEXT("\n%s: %d"), *Announcement::RedTeam, RedTeamScore));
			InfoTextString.Append(FString::Printf(TEXT("\n%s: %d"), *Announcement::BlueTeam, BlueTeamScore));
		}
		else
		{
			InfoTextString = Announcement::BlueTeamWins;
			InfoTextString.Append(FString::Printf(TEXT("\n%s: %d"), *Announcement::BlueTeam, BlueTeamScore));
			InfoTextString.Append(FString::Printf(TEXT("\n%s: %d"), *Announcement::RedTeam, RedTeamScore));
		}
	}
	return InfoTextString;
}

void ASPlayerController::HandleCooldown()
{
	if (IsLocalController())
	{
		HUD->ShowAnnouncement(true);
		HUD->ShowOverlay(false);

		const FString AnnouncementText(Announcement::NewMatchStartsIn);
		HUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
		
		if (ASGameState* GS = Cast<ASGameState>(UGameplayStatics::GetGameState(this)))
		{
			TArray<ASPlayerState*> TopPlayers = GS->TopScoringPlayers;
			const FString InfoTextString = bShowTeamScores ? GetTeamsInfoText(GS) : GetInfoText(GS->TopScoringPlayers);
			
			HUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
		}
	}

	if (ASCharacter* SCharacter = Cast<ASCharacter>(GetPawn()))
	{
		SCharacter->SetDisableGameplay(true);
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

void ASPlayerController::SetHUDRedTeamScore(int32 Score)
{
	if (IsLocalController())
	{
		HUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(FString::FromInt(Score)));
	}
}

void ASPlayerController::SetHUDBlueTeamScore(int32 Score)
{
	if (IsLocalController())
	{
		HUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(FString::FromInt(Score)));
	}
}

void ASPlayerController::HideTeamScores()
{
	UE_LOG(LogTemp, Warning, TEXT("%s %s"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR);
	if (IsLocalController())
	{
		HUD->CharacterOverlay->RedTeamScore->SetText(FText());
		HUD->CharacterOverlay->BlueTeamScore->SetText(FText());
		HUD->CharacterOverlay->ScoreSpacerText->SetText(FText());
	}
}

void ASPlayerController::InitTeamScores()
{
	UE_LOG(LogTemp, Warning, TEXT("%s %s"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR);
	if (IsLocalController())
	{
		HUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString("0"));
		HUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString("0"));
		HUD->CharacterOverlay->ScoreSpacerText->SetText(FText::FromString("|"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Not Local"))
	}
}

void ASPlayerController::BroadcastElimination(APlayerState* Attacker, APlayerState* Victim)
{
	ClientEliminationAnnouncement(Attacker, Victim);
}



void ASPlayerController::ClientEliminationAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	const APlayerState* Self = GetPlayerState<APlayerState>();
	if (Attacker && Victim && Self)
	{
		if (HUD)
		{
			if (Attacker == Self && Victim != Self)
			{
				HUD->AddEliminationAnnouncement("You", Victim->GetPlayerName());
			}
			else if (Victim == Self && Attacker != Self)
			{
				HUD->AddEliminationAnnouncement(Attacker->GetPlayerName(), "You");
			}
			else if (Attacker == Victim && Attacker == Self)
			{
				HUD->AddEliminationAnnouncement("You", "Yourself");
			}
			else if (Attacker == Victim && Attacker != Self)
			{
				HUD->AddEliminationAnnouncement(Attacker->GetPlayerName(), "Themself");
			}
			else
			{
				HUD->AddEliminationAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
			}
		}
	}
}

void ASPlayerController::OnRep_ShowTeamScores()
{
	UE_LOG(LogTemp, Warning, TEXT("%s %s %d"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR, bShowTeamScores)
	if (IsLocalController())
	{
		if (bShowTeamScores)
		{
			InitTeamScores();
		}
		else
		{
			HideTeamScores();
		}
	}
}

void ASPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent) return;
	
	InputComponent->BindAction("Quit", IE_Pressed, this, &ASPlayerController::ShowReturnToMainMenu);
	
}

void ASPlayerController::DisplayHighPingWarning()
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

FORCEINLINE void ASPlayerController::SetHUDTime()
{
	uint32 SecondsLeft;
	if (HasAuthority() && GameMode)
	{
		SecondsLeft = FMath::CeilToInt(GameMode->GetCountdownTime() + LevelStartingTime);
	}
	else
	{
		float TimeLeft = 0.f;
		if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
		else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
		else if (MatchState == MatchState::Cooldown) TimeLeft = WarmupTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;
		SecondsLeft = FMath::CeilToInt(TimeLeft);
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



#undef LOG_WARNING
