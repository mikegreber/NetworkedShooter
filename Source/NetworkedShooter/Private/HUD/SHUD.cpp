// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/SHUD.h"
#include "Components/TextBlock.h"
#include "HUD/SAnnouncementWidget.h"
#include "HUD/SCharacterOverlay.h"
#include "HUD/SEliminationAnnouncementWidget.h"

void ASHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ASHUD::Initialize()
{
	CreateAnnouncement();
	CreateEliminationAnnouncement();
	CreateOverlay();
}

void ASHUD::CreateEliminationAnnouncement()
{
	if (APlayerController* OwningPlayer = GetOwningPlayerController())
	{
		if (EliminationAnnouncementClass)
		{
			EliminationAnnouncement = CreateWidget<USEliminationAnnouncementWidget>(OwningPlayer, EliminationAnnouncementClass);
			if (EliminationAnnouncement) EliminationAnnouncement->AddToViewport();
		}
	}
}

void ASHUD::AddEliminationAnnouncement(FString Attacker, FString Victim) const
{
	if (EliminationAnnouncementClass)
	{
		if (EliminationAnnouncement)
		{
			EliminationAnnouncement->SetEliminationAnnouncementText(Attacker, Victim);
		}
	}
}

void ASHUD::EliminationAnnouncementTimerFinished(USEliminationAnnouncementWidget* AnnouncementToRemove)
{
	if (AnnouncementToRemove)
	{
		AnnouncementToRemove->RemoveFromParent();
	}
}

void ASHUD::CreateAnnouncement()
{
	if (Announcement) return;
	
	if (APlayerController* PlayerController = GetOwningPlayerController())
	{
		if (AnnouncementClass)
		{
			Announcement = CreateWidget<USAnnouncementWidget>(PlayerController, AnnouncementClass);
			Announcement->SetVisibility(ESlateVisibility::Collapsed);
			Announcement->AddToViewport();
		}
	}
}

void ASHUD::ShowAnnouncement(bool bShow) const
{
	if (Announcement)
	{
		Announcement->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void ASHUD::CreateOverlay()
{
	if (CharacterOverlay) return;
	
	if (APlayerController* PlayerController = GetOwningPlayerController())
	{
		if (CharacterOverlayClass)
		{
			CharacterOverlay = CreateWidget<USCharacterOverlay>(PlayerController, CharacterOverlayClass);
			CharacterOverlay->SetVisibility(ESlateVisibility::Collapsed);
			CharacterOverlay->AddToViewport();

			HealthBar = CharacterOverlay->HealthBar;
			HealthText = CharacterOverlay->HealthText;
			ShieldBar = CharacterOverlay->ShieldBar;
			ShieldText = CharacterOverlay->ShieldText;
			KillsAmount = CharacterOverlay->KillsAmount;
			DeathsAmount = CharacterOverlay->DeathsAmount;
			WeaponAmmoAmount = CharacterOverlay->WeaponAmmoAmount;
			CarriedAmmoAmount = CharacterOverlay->CarriedAmmoAmount;
			MatchCountdownText = CharacterOverlay->MatchCountdownText;
			GrenadesText = CharacterOverlay->GrenadesText;
			RedTeamScore = CharacterOverlay->RedTeamScore;
			BlueTeamScore = CharacterOverlay->BlueTeamScore;
			ScoreSpacerText = CharacterOverlay->ScoreSpacerText;
			HighPingImage = CharacterOverlay->HighPingImage;
			HighPingAnimation = CharacterOverlay->HighPingAnimation;
		}
	}
}

void ASHUD::ShowOverlay(bool bShow) const
{
	if (CharacterOverlay)
	{
		CharacterOverlay->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void ASHUD::DrawHUD()
{
	Super::DrawHUD();
	
	if (GEngine)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter(ViewportSize / 2.f);

		const float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;
		
		if (HUDPackage.CrosshairsCenter)
		{
			DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, FVector2D(), HUDPackage.CrosshairsColor);
		}
		if (HUDPackage.CrosshairsLeft)
		{
			DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, FVector2D(-SpreadScaled, 0.f), HUDPackage.CrosshairsColor);
		}
		if (HUDPackage.CrosshairsRight)
		{
			DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, FVector2D(SpreadScaled, 0.f), HUDPackage.CrosshairsColor);
		}
		if (HUDPackage.CrosshairsTop)
		{
			DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter, FVector2D(0.f, -SpreadScaled), HUDPackage.CrosshairsColor);
		}
		if (HUDPackage.CrosshairsBottom)
		{
			DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter, FVector2D(0.f, SpreadScaled), HUDPackage.CrosshairsColor);
		}
	}
}

void ASHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread,  FLinearColor CrosshairColor)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();

	DrawTexture(
		Texture,
		ViewportCenter.X - (TextureWidth / 2.f) + Spread.X,
		ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y,
		TextureWidth,
		TextureHeight,
		0.f,
		0.f,
		1.f,
		1.f,
		CrosshairColor
	);
}

