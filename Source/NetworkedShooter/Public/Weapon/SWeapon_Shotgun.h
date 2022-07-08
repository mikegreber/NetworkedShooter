// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/SWeapon_HitScan.h"
#include "SWeapon_Shotgun.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API ASWeapon_Shotgun : public ASWeapon_HitScan
{
	GENERATED_BODY()

public:
	virtual void Fire(FVector_NetQuantize HitTarget) override;
	
protected:
	void LocalFireWithSeed(const FVector_NetQuantize& HitTarget, int8 Seed);
	
	UFUNCTION(Server, Reliable)
	void ServerFireWithSeed(const FVector_NetQuantize& HitTarget, int8 Seed);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFireWithSeed(const FVector_NetQuantize& HitTarget, int8 Seed);
private:

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Weapon Scatter")
	uint32 NumberOfPellets = 10;
};
