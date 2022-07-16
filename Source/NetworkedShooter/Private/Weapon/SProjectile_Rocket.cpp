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

	if (!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &ASProjectile_Rocket::OnHit);
	}
	
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
						ApplyDamage(this, GetActorLocation(), OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
					}
					else // shot from local client
					{
						TArray<ASCharacter*> HitCharacters;
						UShooterGameplayStatics::GetActorsInRadius(this, GetActorLocation(), DamageOuterRadius, HitCharacters);
					
						OwnerCharacter->GetLagCompensationComponent()->ServerRewindHitProjectile(
							HitCharacters,
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
				ApplyDamage(this, GetActorLocation(), OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
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


void ASProjectile_Rocket::Destroyed()
{
}
