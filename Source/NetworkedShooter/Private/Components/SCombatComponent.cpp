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
	PrimaryComponentTick.SetTickFunctionEnable(false);
	
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

void USCombatComponent::SetOwnerCharacter(ASCharacter* NewCharacter)
{
	if (NewCharacter && NewCharacter != OwnerCharacter)
	{
		OwnerCharacter = NewCharacter;
		OwnerCharacter->OnEliminated.AddDynamic(this, &USCombatComponent::OnEliminated);

		OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		if (OwnerCharacter->GetFollowCamera())
		{
			DefaultFOV = OwnerCharacter->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}

		bIsLocallyControlled |= OwnerCharacter->IsLocallyControlled();
		bHasAuthority |= OwnerCharacter->HasAuthority();

		
		OnCharacterSet.Broadcast();
		OnCharacterSet.Clear();

		if (OwnerCharacter && PlayerController && IsLocallyControlled()) SetComponentTickEnabled(true);


	}
}

void USCombatComponent::SetPlayerController(AController* NewController)
{
	if (NewController && NewController != PlayerController)
	{
		if (ASPlayerController* NewPlayerController = Cast<ASPlayerController>(NewController))
		{
			PlayerController = NewPlayerController;
			HUD = PlayerController->GetHUD<ASHUD>();
			
			bIsLocallyControlled |= PlayerController->IsLocalController();
			bHasAuthority |= PlayerController->HasAuthority();

			OnPlayerControllerSet.Broadcast();
			OnPlayerControllerSet.Clear();
			
			if (OwnerCharacter && PlayerController && IsLocallyControlled()) SetComponentTickEnabled(true);
		}
	}
}

