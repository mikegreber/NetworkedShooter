// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SWeapon_HitScan.h"
#include "AbilitySystem/SAbilitySystemComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Character/SCharacter.h"
#include "Components/SLagCompensationComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Library/ShooterGameplayStatics.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "Particles/ParticleSystemComponent.h"
#include "PlayerController/SPlayerController.h"
#include "Sound/SoundCue.h"


void ASWeapon_HitScan::LocalFire(const FTransform& MuzzleTransform, const FVector_NetQuantize& HitTarget, bool bIsRewindFire, int8 Seed)
{
	Super::LocalFire(MuzzleTransform, HitTarget, bIsRewindFire, Seed);

	const FVector TraceStart = MuzzleTransform.GetLocation();
	const ECollisionChannel TraceType = bIsRewindFire ? ECC_RewindTrace : ECC_Visibility;

	FHitResult FireHit;
	WeaponTraceHit(TraceStart, HitTarget, FireHit, TraceType);

	if (ASCharacter* HitCharacter = Cast<ASCharacter>(FireHit.GetActor()))
	{
		if (HasAuthority())
		{   
			if ((IsServerControlled() && IsLocallyControlled()) || bIsRewindFire || !CanUseServerSideRewind())
			{
				const FName BoneName = bIsRewindFire ? FireHit.GetComponent()->GetFName() : FireHit.BoneName;
				UShooterGameplayStatics::ApplyGameplayEffect(OwnerCharacter, HitCharacter, DamageEffectClass, BoneName == "Head" ? HeadshotDamage : Damage);
			}
		}
		else if (IsLocallyControlled() && CanUseServerSideRewind()) // local client - use server-side rewind
		{
			UE_LOG(LogTemp, Warning, TEXT("Server Rewind"))
			ServerRewind(HitCharacter, TraceStart, HitTarget);
		}
	}

	if (!bIsRewindFire)
	{
		PlayFireEffects(MuzzleTransform, FireHit);
	}
}

void ASWeapon_HitScan::PlayFireEffects(const FTransform& MuzzleTransform, const FHitResult& FireHit)
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
			FireHit.ImpactPoint
		);
	}
	
	if (MuzzleFlash)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			MuzzleFlash,
			MuzzleTransform
		);
	}
	
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			FireSound,
			GetActorLocation()
		);
	}
}

void ASWeapon_HitScan::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit, ECollisionChannel CollisionChannel) const
{
	if (const UWorld* World = GetWorld())
	{
		const FVector TraceEnd = TraceStart + (HitTarget - TraceStart) * 1.25f;
		
		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			TraceEnd,
			CollisionChannel
		);

		if (!OutHit.bBlockingHit) OutHit.ImpactPoint = TraceEnd;

#if WITH_EDITOR
		switch(CVarDebugWeaponTrace.GetValueOnGameThread())
		{
		case 2:
			{
				DrawDebugSphere(
					GetWorld(),
					OutHit.ImpactPoint,
					CollisionChannel == ECC_RewindTrace ? 14.f : 12.f,
					12,
					CollisionChannel == ECC_RewindTrace ? FColor::Green : FColor::Yellow,
					false,
					10
				);
			}
		case 1:
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("%s %s %d"),
					__FUNCTIONW__,
					OutHit.GetComponent() ? *OutHit.GetComponent()->GetName() : TEXT("No Hit"),
					CollisionChannel == ECC_RewindTrace ? TEXT("Rewind Trace") : TEXT("Visibility Trace")
				)
			}
		default: break;
		}
#endif
		
	}
}

void ASWeapon_HitScan::ApplyDamage(ASCharacter* HitCharacter, FName BoneName)
{
	if (HitCharacter)
	{
		UAbilitySystemComponent* OwnerASC = OwnerCharacter->GetAbilitySystemComponent();
		FGameplayEffectContextHandle ContextHandle = OwnerASC->MakeEffectContext();
		
		FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(DamageEffectClass, BoneName == "Head" ? HeadshotDamage : Damage, ContextHandle);

		if (SpecHandle.Data.IsValid())
		{
			OwnerASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), HitCharacter->GetAbilitySystemComponent());
		}
	}
}

void ASWeapon_HitScan::ServerRewind(ASCharacter* HitCharacter, const FVector& TraceStart, const FVector_NetQuantize& HitTarget)
{
	OwnerCharacter->GetLagCompensationComponent()->ServerRewindHitTrace(
		{ HitCharacter },
		TraceStart,
		HitTarget,
		OwnerController->GetServerTime() - OwnerController->GetSingleTripTime(),
		this
	);
}