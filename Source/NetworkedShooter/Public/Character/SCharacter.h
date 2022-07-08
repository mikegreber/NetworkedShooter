// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "Interfaces/SCrosshairsInteractionInterface.h"
#include "Types/SCombatState.h"
#include "Types/TurningInPlace.h"
#include "SCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChanged, float, Value, float, MaxValue);

UCLASS()
class NETWORKEDSHOOTER_API ASCharacter : public ACharacter, public ISCrosshairsInteractionInterface
{
	GENERATED_BODY()

public:
	
	ASCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Destroyed() override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_ReplicateMovement() override;
	
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayEliminatedMontage();
	void PlayThrowGrenadeMontage();
	
	void Eliminated();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastEliminated();

	FOnAttributeChanged OnHealthChanged;
	FOnAttributeChanged OnShieldChanged;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	void SpawnDefaultWeapon() const;

	DECLARE_EVENT(ASCharacter, FOnControllerSet);
	FOnControllerSet OnPlayerControllerSet;
protected:
	
	virtual void BeginPlay() override;
	virtual void Jump() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void ReloadButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void FireButtonPressed();
	void FireButtonReleased();
	void GrenadeButtonPressed();
	void SwapWeaponsButtonPressed();
	void CalculateAO_Pitch();
	void PlayHitReactMontage();
	float CalculateSpeed() const;
	void CalculateAimOffset(float DeltaTime);
	void SimProxiesTurn();
	void OnKilled(AController* InstigatorController);

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	void RotateInPlace(float DeltaTime);

	UPROPERTY()
	class ASPlayerController* PlayerController;
	void SetPlayerController(AController* NewController, const AController* OldController = nullptr);
	
private:
	
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	class USCombatComponent* CombatComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
    class USBuffComponent* BuffComponent;
	
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class ASWeapon* OverlappingWeapon;

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	UFUNCTION()
	void OnRep_OverlappingWeapon(ASWeapon* LastWeapon);

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaSeconds);

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;
	
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ThrowGrenadeMontage;
	
	void HideCharacterIfCameraClose() const;

	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float TimeSinceLastMovementReplication;


	/**
	* Player Health
	*/

	UPROPERTY(ReplicatedUsing=OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;
	
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);
	
	UPROPERTY(ReplicatedUsing=OnRep_Shield, EditAnywhere, Category = "Player Stats")
	float Shield = 0.f;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);
	
	bool bEliminated;

	FTimerHandle EliminatedTimer;

	UPROPERTY(EditDefaultsOnly)
	float EliminatedDelay = 3.f;

	void EliminatedTimerFinished();

	/**
	 * Dissolve Effect
	 */

	FOnTimelineFloat DissolveTrack;

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;

	UPROPERTY(EditAnywhere, Category = "Elimination")
	UCurveFloat* DissolveCurve;
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	// dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = "Elimination")
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = "Elimination")
	UMaterialInstance* DissolveMaterialInstance;

	/**
	 * Elim bot
	 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UParticleSystemComponent* EliminationBotComponent;
	
	UPROPERTY(EditAnywhere, Category = "Elimination")
	UParticleSystem* EliminationBotEffect;

	UPROPERTY(EditAnywhere, Category = "Elimination")
	class USoundCue* EliminationBotSound;

	/*
	 * Grenade
	 */

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

	/*
	 * Default Weapon
	 */

	UPROPERTY(EditAnywhere)
	TSubclassOf<ASWeapon> DefaultWeaponClass;
	
public:
	
	void SetOverlappingWeapon(ASWeapon* Weapon);
	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	void SetHealth(float Amount);
	void SetShield(float Amount);

	FVector GetHitTarget() const;
	ASWeapon* GetEquippedWeapon() const;
	ESCombatState GetCombatState() const;

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsEliminated() const { return bEliminated; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	FORCEINLINE ASPlayerController* GetPlayerController() const { return PlayerController; }
	FORCEINLINE USCombatComponent* GetCombatComponent() const { return CombatComponent; }
	FORCEINLINE USBuffComponent* GetBuffComponent() const { return BuffComponent; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
};

