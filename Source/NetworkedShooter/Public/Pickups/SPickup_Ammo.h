// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/SPickup.h"
#include "Weapon/SWeaponTypes.h"
#include "SPickup_Ammo.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API ASPickup_Ammo : public ASPickup
{
	GENERATED_BODY()

protected:
	
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult) override;

private:
	UPROPERTY(EditAnywhere, Category = "Pickup")
	int32 AmmoAmount = 30;

	UPROPERTY(EditAnywhere, Category = "Pickup")
	EWeaponType WeaponType;
};
