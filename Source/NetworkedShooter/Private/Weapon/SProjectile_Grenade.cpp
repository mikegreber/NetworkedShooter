// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SProjectile_Grenade.h"

#include "Character/SCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "PlayerController/SPlayerController.h"
#include "Sound/SoundCue.h"

ASProjectile_Grenade::ASProjectile_Grenade()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->InitialSpeed = 1500.f;
	ProjectileMovementComponent->MaxSpeed = 1500.f;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->bShouldBounce = true;
}

void ASProjectile_Grenade::Destroyed()
{
	if (HasAuthority())
	{
		if (ASCharacter* OwnerCharacter = GetInstigator<ASCharacter>())
		{
			// no server side rewind for grenades, apply damage on server only
			ApplyDamage(this, GetActorLocation(), nullptr, Damage, OwnerCharacter, this, DamageEffectClass, ECC_SkeletalMesh);
		}
	}
	
	Super::Destroyed();
}

void ASProjectile_Grenade::BeginPlay()
{
	AActor::BeginPlay();
	SpawnTrailSystem();
	StartDestroyTimer();

	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &ASProjectile_Grenade::OnBounce);
}

void ASProjectile_Grenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			BounceSound,
			GetActorLocation()
		);
	}
}
