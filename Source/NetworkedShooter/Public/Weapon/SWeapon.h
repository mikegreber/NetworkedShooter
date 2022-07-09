// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SWeaponTypes.h"
#include "Types/CustomDepth.h"
#include "Types/HUDPackage.h"
#include "GameFramework/Actor.h"
#include "SWeapon.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponAmmoChanged, int32, NewValue);

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	EWS_Holstered UMETA(DisplayName = "Holstered"),
	
	EWS_MAX UMETA(DisplayName = "DefaultMAX"),
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan UMETA(DisplayName = "Hit Scan Weapon"),
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun Weapon"),
	
	EWS_MAX UMETA(DisplayName = "DefaultMAX"),
};

UCLASS(Abstract)
class NETWORKEDSHOOTER_API ASWeapon : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties | Components")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties | Components")
	class USphereComponent* SphereComponent;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties | Components")
	class UWidgetComponent* PickupWidget;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Properties")
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Properties")
	EFireType FireType;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties | Properties")
	EWeaponState WeaponState;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | UI", meta = (DisplayName = "Crosshairs", ShowCategories = "true"))
	FHUDPackage HUDPackage;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Zoom")
	float ZoomFOV = 30.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Zoom")
	float ZoomInterpSpeed;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Firing")
	float FireDelay = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Firing")
	bool bAutomatic = true;
	
	// UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Ammo, Category = "Weapon Properties | Firing")
	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Firing")
	int32 Ammo;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Firing")
	int32 MagCapacity;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Effects | Firing")
	UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Effects | Firing")
	TSubclassOf<class ASBulletCasing> CasingClass;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Effects | Firing")
	class USoundCue* EquipSound;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Effects | Physics")
	bool bUseWeaponPhysics;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Effects | Custom Depth")
	ECustomDepthColor DroppedOutlineColor = ECustomDepthColor::CDC_Blue;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Effects | Custom Depth")
	ECustomDepthColor SecondaryOutlineColor = ECustomDepthColor::CDC_Tan;

protected:
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Weapon Scatter")
	bool bUseScatter = false;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Weapon Scatter", meta = (EditCondition = "bUseScatter"))
	float DistanceToSphere = 800.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Weapon Scatter", meta = (EditCondition = "bUseScatter"))
	float SphereRadius = 75.f;
	
public:
	
	ASWeapon();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void SetOwner(AActor* NewOwner) override;
	
	virtual void Fire(FVector_NetQuantize HitTarget);
	
	void Equip(ACharacter* Character);
	
	void Drop();

	void AddAmmo(int32 AmmoToAdd);
	
	void Holster();

	bool IsLocallyControlled() const;
	
	void ShowPickupWidget(bool bShowWidget) const;

	FOnWeaponAmmoChanged OnWeaponAmmoChanged;

	bool bDestroyWeaponOnKilled = false;

protected:
	
	virtual void BeginPlay() override;

	bool CanFire() const;
	
	virtual void LocalFire(const FVector_NetQuantize& HitTarget);
	
	UFUNCTION(Server, Reliable)
	virtual void ServerFire(const FVector_NetQuantize& HitTarget);

	UFUNCTION(NetMulticast, Reliable)
	virtual void MulticastFire(const FVector_NetQuantize& HitTarget);

	void StartFireTimer();
	
	void FireTimerFinished();
	
	void SetWeaponState(EWeaponState NewState);
	UFUNCTION()
	void OnRep_WeaponState();
	void OnEquipped() const;
	void OnDropped();
	void OnHolstered() const;

	void SetAmmo(int32 NewAmmo);
	
	void SpendRound();
	
	UFUNCTION(Client, Reliable)
	void ClientSpendRound(int32 ServerAmmo);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);
	
	virtual void OnRep_Owner() override;
	
	FVector TraceEndWithScatter(const FVector& HitTarget) const;
	FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget) const;
	
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool bCanFire;
	FTimerHandle FireTimer;
	int32 SpendRoundSequence = 0;
	UPROPERTY() class ASCharacter* OwnerCharacter;
	UPROPERTY() class USCombatComponent* OwnerComponent;
	UPROPERTY() class ASPlayerController* OwnerController;

public:
	
	FORCEINLINE USphereComponent* GetSphereComponent() const { return SphereComponent; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE EFireType GetFireType() const { return FireType; }
	FORCEINLINE const FHUDPackage& GetHUDPackage() const { return HUDPackage; }
	FORCEINLINE float GetZoomFOV() const { return ZoomFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE float GetFireDelay() const { return FireDelay; }
	FORCEINLINE bool IsAutomatic() const { return bAutomatic; }
	FORCEINLINE bool IsEmpty() const { return Ammo <= 0; }
	FORCEINLINE bool IsFull() const { return Ammo == MagCapacity; };
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }

};
