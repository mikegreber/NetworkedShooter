// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SBulletCasing.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

ASBulletCasing::ASBulletCasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	CasingMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);
	SetRootComponent(CasingMesh);

	ShellEjectionImpulse = 10.f;

	bHasHit = false;
}

void ASBulletCasing::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	CasingMesh->OnComponentHit.AddDynamic(this, &ASBulletCasing::OnHit);
}

void ASBulletCasing::BeginPlay()
{
	Super::BeginPlay();
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);
	CasingMesh->AddTorqueInDegrees(FMath::VRand() * 5000.f, FName(), true);
}

void ASBulletCasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bHasHit)
	{
		bHasHit = true;

		if (ShellSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShellSound, GetActorLocation());
		}
		
		SetLifeSpan(1.f);
	}
}

