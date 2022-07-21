// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SProjectile_Rocket.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/SCharacter.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SLagCompensationComponent.h"
#include "Library/ShooterGameplayStatics.h"
#include "PlayerController/SPlayerController.h"
#include "Sound/SoundCue.h"
#include "Weapon/SRocketMovementComponent.h"
#include "Weapon/SWeapon_Projectile.h"

ASProjectile_Rocket::ASProjectile_Rocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	ProjectileMovementComponent = CreateDefaultSubobject<USRocketMovementComponent>(TEXT("RocketMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->InitialSpeed = 1500.f;
	ProjectileMovementComponent->MaxSpeed = 1500.f;
	ProjectileMovementComponent->ProjectileGravityScale = 0.f;
	ProjectileMovementComponent->SetIsReplicated(true);
}


void ASProjectile_Rocket::BeginPlay()
{
	Super::BeginPlay();
	
	SpawnTrailSystem();

	if (ProjectileLoop && LoopingSoundAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			LoopingSoundAttenuation,
			nullptr,
			false
		);
	}
}

void ASProjectile_Rocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner()) return;
	
	if (ASCharacter* OwnerCharacter = GetInstigator<ASCharacter>())
	{
		if (const ASPlayerController* OwnerController = OwnerCharacter->GetPlayerController())
		{
			if (OwnerCharacter->HasAuthority())
			{
				// fired from server host or from server with no server-side rewind
				if ((OwnerCharacter->IsServerControlled() && OwnerCharacter->IsLocallyControlled()) || !bUseServerSideRewind)
				{
					ApplyDamage(this, GetActorLocation(), OtherActor, Damage, OwnerCharacter, this, DamageEffectClass);
				}
			}
			else if (OwnerCharacter->IsLocallyControlled() && bUseServerSideRewind) // fired from local client with server-side rewind enabled - use server-side rewind
			{
				ServerRewind(OwnerCharacter, OwnerController);
			}
		}
	}
	
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, GetActorLocation());
	}
	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
	}
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstanceController())
	{
		TrailSystemComponent->GetSystemInstanceController()->Deactivate();
	}
	if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}

	StartDestroyTimer();
}

void ASProjectile_Rocket::ServerRewind(const ASCharacter* OwnerCharacter, const ASPlayerController* OwnerController) const
{
	TArray<ASCharacter*> HitCharacters;
	UShooterGameplayStatics::GetActorsInRadius(this, GetActorLocation(), DamageOuterRadius, HitCharacters);
					
	OwnerCharacter->GetLagCompensationComponent()->ServerRewindHitProjectile(
		HitCharacters,
		TraceStart,
		InitialVelocity,
		OwnerController->GetServerTime() - OwnerController->GetSingleTripTime(),
		GetOwner<ASWeapon_Projectile>()
	);
}

void ASProjectile_Rocket::Destroyed()
{
}
