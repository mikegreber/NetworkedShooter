// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/SHUD.h"

#include "Components/TextBlock.h"
#include "HUD/SAnnouncementWidget.h"
#include "HUD/SCharacterOverlay.h"

void ASHUD::BeginPlay()
{
	Super::BeginPlay();
	
	// UE_LOG(LogTemp, Warning, TEXT("%s %s"), __FUNCTIONW__, *GetNameSafe(this));

}

void ASHUD::Initialize()
{
	CreateAnnouncement();
	CreateOverlay();
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

void ASHUD::ShowAnnouncement(bool bShow)
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
		}
	}
}

void ASHUD::ShowOverlay(bool bShow)
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

void ASHUD::UpdateKills(int32 NewValue)
{
	CharacterOverlay->KillsAmount->SetText(FText::FromString(FString::Printf(TEXT("%d"), NewValue)));
}

void ASHUD::UpdateDeaths(int32 NewValue)
{
	CharacterOverlay->DeathsAmount->SetText(FText::FromString(FString::Printf(TEXT("%d"), NewValue)));
}

// void ASHUD::BindDelegates(ASPlayerState* PlayerState)
// {
// 	PlayerState->OnKillsUpdated.AddDynamic(this, &ASHUD::UpdateKills);
// 	PlayerState->OnDeathsUpdated.AddDynamic(this, &ASHUD::UpdateDeaths);
// }

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
