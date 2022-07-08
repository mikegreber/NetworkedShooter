// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/SPickup.h"
#include "SPickup_Health.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API ASPickup_Health : public ASPickup
{
	GENERATED_BODY()

public:
	ASPickup_Health();

protected:
	
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult) override;

private:

	UPROPERTY(EditAnywhere, Category = "Pickup | Properties")
	float HealAmount = 100.f;

	UPROPERTY(EditAnywhere, Category = "Pickup | Properties")
	float HealingTime = 5.f;
};
