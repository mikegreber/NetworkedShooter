// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/SPickup_Shield.h"

#include "Character/SCharacter.h"
#include "Components/SBuffComponent.h"

void ASPickup_Shield::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (const ASCharacter* Character = Cast<ASCharacter>(OtherActor))
	{
		if (USBuffComponent* BuffComponent = Character->GetBuffComponent())
		{
			BuffComponent->ReplenishShield(ShieldAmount, ShieldReplenishTime);
			Destroy();
		}
	}
}
