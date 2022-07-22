// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SWeapon_Shotgun.h"

#include "Character/SCharacter.h"
#include "Components/SCombatComponent.h"
#include "Components/SLagCompensationComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Library/ShooterGameplayStatics.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "Particles/ParticleSystemComponent.h"
#include "PlayerController/SPlayerController.h"
#include "Sound/SoundCue.h"

void ASWeapon_Shotgun::Fire(FVector_NetQuantize HitTarget)
{
	if (CanFire())
	{
		bCanFire = false;

		OwnerComponent->PlayFireMontage();
		
		// seed for random scatter to replicate spread
		const int8 Seed = FMath::RoundToInt(GetWorld()->GetTimeSeconds());
		
		LocalFire(GetWeaponMesh()->GetSocketTransform("MuzzleFlash"), HitTarget, false, Seed);
		ServerFireWithSeed(HitTarget, Seed);
		
		StartFireTimer();
	}
}

void ASWeapon_Shotgun::LocalFire(const FTransform& MuzzleTransform, const FVector_NetQuantize& HitTarget, bool bIsRewindFire, int8 Seed)
{
	ASWeapon::LocalFire(MuzzleTransform, HitTarget, bIsRewindFire, Seed);
	
	const FVector TraceStart = MuzzleTransform.GetLocation();
	const ECollisionChannel TraceType = bIsRewindFire ? ECC_RewindTrace : ECC_Visibility;
	
	TMap<ASCharacter*, FHit> HitMap;
	TArray<FHitResult> FireHits;
	FireHits.Init(FHitResult(), NumberOfPellets);
	for (FHitResult& FireHit : FireHits)
	{
		WeaponTraceHit(TraceStart, TraceEndWithScatter(TraceStart, HitTarget), FireHit, TraceType);
		
		if (ASCharacter* HitCharacter = Cast<ASCharacter>(FireHit.GetActor()))
		{
			const FName BoneName = bIsRewindFire ? FireHit.GetComponent()->GetFName() : FireHit.BoneName;
			const FHit Hit(BoneName);
			
			if (HitMap.Contains(HitCharacter)) HitMap[HitCharacter] += Hit;
			else HitMap.Emplace(HitCharacter, Hit);
		}
	}
	
	if (!HitMap.IsEmpty())
	{
		if (HasAuthority())
		{   
			if ((IsServerControlled() && IsLocallyControlled()) || bIsRewindFire || !CanUseServerSideRewind())
			{
				// fired from server host, from rewind, or if rewind is disabled - apply damage on server
				ApplyDamage(HitMap);
			}
		}
		else if (IsLocallyControlled() && CanUseServerSideRewind()) // local client - use server-side rewind
		{
			ServerRewind(HitMap, TraceStart, HitTarget, Seed);
		}
	}

	if (!bIsRewindFire)
	{
		PlayFireEffects(MuzzleTransform, FireHits);
	}
}

void ASWeapon_Shotgun::PlayFireEffects(const FTransform& MuzzleTransform, const TArray<FHitResult>& FireHits) const
{
	for (const FHitResult& FireHit : FireHits)
	{
		if (BeamParticles)
		{
			if (UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				BeamParticles,
				MuzzleTransform.GetLocation(),
				FRotator::ZeroRotator,
				true
			))
			{
				Beam->SetVectorParameter(FName("Target"), FireHit.ImpactPoint);
			}
		}
	
		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticles,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation()
			);
		}
	
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				HitSound,
				FireHit.ImpactPoint,
				0.5f,
				FMath::FRandRange(-.5f, .5f)
			);
		}
	}
}

void ASWeapon_Shotgun::ApplyDamage(const TMap<ASCharacter*, FHit>& HitMap) const
{
	for (const auto& [HitCharacter, Hits] : HitMap)
	{
		if (HitCharacter)
		{
			UShooterGameplayStatics::ApplyGameplayEffect(OwnerCharacter, HitCharacter, DamageEffectClass, Damage * Hits.Body + HeadshotDamage * Hits.Head);
		}
	}
}

void ASWeapon_Shotgun::ServerRewind(const TMap<ASCharacter*, FHit>& HitMap, const FVector& TraceStart, const FVector_NetQuantize& HitTarget, int8 Seed)
{
	TArray<ASCharacter*> HitCharacters;
	HitMap.GetKeys(HitCharacters);
				
	OwnerCharacter->GetLagCompensationComponent()->ServerRewindHitTrace(
		HitCharacters,
		TraceStart,
		HitTarget,
		OwnerController->GetServerTime() - OwnerController->GetSingleTripTime(),
		this,
		Seed
	);
}