// only ticks on locally controlled character after Character are PlayerController are both set
void USCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	FHitResult HitResult;
	TraceUnderCrosshairs(HitResult);
	HitTarget = HitResult.ImpactPoint;
	SetHUDCrosshairs(DeltaTime);
	InterpFOV(DeltaTime);
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
		if (OwnerCharacter)
		{
			const float DistanceToCharacter = (TraceStart - OwnerCharacter->GetActorLocation()).Size();
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
		if (EquippedWeapon)
		{
			HUDPackage.SetCrosshairs(EquippedWeapon->GetHUDPackage());
			
			FVector Velocity = OwnerCharacter->GetVelocity();
			Velocity.Z = 0;
			
			const FVector2D WalkSpeedRange(0.f, OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed);
			const FVector2D VelocityMultiplierRange(0.f, 1.f);

			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			if (OwnerCharacter->GetCharacterMovement()->IsFalling())
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
		else
		{
			HUDPackage.SetCrosshairs(FHUDPackage());
		}
		
		HUD->SetHUDPackage(HUDPackage);
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
	
	if (OwnerCharacter && OwnerCharacter->GetFollowCamera())
	{
		OwnerCharacter->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

bool USCombatComponent::CanFire() const
{
	return CombatState == ESCombatState::ECS_Unoccupied && !bLocallyReloading;
}

void USCombatComponent::FireButtonPressed(bool bPressed)
{
	LOCALLY_CONTROLLED_ONLY(OwnerCharacter);
	
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed)
	{
		FireWeapon();
	}
}

void USCombatComponent::FireWeapon()
{
	LOCALLY_CONTROLLED_ONLY(OwnerCharacter);
	
	if (EquippedWeapon)
	{
		EquippedWeapon->Fire(HitTarget);
		CrosshairShootingFactor = 1.f;
	}
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
	if (OwnerCharacter && WeaponToEquip)
	{
		ASWeapon* OldEquippedWeapon = EquippedWeapon;
		
		EquippedWeapon = WeaponToEquip;
		OnRep_EquippedWeapon(OldEquippedWeapon);
	}
}

void USCombatComponent::OnRep_EquippedWeapon(ASWeapon* OldEquippedWeapon)
{
	UpdateCarriedAmmo();
	
	if (EquippedWeapon && OwnerCharacter)
	{
		EquippedWeapon->Equip(OwnerCharacter);

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

bool USCombatComponent::CanReload() const
{
	return CarriedAmmo > 0 && CombatState == ESCombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsFull() && !bLocallyReloading;
}

void USCombatComponent::ReloadWeapon()
{
	if (CanReload())
	{
		bLocallyReloading = true;
		SetCombatState(ESCombatState::ECS_Reloading);
		ServerReloadWeapon();
	}
}

void USCombatComponent::ServerReloadWeapon_Implementation()
{
	if (CanReload() && !IsLocallyControlled())
	{
		SetCombatState(ESCombatState::ECS_Reloading);
	}
}

void USCombatComponent::FinishReloading()
{
	bLocallyReloading = false;
	
	if (!EquippedWeapon) return;

	const int32 Amount = ReloadAmount();
	if (HasAuthority())
	{
		SetCombatState(ESCombatState::ECS_Unoccupied);
		if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
		{
			CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= Amount;
			UpdateCarriedAmmo();
		}
		EquippedWeapon->AddAmmo(Amount);
	}
	else if (IsLocallyControlled())
	{
		EquippedWeapon->AddAmmo(Amount);
		
		CarriedAmmo -= Amount;
		OnRep_CarriedAmmo();
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
		const int32 AmountCarried = HasAuthority() ? CarriedAmmoMap[EquippedWeapon->GetWeaponType()] : CarriedAmmo;
		return FMath::Clamp(RoomInMag, 0, AmountCarried);
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
	if (OwnerCharacter && EquippedWeapon && bAiming != bIsAiming)
	{
		if (IsLocallyControlled()) bAimButtonPressed = bIsAiming;
		
		bAiming = bIsAiming;

		EquippedWeapon->SetAiming(bAiming);
		
		if (HasAuthority())
		{
			OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
		}
		else
		{
			ServerSetAiming(bAiming);
		}
	}
}

void USCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	SetAiming(bIsAiming);
}

void USCombatComponent::OnRep_Aiming()
{
	if (IsLocallyControlled()) bAiming = bAimButtonPressed;
}

void USCombatComponent::ThrowGrenade()
{
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
	if (OwnerCharacter && OwnerCharacter->GetAttachedGrenade())
	{
		OwnerCharacter->GetAttachedGrenade()->SetVisibility(bShowGrenade);
	}
}

void USCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);

	if (IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTarget);
	}
}

void USCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (GrenadeClass && OwnerCharacter && OwnerCharacter->GetAttachedGrenade())
	{
		const FVector StartingLocation = OwnerCharacter->GetAttachedGrenade()->GetComponentLocation();
		const FVector ToTarget = Target - StartingLocation;
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwnerCharacter;
		SpawnParams.Instigator = OwnerCharacter;
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
	if (HasAuthority())
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
	if (PlayerController)
	{
		if (EquippedWeapon && CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
		{
			CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
			if (PlayerController->HasLocalAuthority()) OnRep_CarriedAmmo();
		}
	}
	else
	{
		OnPlayerControllerSet.AddUniqueDynamic(this, &USCombatComponent::UpdateCarriedAmmo);
	}
}

void USCombatComponent::OnRep_CarriedAmmo() const
{
	if (IsLocallyControlled())
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
	if (IsLocallyControlled()) OnRep_CarriedGrenades();
}

void USCombatComponent::OnRep_CarriedGrenades() const
{
	if (IsLocallyControlled())
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
	if (CombatState == NewCombatState) return;

	const ESCombatState OldCombatState = CombatState;
	CombatState = NewCombatState;
	OnRep_CombatState(OldCombatState);
}

void USCombatComponent::OnRep_CombatState(ESCombatState OldCombatState)
{
	if (CombatState == OldCombatState) return;
	
	switch (CombatState)
	{
	case ESCombatState::ECS_Unoccupied:
		{
			if (bFireButtonPressed) FireWeapon();
			break;
		}
	case ESCombatState::ECS_Reloading:
		{
			PlayReloadMontage();
			break;
		}
	case ESCombatState::ECS_ThrowingGrenade:
		{
			if (OwnerCharacter)
			{
				OwnerCharacter->PlayThrowGrenadeMontage();
				AttachActorToLeftHand(EquippedWeapon);
				ShowAttachedGrenade(true);
			}
			break;
		}
	default:;
	}
}

void USCombatComponent::PlayFireMontage() const
{
	if (EquippedWeapon)
	{
		UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
		if (AnimInstance && FireWeaponMontage)
		{
			AnimInstance->Montage_Play(FireWeaponMontage);
			AnimInstance->Montage_JumpToSection(bAiming ? "RifleAim" : "RifleHip");
		}
	}
}

void USCombatComponent::PlayReloadMontage() const
{
	if (EquippedWeapon)
	{
		UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
		if (AnimInstance && ReloadMontage)
		{
			AnimInstance->Montage_Play(ReloadMontage);
			FName SectionName;
			switch(EquippedWeapon->GetWeaponType())
			{
			case EWeaponType::EWT_AssaultRifle:
				{
					SectionName = "Rifle";
					break;
				}
			case EWeaponType::EWT_RocketLauncher:
				{
					SectionName = "Rifle";
					break;
				}
			case EWeaponType::EWT_Pistol:
				{
					SectionName = "Pistol";
					break;
				}
			case EWeaponType::EWT_SubmachineGun:
				{
					SectionName = "Rifle";
					break;
				}
			case EWeaponType::EWT_Shotgun:
				{
					SectionName = "Rifle";
					break;
				}
			case EWeaponType::EWT_SniperRifle:
				{
					SectionName = "Rifle";
					break;
				}
			case EWeaponType::EWT_GrenadeLauncher:
				{
					SectionName = "Rifle";
					break;
				}
			default:;
			}
			
			AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

void USCombatComponent::AttachActorToRightHand(AActor* ActorToAttach) const
{
	if (OwnerCharacter && ActorToAttach)
	{
		ActorToAttach->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "RightHandSocket");
	}
}

void USCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach) const
{
	if (OwnerCharacter && ActorToAttach)
	{
		ActorToAttach->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "LeftHandSocket");
	}
}

void USCombatComponent::AttachActorToBackpack(AActor* ActorToAttach) const
{
	if (OwnerCharacter && ActorToAttach)
	{
		ActorToAttach->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "BackpackSocket");
	}
}

void USCombatComponent::OnEliminated()
{
	if (EquippedWeapon)
	{
		if (EquippedWeapon->bDestroyWeaponOnKilled)
		{
			EquippedWeapon->Destroy();
		}
		else
		{
			EquippedWeapon->Drop();
		}
	}
	
	if (SecondaryWeapon)
	{
		if (SecondaryWeapon->bDestroyWeaponOnKilled)
		{
			SecondaryWeapon->Destroy();
		}
		else
		{
			SecondaryWeapon->Drop();
		}
	}
}
