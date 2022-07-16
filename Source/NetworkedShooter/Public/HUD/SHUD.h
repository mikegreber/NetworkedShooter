// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Types/HUDPackage.h"
#include "SHUD.generated.h"

// DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWidgetsCreated);

UCLASS()
class NETWORKEDSHOOTER_API ASHUD : public AHUD
{
	GENERATED_BODY()

public:
	
	virtual void DrawHUD() override;

	UFUNCTION()
	void UpdateKills(int32 NewValue);

	UFUNCTION()
	void UpdateDeaths(int32 NewValue);
	
	// void BindDelegates(ASPlayerState* AsPlayerState);

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class USCharacterOverlay> CharacterOverlayClass;
	UPROPERTY()
	USCharacterOverlay* CharacterOverlay;
	
	UPROPERTY(EditAnywhere, Category = "Announcements")
	TSubclassOf<class USAnnouncementWidget> AnnouncementClass;
	UPROPERTY()
	class USAnnouncementWidget* Announcement;

	void Initialize();

	void AddEliminationAnnouncement(FString Attacker, FString Victim);
	
	void ShowAnnouncement(bool bShowAnnouncement);

	void CreateEliminationAnnouncement();

	// call before accessing CharacterOverlay

	void ShowOverlay(bool bShowOverlay);
	
protected:
	virtual void BeginPlay() override;
	
	void CreateAnnouncement();
	void CreateOverlay();
	
private:
	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;

	UPROPERTY(EditAnywhere, Category = "Elimination")
	TSubclassOf<class USEliminationAnnouncementWidget> EliminationAnnouncementClass;
	UPROPERTY() USEliminationAnnouncementWidget* EliminationAnnouncement;

	UPROPERTY(EditAnywhere, Category = "Elimination")
	float EliminationAnnouncementTime = 1.5f;

	UFUNCTION()
	void EliminationAnnouncementTimerFinished(USEliminationAnnouncementWidget* AnnouncementToRemove);
	
public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};

