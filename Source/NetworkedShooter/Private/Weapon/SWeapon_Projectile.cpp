// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SWeapon_Projectile.h"

#include "Character/SCharacter.h"
#include "Weapon/SProjectile.h"


void ASWeapon_Projectile::LocalFire(const FTransform& MuzzleTransform, const FVector_NetQuantize& HitTarget, bool bIsRewindFire, int8 Seed)
{
	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s ProjectileClass is null on %s"), __FUNCTIONW__, *GetName());
		return;
	}
	
	Super::LocalFire(MuzzleTransform, HitTarget, bIsRewindFire, Seed);
	
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	UWorld* World = GetWorld();
	
	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	const FVector ToTarget = HitTarget - MuzzleLocation;
	const FRotator TargetRotation = ToTarget.Rotation();
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (IsServerControlled()) // fired from server host, use replicated projectile
	{
		if (IsLocallyControlled())
		{
			ASProjectile* SpawnedProjectile = World->SpawnActor<ASProjectile>(ProjectileClass, MuzzleLocation, TargetRotation, SpawnParams);
			SpawnedProjectile->bUseServerSideRewind = false;
			SpawnedProjectile->Damage = Damage;
			SpawnedProjectile->HeadshotDamage = HeadshotDamage;
		}
	}
	else if (CanUseServerSideRewind())
	{
		if (HasAuthority()) // server, not host - spawn non-replicated projectile, use server-side rewind
		{
			ASProjectile* SpawnedProjectile = World->SpawnActor<ASProjectile>(ServerSideRewindProjectileClass, MuzzleLocation, TargetRotation, SpawnParams);
			SpawnedProjectile->bUseServerSideRewind = true;
		}
		else  // client
		{
			if (IsLocallyControlled()) // client, locally controlled - spawn non-replicated projectile, use server-side rewind
			{
				ASProjectile* SpawnedProjectile = World->SpawnActor<ASProjectile>(ServerSideRewindProjectileClass, MuzzleLocation, TargetRotation, SpawnParams);
				SpawnedProjectile->bUseServerSideRewind = true;
				SpawnedProjectile->TraceStart = MuzzleLocation;
				SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
			}
			else // client, not locally controlled - spawn non-replicated projectile, no server-side rewind
			{
				ASProjectile* SpawnedProjectile = World->SpawnActor<ASProjectile>(ServerSideRewindProjectileClass, MuzzleLocation, TargetRotation, SpawnParams);
				SpawnedProjectile->bUseServerSideRewind = false;
			}
		}
	}
	else if (HasAuthority()) // no server-side rewind, spawn server only
	{
		ASProjectile* SpawnedProjectile = World->SpawnActor<ASProjectile>(ProjectileClass, MuzzleLocation, TargetRotation, SpawnParams);
		SpawnedProjectile->bUseServerSideRewind = false;
		SpawnedProjectile->Damage = Damage;
		SpawnedProjectile->HeadshotDamage = HeadshotDamage;
	}
}
