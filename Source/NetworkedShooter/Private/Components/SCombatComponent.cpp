// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SCombatComponent.h"
#include "Camera/CameraComponent.h"
#include "Character/SCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameMode/SGameMode.h"
#include "HUD/SHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Library/ShooterGameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "PlayerController/SPlayerController.h"
#include "Weapon/SProjectile.h"
#include "Weapon/SWeapon.h"

static FGameplayTag GrenadeTag = FGameplayTag::RequestGameplayTag(FName("Ammo.Grenade"));

void USCombatComponent::OnRep_AmmoContainer(const FGameplayTagStackContainer& OldAmmoContainer)
{
	OnAmmoContainerChanged.Broadcast(AmmoContainer, EquippedWeapon ? EquippedWeapon->GetAmmoType() : FGameplayTag());

	if (EquippedWeapon && EquippedWeapon->IsEmpty() && AmmoContainer.ContainsTag(EquippedWeapon->GetAmmoType()))
	{
		ReloadWeapon();
	}
}

USCombatComponent::USCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.SetTickFunctionEnable(false);
	
	SetIsReplicatedByDefault(true);
	
	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
}

void USCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USCombatComponent, EquippedWeapon);
	DOREPLIFETIME(USCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(USCombatComponent, bAiming);
	DOREPLIFETIME(USCombatComponent, CombatState);
	DOREPLIFETIME_CONDITION(USCombatComponent, AmmoContainer, COND_OwnerOnly);
}

void USCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		SpawnDefaultWeapon();
		AmmoContainer.Initialize(StartingAmmo);
	}
}

void USCombatComponent::SetOwnerCharacter(ASCharacter* NewCharacter)
{
	if (NewCharacter && NewCharacter != Character)
	{
		Character = NewCharacter;
		Character->OnEliminated.AddDynamic(this, &USCombatComponent::OnEliminated);

		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}

		AnimInstance = Character->GetMesh()->GetAnimInstance();

		bIsLocallyControlled |= Character->IsLocallyControlled();
		bHasAuthority |= Character->HasAuthority();
		
		OnCharacterSet.Broadcast();
		OnCharacterSet.Clear();

		if (Character && Controller && IsLocallyControlled()) SetComponentTickEnabled(true);
	}
}

void USCombatComponent::SetPlayerController(AController* NewController)
{
	if (NewController && NewController != Controller)
	{
		if (ASPlayerController* NewPlayerController = Cast<ASPlayerController>(NewController))
		{
			Controller = NewPlayerController;
			HUD = Controller->GetHUD<ASHUD>();
			
			bIsLocallyControlled |= Controller->IsLocalController();
			bHasAuthority |= Controller->HasAuthority();

			OnPlayerControllerSet.Broadcast();
			OnPlayerControllerSet.Clear();
			
			if (Character && Controller && IsLocallyControlled()) SetComponentTickEnabled(true);
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
	
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

bool USCombatComponent::CanFire() const
{
	return CombatState == ESCombatState::ECS_Unoccupied && !bLocallyReloading;
}

void USCombatComponent::FireButtonPressed(bool bPressed)
{
	LOCALLY_CONTROLLED_ONLY(Character);
	
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed)
	{
		FireWeapon();
	}
}

void USCombatComponent::FireWeapon()
{
	LOCALLY_CONTROLLED_ONLY(Character);
	
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
	if (Character && WeaponToEquip)
	{
		ASWeapon* OldEquippedWeapon = EquippedWeapon;
		
		EquippedWeapon = WeaponToEquip;
		OnRep_EquippedWeapon(OldEquippedWeapon);
	}
}

void USCombatComponent::OnRep_EquippedWeapon(ASWeapon* OldEquippedWeapon)
{
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->Equip(Character);

		OnAmmoContainerChanged.Broadcast(AmmoContainer, EquippedWeapon->GetAmmoType());
		
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
	return CombatState == ESCombatState::ECS_Unoccupied && EquippedWeapon && AmmoContainer.ContainsTag(EquippedWeapon->GetAmmoType()) && !EquippedWeapon->IsFull() && !bLocallyReloading;
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

	SetCombatState(ESCombatState::ECS_Unoccupied);
	if (AmmoContainer.ContainsTag(EquippedWeapon->GetAmmoType()))
	{
		AmmoContainer.RemoveStack(EquippedWeapon->GetAmmoType(), Amount);
		OnRep_AmmoContainer(AmmoContainer);

		EquippedWeapon->AddAmmo(Amount);
	}
	
	if (bFireButtonPressed)
	{
		FireWeapon();
	}
}

int32 USCombatComponent::ReloadAmount() const
{
	if (EquippedWeapon)
	{
		const int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
		const int32 AmountCarried = AmmoContainer.GetStackCount(EquippedWeapon->GetAmmoType());
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
	if (Character && EquippedWeapon && bAiming != bIsAiming)
	{
		if (IsLocallyControlled()) bAimButtonPressed = bIsAiming;
		
		bAiming = bIsAiming;

		EquippedWeapon->SetAiming(bAiming);
		
		if (HasAuthority())
		{
			Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
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

void USCombatComponent::PlayThrowGrenadeMontage() const
{
	UShooterGameplayStatics::PlayMontage(AnimInstance, ThrowGrenadeMontage);
}

void USCombatComponent::ThrowGrenade()
{
	if (AmmoContainer.ContainsTag(GrenadeTag) && CombatState == ESCombatState::ECS_Unoccupied && EquippedWeapon)
	{
		ServerThrowGrenade();
		SetCombatState(ESCombatState::ECS_ThrowingGrenade);
	}
}

void USCombatComponent::ServerThrowGrenade_Implementation()
{
	if (AmmoContainer.ContainsTag(GrenadeTag) && CombatState == ESCombatState::ECS_Unoccupied && EquippedWeapon)
	{
		AmmoContainer.RemoveStack(GrenadeTag, 1);
		OnRep_AmmoContainer(AmmoContainer);
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
	ShowAttachedGrenade(false);

	if (IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTarget);
	}
}

void USCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
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
	if (HasAuthority())
	{
		SetCombatState(ESCombatState::ECS_Unoccupied);
	}
	
	AttachActorToRightHand(EquippedWeapon);
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
			if (Character)
			{
				PlayThrowGrenadeMontage();
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
	UShooterGameplayStatics::PlayMontage(AnimInstance, FireWeaponMontage, bAiming ? "RifleAim" : "RifleHip");
}

void USCombatComponent::PlayReloadMontage() const
{
	if (EquippedWeapon)
	{
		UShooterGameplayStatics::PlayMontage(AnimInstance, ReloadMontage, EquippedWeapon->GetReloadMontageSection());
	}
}

void USCombatComponent::AddTagStack(const FGameplayTagStack& Stack)
{
	AmmoContainer.AddStack(Stack);
	OnRep_AmmoContainer(AmmoContainer);
}

void USCombatComponent::BroadcastState() const
{
	OnAmmoContainerChanged.Broadcast(AmmoContainer, EquippedWeapon ? EquippedWeapon->GetAmmoType() : FGameplayTag());
}

void USCombatComponent::SpawnDefaultWeapon()
{
	// only call on server when using ASGameMode
	if (Cast<ASGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (DefaultWeaponClass && !Character->IsEliminated())
		{
			if (UWorld* World = GetWorld())
			{
				ASWeapon* Weapon = World->SpawnActor<ASWeapon>(DefaultWeaponClass);
				Weapon->bDestroyWeaponOnKilled = true;
				EquipWeapon(Weapon);
			}
		}
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
