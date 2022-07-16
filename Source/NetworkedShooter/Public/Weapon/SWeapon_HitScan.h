// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/SWeapon.h"
#include "SWeapon_HitScan.generated.h"


UCLASS()
class NETWORKEDSHOOTER_API ASWeapon_HitScan : public ASWeapon
{
	GENERATED_BODY()

protected:
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Effects | Firing")
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Effects | Firing")
	UParticleSystem* BeamParticles;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Effects | Firing")
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Effects | Firing")
	USoundCue* FireSound;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Effects | Firing")
	USoundCue* HitSound;

	virtual void LocalFire(const FTransform& MuzzleTransform, const FVector_NetQuantize& HitTarget, bool bIsRewindFire, int8 Seed) override;

	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit, ECollisionChannel CollisionChannel = ECC_Visibility) const;

	void ApplyDamage(ASCharacter* HitCharacter, FName BoneName);
	
	void ServerRewind(ASCharacter* HitCharacter, const FVector& TraceStart, const FVector_NetQuantize& HitTarget);

private:
	
	void PlayFireEffects(const FTransform& MuzzleTransform, const FHitResult& FireHit);
};
