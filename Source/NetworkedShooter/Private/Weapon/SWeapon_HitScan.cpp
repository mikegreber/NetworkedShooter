// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SWeapon_HitScan.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Character/SCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "PlayerController/SPlayerController.h"
#include "Sound/SoundCue.h"

static TAutoConsoleVariable CVarDebugAbilities(TEXT("ns.DrawWeaponTraceHit"), false, TEXT("Show trace hit from WeaponTraceHit"), ECVF_Cheat);

void ASWeapon_HitScan::LocalFire(const FVector_NetQuantize& HitTarget)
{
	Super::LocalFire(HitTarget);
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;
	
	if (const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash"))
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector TraceStart = SocketTransform.GetLocation();
		
		FHitResult FireHit;
		WeaponTraceHit(TraceStart, HitTarget, FireHit);
		
		if (HasAuthority() && OwnerController)
		{
			if (ASCharacter* HitCharacter = Cast<ASCharacter>(FireHit.GetActor()))
			{
				UGameplayStatics::ApplyDamage(
					HitCharacter,
					Damage,
					OwnerController,
					this,
					UDamageType::StaticClass()
				);
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
				SocketTransform
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
	
}

void ASWeapon_HitScan::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit) const
{
	if (const UWorld* World = GetWorld())
	{
		const FVector TraceEnd = TraceStart + (HitTarget - TraceStart) * 1.25f;
		
		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			TraceEnd,
			ECC_Visibility
		);

		const FVector BeamEnd = OutHit.bBlockingHit ? OutHit.ImpactPoint : TraceEnd;

		#if !UE_BUILD_SHIPPING
		if (CVarDebugAbilities.GetValueOnGameThread()) DrawDebugSphere(GetWorld(), BeamEnd, 12.f, 12, FColor::Red, true);
		#endif	
		
		if (BeamParticles)
		{
			if (UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator,
				true
			))
			{
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
		
	}
}

