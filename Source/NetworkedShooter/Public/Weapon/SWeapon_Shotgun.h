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

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Weapon Scatter")
	uint32 NumberOfPellets = 10;
	
public:
	virtual void Fire(FVector_NetQuantize HitTarget) override;
	
protected:
	
	virtual void LocalFire(const FTransform& MuzzleTransform, const FVector_NetQuantize& HitTarget, bool bIsRewindFire, int8 Seed = 0) override;
	
	void ApplyDamage(const TMap<ASCharacter*, uint32>& HitMap);
	
	void ServerRewind(const TMap<ASCharacter*, uint32>& HitMap, const FVector& TraceStart, const FVector_NetQuantize& HitTarget, int8 Seed);

private:
	
	void PlayFireEffects(const FTransform& MuzzleTransform, const TArray<FHitResult>& FireHits) const;

};
