// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/SWeapon.h"
#include "SWeapon_Projectile.generated.h"

UCLASS()
class NETWORKEDSHOOTER_API ASWeapon_Projectile : public ASWeapon
{
	GENERATED_BODY()
protected:

	UPROPERTY(EditAnywhere, Category = "WeaponProperties | Projectile")
	TSubclassOf<class ASProjectile> ProjectileClass;
	
	UPROPERTY(EditAnywhere, Category = "WeaponProperties | Projectile")
	TSubclassOf<ASProjectile> ServerSideRewindProjectileClass;
	
	virtual void LocalFire(const FTransform& MuzzleTransform, const FVector_NetQuantize& HitTarget, bool bIsRewindFire = false, int8 Seed = 0) override;

public:
	FORCEINLINE TSubclassOf<ASProjectile> GetProjectileClass() const { return ProjectileClass; }
};
