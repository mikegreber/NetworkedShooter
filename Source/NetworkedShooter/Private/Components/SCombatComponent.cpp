// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SCombatComponent.h"
#include "Camera/CameraComponent.h"
#include "Character/SCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HUD/SHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "PlayerController/SPlayerController.h"
#include "Weapon/SProjectile.h"
#include "Weapon/SWeapon.h"

USCombatComponent::USCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	
	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;

	CarriedAmmoMap = TMap<EWeaponType, int32>({
		{EWeaponType::EWT_Pistol, 30},
		{EWeaponType::EWT_SubmachineGun, 20},
		{EWeaponType::EWT_AssaultRifle, 30},
		{EWeaponType::EWT_Shotgun, 12},
		{EWeaponType::EWT_SniperRifle, 2},
		{EWeaponType::EWT_GrenadeLauncher, 4},
		{EWeaponType::EWT_RocketLauncher, 4},
	});
}

void USCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USCombatComponent, EquippedWeapon);
	DOREPLIFETIME(USCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(USCombatComponent, bAiming);
	DOREPLIFETIME(USCombatComponent, CombatState);
	DOREPLIFETIME_CONDITION(USCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(USCombatComponent, Grenades, COND_OwnerOnly);
}

void USCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}

		Character->ReceiveControllerChangedDelegate.AddDynamic(this, &USCombatComponent::OnControllerChanged);
	}
}

void USCombatComponent::OnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	Controller = Cast<ASPlayerController>(NewController);
	if (Controller)
	{
		OnControllerSet.Broadcast();
		OnControllerSet.Clear();
	}
}

void USCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;
		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void USCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	
	if (FVector TraceStart, TraceDirection;
		UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		ViewportSize / 2.f,
		TraceStart,
		TraceDirection
	))
	{
		if (Character)
		{
			const float DistanceToCharacter = (TraceStart - Character->GetActorLocation()).Size();
			TraceStart += TraceDirection * (DistanceToCharacter + 100.f);
		}
		const FVector TraceEnd = TraceStart + TraceDirection * TRACE_LENGTH;
		
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			TraceStart,
			TraceEnd,
			ECC_Visibility
		);
		
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = TraceEnd;
		}

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<USCrosshairsInteractionInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
	}
}

void USCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
		if (Character && Character->Controller)
		{
			Controller = Controller ? Controller : Cast<ASPlayerController>(Character->Controller);
			if (Controller)
			{
				HUD = HUD == nullptr ? Cast<ASHUD>(Controller->GetHUD()) : HUD;
				if (HUD)
				{
					if (EquippedWeapon)
					{
						HUDPackage.SetCrosshairs(EquippedWeapon->GetHUDPackage());
						
						FVector Velocity = Character->GetVelocity();
						Velocity.Z = 0;
						
						const FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
						const FVector2D VelocityMultiplierRange(0.f, 1.f);

						CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

						if (Character->GetCharacterMovement()->IsFalling())
						{
							CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25);
						}
						else
						{
							CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
						}
						
						if (bAiming)
						{
							CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
						}
						else
						{
							CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
						}

						CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);
						
						HUDPackage.CrosshairSpread =
							0.5f +
							CrosshairVelocityFactor +
							CrosshairInAirFactor -
							CrosshairAimFactor +
							CrosshairShootingFactor;
					}
					
					HUD->SetHUDPackage(HUDPackage);
				}
			}
		}
}

void USCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon)
	{
		if (bAiming)
		{
			CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
		}
		else
		{
			CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
		}
	}
	
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

bool USCombatComponent::CanFire() const
{
	return CombatState == ESCombatState::ECS_Unoccupied;
}

void USCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed)
	{
		FireWeapon();
	}
}

void USCombatComponent::FireWeapon()
{
	EquippedWeapon->Fire(HitTarget);
	CrosshairShootingFactor = 1.f;
}

void USCombatComponent::EquipWeapon(ASWeapon* WeaponToEquip)
{
	SERVER_ONLY();

	if (WeaponToEquip == nullptr || CombatState != ESCombatState::ECS_Unoccupied) return;
	
	if (EquippedWeapon && SecondaryWeapon == nullptr)
	{
		EquipSecondaryWeapon(EquippedWeapon);
		EquipPrimaryWeapon(WeaponToEquip);
	}
	else if (EquippedWeapon && SecondaryWeapon)
	{
		DropWeapon(EquippedWeapon);
		EquipPrimaryWeapon(WeaponToEquip);
	}
	else
	{
		EquipPrimaryWeapon(WeaponToEquip);
	}
}

