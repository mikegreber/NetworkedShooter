// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SProjectile_Bullet.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "Character/SCharacter.h"
#include "Components/SLagCompensationComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Library/ShooterGameplayStatics.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "PlayerController/SPlayerController.h"
#include "Weapon/SWeapon_Projectile.h"

ASProjectile_Bullet::ASProjectile_Bullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
	ProjectileMovementComponent->SetIsReplicated(true);
}

void ASProjectile_Bullet::ApplyDamage(const UObject* WorldContextObject, const FVector& Location, AActor* DamagedActor, float BaseDamage, IAbilitySystemInterface* Source, AActor* DamageCauser, TSubclassOf<UGameplayEffect> DamageEffect, ECollisionChannel DamageChannel) const
{
	UShooterGameplayStatics::ApplyGameplayEffect(Source, Cast<IAbilitySystemInterface>(DamagedActor), DamageEffect, BaseDamage);
}

void ASProjectile_Bullet::BeginPlay()
{
	Super::BeginPlay();
}

void ASProjectile_Bullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ASCharacter* HitCharacter = Cast<ASCharacter>(OtherActor))
	{
		if (const ASCharacter* OwnerCharacter = GetInstigator<ASCharacter>())
		{
			if (ASPlayerController* OwnerController = OwnerCharacter->GetPlayerController())
			{
				if (bUseServerSideRewind)
				{
					if (OwnerCharacter->IsLocallyControlled())
					{
						if (OwnerCharacter->HasAuthority()) // shot from local server
						{
							UShooterGameplayStatics::ApplyGameplayEffect(OwnerCharacter, HitCharacter, DamageEffectClass, Hit.BoneName == "Head" ? HeadshotDamage : Damage);
						}
						else  // shot from local client
						{
							OwnerCharacter->GetLagCompensationComponent()->ServerRewindHitProjectile(
								{ HitCharacter },
								TraceStart,
								InitialVelocity,
								OwnerController->GetServerTime() - OwnerController->GetSingleTripTime(),
								Cast<ASWeapon_Projectile>(GetOwner())
							);
						}
					}
				}
				else if (OwnerCharacter->HasAuthority()) // no rewind shot from server
				{
					UShooterGameplayStatics::ApplyGameplayEffect(OwnerCharacter, HitCharacter, DamageEffectClass, Hit.BoneName == "Head" ? HeadshotDamage : Damage);
				}
			}
		}
	}
	
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
