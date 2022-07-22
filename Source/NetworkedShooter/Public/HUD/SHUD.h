#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Types/HUDPackage.h"
#include "SHUD.generated.h"

UCLASS()
class NETWORKEDSHOOTER_API ASHUD : public AHUD
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class USCharacterOverlay> CharacterOverlayClass;
	
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class USAnnouncementWidget> AnnouncementClass;
	
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class USEliminationAnnouncementWidget> EliminationAnnouncementClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	float EliminationAnnouncementTime = 1.5f;
	
	UPROPERTY(EditAnywhere, Category = "UI")
	float CrosshairSpreadMax = 16.f;
	
	FHUDPackage HUDPackage;

public:
	
	UPROPERTY() USCharacterOverlay* CharacterOverlay;
	UPROPERTY() USAnnouncementWidget* Announcement;
	UPROPERTY() class UProgressBar* HealthBar;
	UPROPERTY() class UTextBlock* HealthText;
	UPROPERTY() UProgressBar* ShieldBar;
	UPROPERTY() UTextBlock* ShieldText;
	UPROPERTY() UTextBlock* KillsAmount;
	UPROPERTY() UTextBlock* DeathsAmount;
	UPROPERTY() UTextBlock* WeaponAmmoAmount;
	UPROPERTY() UTextBlock* CarriedAmmoAmount;
	UPROPERTY() UTextBlock* MatchCountdownText;
	UPROPERTY() UTextBlock* GrenadesText;
	UPROPERTY() UTextBlock* RedTeamScore;
	UPROPERTY() UTextBlock* BlueTeamScore;
	UPROPERTY() UTextBlock* ScoreSpacerText;
	UPROPERTY() class UImage* HighPingImage;
	UPROPERTY() class UWidgetAnimation* HighPingAnimation;
	UPROPERTY() USEliminationAnnouncementWidget* EliminationAnnouncement;

public:
	
	virtual void DrawHUD() override;

	void Initialize();

	void AddEliminationAnnouncement(FString Attacker, FString Victim) const;
	
	void ShowAnnouncement(bool bShowAnnouncement) const;

	void CreateEliminationAnnouncement();

	void ShowOverlay(bool bShowOverlay) const;
	
protected:
	
	virtual void BeginPlay() override;
	
	void CreateAnnouncement();
	
	void CreateOverlay();
	
private:

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);

	UFUNCTION()
	void EliminationAnnouncementTimerFinished(USEliminationAnnouncementWidget* AnnouncementToRemove);
	
public:
	
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};

