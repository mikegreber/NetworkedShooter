// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/HUDPackage.h"
#include "Types/SCombatState.h"
#include "Weapon/SWeaponTypes.h"
#include "Components/ActorComponent.h"
#include "SCombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponChanged, class ASWeapon*, NewWeapon, class ASWeapon*, OldWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMulticastNotifyDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInt32UpdatedDelegate, int32, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOn2Int32UpdatedDelegate, int32, A, int32, B);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class NETWORKEDSHOOTER_API USCombatComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class ASCharacter;
	
	UPROPERTY(EditAnywhere, Category = "Combat | Ammo")
	int32 MaxCarriedAmmoAmount = 500;
	
	UPROPERTY(EditAnywhere, Category = "Combat | Ammo", meta = (DisplayName = "Starting Ammo"))
	TMap<EWeaponType, int32> CarriedAmmoMap;
	
	UPROPERTY(EditAnywhere, Category = "Combat | Grenades")
	TSubclassOf<class ASProjectile> GrenadeClass;
	
	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_CarriedGrenades, Category = "Combat | Grenades")
	int32 Grenades = 4;

	UPROPERTY(EditAnywhere, Category = "Combat | Grenades")
	int32 MaxGrenades = 4;

	UPROPERTY(EditAnywhere, Category = "Combat | Movement")
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere, Category = "Combat | Movement")
	float AimWalkSpeed;

	UPROPERTY(EditAnywhere, Category = "Combat | Zoom")
	float ZoomInterpSpeed = 20.f;

public:
	
	FMulticastNotifyDelegate OnControllerSet;
	
	FOnInt32UpdatedDelegate OnCarriedAmmoUpdated;
	
	FOnInt32UpdatedDelegate OnGrenadesUpdated;
	
	FOnWeaponChanged OnWeaponChanged;
	
private:

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	ASWeapon* EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	ASWeapon* SecondaryWeapon;
	
	UPROPERTY(ReplicatedUsing=OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UPROPERTY(ReplicatedUsing=OnRep_CombatState)
	ESCombatState CombatState = ESCombatState::ECS_Unoccupied;

	bool bFireButtonPressed;
	FVector_NetQuantize HitTarget;

	UPROPERTY() ASCharacter* Character;
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
	
	USCombatComponent();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	bool CanFire() const;

	void FireButtonPressed(bool bCond);

	void ReloadWeapon();
	
	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);

protected:
	
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);
	
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

	int32 ReloadAmount();

	void DropWeapon(ASWeapon*& WeaponToDrop);

	void SetAiming(bool bIsAiming);
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

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
	
	void UpdateCarriedAmmo();
	UFUNCTION()
	void OnRep_CarriedAmmo() const;

	void SetGrenades(int32 NewGrenades);
	UFUNCTION()
	void OnRep_CarriedGrenades() const;
	
	void SetCombatState(ESCombatState NewCombatState);
	UFUNCTION()
	void OnRep_CombatState();
	
	void AttachActorToRightHand(AActor* ActorToAttach) const;
	
	void AttachActorToLeftHand(AActor* ActorToAttach) const;
	
	void AttachActorToBackpack(AActor* ActorToAttach) const;

public:
	
	FORCEINLINE int32 GetCarriedAmmo() const { return CarriedAmmo; };
	FORCEINLINE int32 GetGrenades() const { return Grenades; }
	FORCEINLINE bool GetFireButtonPressed() const { return bFireButtonPressed; }
	FORCEINLINE ESCombatState GetCombatState() const { return CombatState; }

};

