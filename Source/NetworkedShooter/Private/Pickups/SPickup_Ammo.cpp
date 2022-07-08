// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/SPickup_Ammo.h"

#include "Character/SCharacter.h"
#include "Components/SCombatComponent.h"

void ASPickup_Ammo::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (const ASCharacter* Character = Cast<ASCharacter>(OtherActor))
	{
		if (USCombatComponent* CombatComponent = Character->GetCombatComponent())
		{
			CombatComponent->PickupAmmo(WeaponType, AmmoAmount);

			Destroy();
		}
	}
}
