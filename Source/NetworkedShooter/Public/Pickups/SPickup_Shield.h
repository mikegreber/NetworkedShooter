// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/SPickup.h"
#include "SPickup_Shield.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API ASPickup_Shield : public ASPickup
{
	GENERATED_BODY()

protected:
	
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult) override;

private:

	UPROPERTY(EditAnywhere, Category = "Pickup | Properties")
	float ShieldAmount = 100.f;

	UPROPERTY(EditAnywhere, Category = "Pickup | Properties")
	float ShieldReplenishTime = 5.f;
};
