// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/SProjectile.h"
#include "SProjectile_Grenade.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API ASProjectile_Grenade : public ASProjectile
{
	GENERATED_BODY()

public:
	ASProjectile_Grenade();

	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

private:
	UPROPERTY(EditAnywhere, Category = "Effects")
	USoundCue* BounceSound;
};
