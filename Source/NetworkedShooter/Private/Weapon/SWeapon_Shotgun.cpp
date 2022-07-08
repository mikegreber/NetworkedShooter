// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SWeapon_Shotgun.h"

#include "Character/SCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void ASWeapon_Shotgun::Fire(FVector_NetQuantize HitTarget)
{
	if (CanFire())
	{
		bCanFire = false;

		// seed for random scatter to replicate spread
		const int8 Seed = FMath::RoundToInt(GetWorld()->GetTimeSeconds());
		
		LocalFireWithSeed(HitTarget, Seed);
		ServerFireWithSeed(HitTarget, Seed);
		
		StartFireTimer();
	}
}

void ASWeapon_Shotgun::LocalFireWithSeed(const FVector_NetQuantize& HitTarget, int8 Seed)
{
	ASWeapon::LocalFire(const_cast<FVector_NetQuantize&>(HitTarget));
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (!MuzzleFlashSocket)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s MuzzleFlash socket not found on weapon mesh (%s)"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR)
		return;
	}
	
	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();
	
	// seed random so shots are the same on all machines
	FMath::RandInit(Seed);

	TMap<ASCharacter*, uint32> HitMap;
	for (uint32 i = 0; i < NumberOfPellets; ++i)
	{
		FHitResult FireHit;
		WeaponTraceHit(TraceStart, TraceEndWithScatter(TraceStart, HitTarget), FireHit);

		// collect hits on server
		if (HasAuthority())
		{
			if (ASCharacter* HitCharacter = Cast<ASCharacter>(FireHit.GetActor()))
			{
				if (HitMap.Contains(HitCharacter)) ++HitMap[HitCharacter];
				else HitMap.Emplace(HitCharacter, 1);
			}
		}

		// play effects on all machines
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

	// apply damage on server
	if (HasAuthority())
	{
		if (AController* InstigatorController = OwnerPawn->GetController())
		{
			for (auto HitPair : HitMap)
			{
				if (HitPair.Key)
				{
					UGameplayStatics::ApplyDamage(
						HitPair.Key,
						Damage * HitPair.Value,
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
			}
		}
	}
	
}

void ASWeapon_Shotgun::ServerFireWithSeed_Implementation(const FVector_NetQuantize& HitTarget, int8 Seed)
{
	MulticastFireWithSeed(HitTarget, Seed);
}

void ASWeapon_Shotgun::MulticastFireWithSeed_Implementation(const FVector_NetQuantize& HitTarget, int8 Seed)
{
	if (!IsLocallyControlled()) LocalFireWithSeed(HitTarget, Seed);
}