void USCombatComponent::EquipPrimaryWeapon(ASWeapon* WeaponToEquip)
{
	if (Character && WeaponToEquip)
	{
		ASWeapon* OldEquippedWeapon = EquippedWeapon;
		
		EquippedWeapon = WeaponToEquip;
		OnRep_EquippedWeapon(OldEquippedWeapon);
	}
}

void USCombatComponent::OnRep_EquippedWeapon(ASWeapon* OldEquippedWeapon)
{
	UpdateCarriedAmmo();
	
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->Equip(Character);

		if (EquippedWeapon->IsEmpty())
		{
			ReloadWeapon();
		}
	}
}

void USCombatComponent::EquipSecondaryWeapon(ASWeapon* WeaponToEquip)
{
	ASWeapon* OldSecondaryWeapon = SecondaryWeapon;
	SecondaryWeapon = WeaponToEquip;
	OnRep_SecondaryWeapon(OldSecondaryWeapon);
}

void USCombatComponent::OnRep_SecondaryWeapon(ASWeapon* OldSecondaryWeapon)
{
	if (SecondaryWeapon)
	{
		AttachActorToBackpack(SecondaryWeapon);
		SecondaryWeapon->Holster();
	}
	
	if (OldSecondaryWeapon)
	{
		EquipPrimaryWeapon(OldSecondaryWeapon);
	}
}

void USCombatComponent::SwapWeapons()
{
	if (CombatState == ESCombatState::ECS_Unoccupied && EquippedWeapon && SecondaryWeapon)
	{
		ServerSwapWeapons();
	}
}

void USCombatComponent::ServerSwapWeapons_Implementation()
{
	if (CombatState == ESCombatState::ECS_Unoccupied && EquippedWeapon && SecondaryWeapon)
	{
		EquipSecondaryWeapon(EquippedWeapon);
	}
}

void USCombatComponent::ReloadWeapon()
{
	if (CarriedAmmo > 0 && CombatState == ESCombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsFull())
	{
		ServerReloadWeapon();
	}
}

void USCombatComponent::ServerReloadWeapon_Implementation()
{
	if (Character && EquippedWeapon)
	{
		SetCombatState(ESCombatState::ECS_Reloading);
	}
}

void USCombatComponent::FinishReloading()
{
	if (Character && Character->HasAuthority())
	{
		CombatState = ESCombatState::ECS_Unoccupied;

		if (EquippedWeapon)
		{
			const int32 Amount = ReloadAmount();
			if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
			{
				CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= Amount;
				UpdateCarriedAmmo();
			}
			EquippedWeapon->AddAmmo(Amount);
		}
	}
	
	if (bFireButtonPressed)
	{
		FireWeapon();
	}
}

int32 USCombatComponent::ReloadAmount()
{
	if (EquippedWeapon)
	{
		const int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
		if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
		{
			const int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
			const int32 Least = FMath::Min(RoomInMag, AmountCarried);
			return FMath::Clamp(RoomInMag, 0, Least);
		}
	}
	
	return 0;
}

void USCombatComponent::DropWeapon(ASWeapon*& WeaponToDrop)
{
	if (WeaponToDrop)
	{
		WeaponToDrop->Drop();
		WeaponToDrop = nullptr;
	}
}

void USCombatComponent::SetAiming(bool bIsAiming)
{
	if (Character && EquippedWeapon && bAiming != bIsAiming)
	{
		bAiming = bIsAiming;
	
		if (Character->HasAuthority())
		{
			if (Character)
			{
				Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
			}

			if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
			{
				Character->ShowSniperScopeWidget(bIsAiming);
			}
		}
		else
		{
			ServerSetAiming(bIsAiming);
		}
	}
}

void USCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	SetAiming(bIsAiming);
}

void USCombatComponent::ThrowGrenade()
{
	UE_LOG(LogTemp, Warning, TEXT("%s %s"), __FUNCTIONW__, *NET_ROLE_STRING_COMPONENT)

	if (Grenades > 0 && CombatState == ESCombatState::ECS_Unoccupied && EquippedWeapon)
	{
		ServerThrowGrenade();
		SetCombatState(ESCombatState::ECS_ThrowingGrenade);
	}
}

