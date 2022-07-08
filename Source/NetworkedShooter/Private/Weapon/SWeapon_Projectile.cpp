// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SWeapon_Projectile.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/SProjectile.h"

void ASWeapon_Projectile::LocalFire(const FVector_NetQuantize& HitTarget)
{
	Super::LocalFire(HitTarget);

	if (!HasAuthority()) return;
	
	if (const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash"))
	{
		APawn* InstigatorPawn = Cast<APawn>(GetOwner());
		if (ProjectileClass && InstigatorPawn)
		{
			const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
			const FVector ToTarget = HitTarget - SocketTransform.GetLocation();
			const FRotator TargetRotation = ToTarget.Rotation();
			
			if (UWorld* World = GetWorld())
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = GetOwner();
				SpawnParams.Instigator = InstigatorPawn;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				
				World->SpawnActor<ASProjectile>(
					ProjectileClass,
					SocketTransform.GetLocation(),
					TargetRotation,
					SpawnParams
				);
			}
		}
	}
}
