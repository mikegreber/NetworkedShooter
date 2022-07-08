// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SProjectile.h"

#include "NiagaraFunctionLibrary.h"
#include "Character/SCharacter.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "Sound/SoundCue.h"

ASProjectile::ASProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECR_Block);
	SetRootComponent(CollisionBox);
}

void ASProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	// if (HasAuthority())
	// {
	// 	CollisionBox->OnComponentHit.AddDynamic(this, &ASProjectile::OnHit);
	// }
}

void ASProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(DestroyTimer, this, &ASProjectile::DestroyTimerFinished, DestroyTime);
}

void ASProjectile::DestroyTimerFinished()
{
	Destroy();
}

void ASProjectile::Destroyed()
{
	Super::Destroyed();
	
	if (ImpactParticles)
    {
    	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
    }
    
    if (ImpactSound)
    {
    	UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, GetActorLocation());
    }
}

void ASProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			RootComponent,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}

	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &ASProjectile::OnHit);
	}
}


void ASProjectile::SpawnTrailSystem()
{
	if (TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
}

void ASProjectile::ExplodeDamage()
{
	if (HasAuthority())
	{
		if (const APawn* FiringPawn = GetInstigator())
		{
			if (AController* FiringController = FiringPawn->GetController())
			{
				UGameplayStatics::ApplyRadialDamageWithFalloff(
					this,
					Damage,
					10.f,
					GetActorLocation(),
					DamageInnerRadius,
					DamageOuterRadius,
					1.f,
					UDamageType::StaticClass(),
					TArray<AActor*>(),
					this,
					FiringController
				);
			}
		}
	}
}

void ASProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	Destroy();
}



void ASProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

