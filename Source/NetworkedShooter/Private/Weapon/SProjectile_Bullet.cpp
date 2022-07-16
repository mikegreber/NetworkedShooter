// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SProjectile_Bullet.h"

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

void ASProjectile_Bullet::ApplyDamage(const UObject* WorldContextObject, const FVector& Location, AActor* DamagedActor, float BaseDamage, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass, ECollisionChannel DamageChannel) const
{
	UShooterGameplayStatics::ApplyDamage(DamagedActor, BaseDamage, EventInstigator, DamageCauser, UDamageType::StaticClass());
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
							ApplyDamage(
								this,
								GetActorLocation(),
								HitCharacter,
								Damage, OwnerController,
								this,
								UDamageType::StaticClass()
							);
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
					ApplyDamage(
						this,
						 GetActorLocation(),
						 HitCharacter,
						 Damage,
						 OwnerController,
						 this,
						 UDamageType::StaticClass()
					);
				}
			}
		}
	}
	
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
