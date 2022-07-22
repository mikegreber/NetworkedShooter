// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameplayTagStack.h"
#include "Character/SAnimInstance.h"
#include "Types/HUDPackage.h"
#include "Types/SCombatState.h"
#include "Weapon/SWeaponTypes.h"
#include "Components/ActorComponent.h"
#include "SCombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponChanged, class ASWeapon*, NewWeapon, class ASWeapon*, OldWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMulticastNotifyDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInt32UpdatedDelegate, int32, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOn2Int32UpdatedDelegate, int32, A, int32, B);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoContainerChanged, const FGameplayTagStackContainer&, AmmoContainer, FGameplayTag, EquippedAmmoType);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class NETWORKEDSHOOTER_API USCombatComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class ASCharacter;

	UPROPERTY(EditAnywhere, Category = "Combat | Defaults")
	TSubclassOf<ASWeapon> DefaultWeaponClass;

	UPROPERTY(EditAnywhere, Category = "Combat | Defaults")
	TSubclassOf<class ASProjectile> GrenadeClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat | Defaults")
	FGameplayTagStackContainer StartingAmmo;
	
	UPROPERTY(ReplicatedUsing=OnRep_AmmoContainer, VisibleInstanceOnly, Category = "Combat | Ammo")
	FGameplayTagStackContainer AmmoContainer;

	UPROPERTY(EditAnywhere, Category = "Combat | Movement")
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere, Category = "Combat | Movement")
	float AimWalkSpeed;

	UPROPERTY(EditAnywhere, Category = "Combat | Zoom")
	float ZoomInterpSpeed = 20.f;

	UPROPERTY(EditAnywhere, Category = "Combat | Animation")
	UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = "Combat | Animation")
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = "Combat | Animation")
	UAnimMontage* ThrowGrenadeMontage;
	
	// read with HasAuthority()
	bool bHasAuthority;

	// read with IsLocallyControlled()
	bool bIsLocallyControlled;

	bool bLocallyReloading;
	
	bool bAimButtonPressed = false;
	
	UPROPERTY(ReplicatedUsing=OnRep_Aiming)
	bool bAiming = false;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	ASWeapon* EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	ASWeapon* SecondaryWeapon;

	UPROPERTY(ReplicatedUsing=OnRep_CombatState)
	ESCombatState CombatState = ESCombatState::ECS_Unoccupied;

	bool bFireButtonPressed;
	FVector_NetQuantize HitTarget;

	UPROPERTY() ASCharacter* Character;
	UPROPERTY() UAnimInstance* AnimInstance;
	UPROPERTY() class ASPlayerController* Controller;
	UPROPERTY() class ASHUD* HUD;

	FHUDPackage HUDPackage;
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;
	
	float DefaultFOV;
	float CurrentFOV;

public:
	
	FOnAmmoContainerChanged OnAmmoContainerChanged;
	FMulticastNotifyDelegate OnPlayerControllerSet;
	FMulticastNotifyDelegate OnCharacterSet;
	FOnWeaponChanged OnWeaponChanged;
	
public:
	
	USCombatComponent();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;
	
	void SetOwnerCharacter(ASCharacter* NewCharacter);

	void SetPlayerController(AController* NewController);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	bool CanFire() const;

	void FireButtonPressed(bool bCond);

	bool CanReload() const;
	
	void ReloadWeapon();
	
	void PlayFireMontage() const;
	
	void PlayReloadMontage() const;

	void AddTagStack(const FGameplayTagStack& Stack);
	
	void BroadcastState() const;

protected:
	
	void SpawnDefaultWeapon();
	
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	void InterpFOV(float DeltaTime);

	void FireWeapon();
	
	void EquipWeapon(ASWeapon* WeaponToEquip);
	
	void EquipPrimaryWeapon(ASWeapon* WeaponToEquip);
	
	UFUNCTION()
	void OnRep_EquippedWeapon(ASWeapon* OldEquippedWeapon);
	
	void EquipSecondaryWeapon(ASWeapon* WeaponToEquip);
	
	UFUNCTION()
	void OnRep_SecondaryWeapon(ASWeapon* OldSecondaryWeapon);
	
	void SwapWeapons();
	UFUNCTION(Server, Reliable)
	void ServerSwapWeapons();
	
	UFUNCTION(Server, Reliable)
	void ServerReloadWeapon();
	
	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	int32 ReloadAmount() const;

	void DropWeapon(ASWeapon*& WeaponToDrop);

	void SetAiming(bool bIsAiming);
	
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);
	
	UFUNCTION()
	void OnRep_Aiming();

	void PlayThrowGrenadeMontage() const;

	void ThrowGrenade();
	
	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	void ShowAttachedGrenade(bool bShowGrenade) const;
	
	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();
	
	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);
	
	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	UFUNCTION()
	void OnRep_AmmoContainer(const FGameplayTagStackContainer& OldAmmoContainer);
	
	void SetCombatState(ESCombatState NewCombatState);
	UFUNCTION()
	void OnRep_CombatState(ESCombatState OldCombatState);
	
	void AttachActorToRightHand(AActor* ActorToAttach) const;
	
	void AttachActorToLeftHand(AActor* ActorToAttach) const;
	
	void AttachActorToBackpack(AActor* ActorToAttach) const;

	UFUNCTION()
	void OnEliminated();
	
public:
	
	FORCEINLINE bool HasAuthority() const { return bHasAuthority; }
	FORCEINLINE bool IsLocallyControlled() const { return bIsLocallyControlled; }
	FORCEINLINE bool IsAiming() const { return bAiming; }
	FORCEINLINE bool GetFireButtonPressed() const { return bFireButtonPressed; }
	FORCEINLINE ESCombatState GetCombatState() const { return CombatState; }
	FORCEINLINE ASWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
	
};