void USCombatComponent::ServerThrowGrenade_Implementation()
{
	if (Grenades > 0 && CombatState == ESCombatState::ECS_Unoccupied && EquippedWeapon)
	{
		SetGrenades(Grenades-1);
		SetCombatState(ESCombatState::ECS_ThrowingGrenade);
	}
}

void USCombatComponent::ShowAttachedGrenade(bool bShowGrenade) const
{
	if (Character && Character->GetAttachedGrenade())
	{
		Character->GetAttachedGrenade()->SetVisibility(bShowGrenade);
	}
}

void USCombatComponent::LaunchGrenade()
{
	UE_LOG(LogTemp, Warning, TEXT("%s %s"), __FUNCTIONW__, *NET_ROLE_STRING_COMPONENT);

	ShowAttachedGrenade(false);

	if (Character && Character->IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTarget);
	}
}

void USCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	UE_LOG(LogTemp, Warning, TEXT("%s %s"), __FUNCTIONW__, *NET_ROLE_STRING_COMPONENT);

	if (GrenadeClass && Character && Character->GetAttachedGrenade())
	{
		const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation();
		const FVector ToTarget = Target - StartingLocation;
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		if (UWorld* World = GetWorld())
		{
			World->SpawnActor<ASProjectile>(
				GrenadeClass,
				StartingLocation,
				ToTarget.Rotation(),
				SpawnParams
			);
		}
	}
}

void USCombatComponent::ThrowGrenadeFinished()
{
	if (Character->HasAuthority())
	{
		SetCombatState(ESCombatState::ECS_Unoccupied);
	}
	
	AttachActorToRightHand(EquippedWeapon);
}

void USCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmoAmount);
		UpdateCarriedAmmo();
	}
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && WeaponType == EquippedWeapon->GetWeaponType())
	{
		ReloadWeapon();
	}
}

void USCombatComponent::UpdateCarriedAmmo()
{
	if (Controller)
	{
		if (EquippedWeapon)
		{
			if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
			{
				CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
				if (Controller->HasLocalAuthority()) OnRep_CarriedAmmo();
			}
		}
	}
	else
	{
		OnControllerSet.AddUniqueDynamic(this, &USCombatComponent::UpdateCarriedAmmo);
	}
}

void USCombatComponent::OnRep_CarriedAmmo() const
{
	if (Character->IsLocallyControlled())
	{
		OnCarriedAmmoUpdated.Broadcast(CarriedAmmo);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Called %s on non-local controller (%s)"), __FUNCTIONW__, *NET_ROLE_STRING_COMPONENT);
	}
}

void USCombatComponent::SetGrenades(int32 NewGrenades)
{
	Grenades = FMath::Clamp(NewGrenades, 0, MaxGrenades);
	if (Character->IsLocallyControlled()) OnRep_CarriedGrenades();
}

void USCombatComponent::OnRep_CarriedGrenades() const
{
	if (Character->IsLocallyControlled())
	{
		OnGrenadesUpdated.Broadcast(Grenades);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Called %s on non-local controller (%s)"), __FUNCTIONW__, *NET_ROLE_STRING_COMPONENT);
	}
}

void USCombatComponent::SetCombatState(ESCombatState NewCombatState)
{
	if (CombatState != NewCombatState)
	{
		CombatState = NewCombatState;
		OnRep_CombatState();
	}
}

void USCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ESCombatState::ECS_Unoccupied:
		{
			if (bFireButtonPressed) FireWeapon();
			break;
		}
	case ESCombatState::ECS_Reloading:
		{
			if (Character) Character->PlayReloadMontage();
			break;
		}
	case ESCombatState::ECS_ThrowingGrenade:
		{
			if (Character)
			{
				Character->PlayThrowGrenadeMontage();
				AttachActorToLeftHand(EquippedWeapon);
				ShowAttachedGrenade(true);
			}
			break;
		}
	default:;
	}
}

void USCombatComponent::AttachActorToRightHand(AActor* ActorToAttach) const
{
	if (Character && ActorToAttach)
	{
		ActorToAttach->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "RightHandSocket");
	}
}

void USCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach) const
{
	if (Character && ActorToAttach)
	{
		ActorToAttach->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "LeftHandSocket");
	}
}

void USCombatComponent::AttachActorToBackpack(AActor* ActorToAttach) const
{
	if (Character && ActorToAttach)
	{
		ActorToAttach->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "BackpackSocket");
	}
}
