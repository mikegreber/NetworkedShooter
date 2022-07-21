// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "Interfaces/SCrosshairsInteractionInterface.h"
#include "Types/SCombatState.h"
#include "Types/Team.h"
#include "Types/TurningInPlace.h"
#include "GameplayAbilities/Public/AbilitySystemInterface.h"
#include "SCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChangedd, float, Value, float, MaxValue);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnControllerSet, class ASPlayerController* NewController);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEliminated);

USTRUCT(BlueprintType)
struct FTeamMaterial
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UMaterialInstance* MaterialInstance;
	
	UPROPERTY(EditAnywhere)
	UMaterialInstance* DissolveMaterialInstance;
};


UCLASS()
class NETWORKEDSHOOTER_API ASCharacter : public ACharacter, public ISCrosshairsInteractionInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
	UPROPERTY()
	class USAttributeSet* AttributeSet;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	class USCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	class USLagCompensationComponent* LagCompensationComponent;
	
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class ASWeapon* OverlappingWeapon;

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;
	
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category = "Initialization")
	TArray<TSubclassOf<class UGameplayEffect>> StartingEffects;
	
	UPROPERTY(EditAnywhere, Category = "Initialization")
	TArray<TSubclassOf<class UGameplayAbility>> StartingAbilities;
	
	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Materials")
	TMap<ETeam, FTeamMaterial> TeamMaterials;
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UParticleSystemComponent* EliminationBotComponent;
	
	UPROPERTY(EditAnywhere, Category = "Elimination")
	UParticleSystem* EliminationBotEffect;

	UPROPERTY(EditAnywhere, Category = "Elimination")
	class USoundCue* EliminationBotSound;

	UPROPERTY(EditAnywhere, Category = "Elimination")
	UCurveFloat* DissolveCurve;

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

	UPROPERTY(EditDefaultsOnly)
	float EliminatedDelay = 3.f;
	
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* CrownSystem;

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Abilities")
	class USAbilitySystemComponent* ASC;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStateSet, class ASPlayerState*, NewPlayerState);
	
	FOnControllerSet OnPlayerControllerSet;
	FOnPlayerStateSet OnPlayerStateSet;
	FOnLeftGame OnLeftGame;
	FOnEliminated OnEliminated;

protected:
	
	UPROPERTY() class UAnimInstance* AnimInstance;
	UPROPERTY() class ASPlayerController* ShooterPlayerController;
	UPROPERTY() class ASPlayerState* ShooterPlayerState;
	UPROPERTY() class ASGameState* GameState;
	UPROPERTY() class ASGameMode* ShooterGameMode;
	UPROPERTY() UMaterialInstance* DissolveMaterialInstance;
	UPROPERTY() UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;
	UPROPERTY() class UNiagaraComponent* CrownComponent;

	UPROPERTY(EditAnywhere)
	TMap<FName, UShapeComponent*> RewindCapsules;

private:

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;
	
	UPROPERTY(Replicated)
	bool bIsServerControlled;
	
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;
	
	ETurningInPlace TurningInPlace;
	
	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float TimeSinceLastMovementReplication;

	FTimerHandle EliminatedTimerHandle;

	FOnTimelineFloat DissolveTrack;
	
	bool bEliminated;
	
	bool bLeftGame;

public:
	
	ASCharacter(const FObjectInitializer& ObjectInitializer);

	// IAbilitySystem Interface start
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	// IAbilitySystem Interface end
	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void PostInitializeComponents() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Destroyed() override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_ReplicateMovement() override;
	virtual void OnRep_PlayerState() override;

protected:
	virtual void BeginPlay() override;
	virtual void Jump() override;
	void InitializeAbilities();

public:
	
	void CheckInLead();
	void SetTeamColor(ETeam NewTeam);
	void SetShooterPlayerState(APlayerState* NewPlayerState);
	void Eliminated(bool bPlayerLeftGame);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastEliminated(bool bPlayerLeftGame);
	UFUNCTION(Server, Reliable)
	void ServerLeaveGame();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastGainedTheLead();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastLostTheLead();
	
protected:
	
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
	
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser);

	UFUNCTION()
	void OnHealthChanged(float NewValue, float OldValue);
	
private:

	void SetPlayerController(AController* NewController, AController* OldController = nullptr);
	void SetGameState(AGameStateBase* NewGameState);
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();
	UFUNCTION()
	void OnRep_OverlappingWeapon(ASWeapon* LastWeapon);
	void EliminatedTimerFinished();
	void StartDissolve();
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	UFUNCTION()
	void OnKilled(AController* InstigatorController);
	void TurnInPlace(float DeltaSeconds);
	void HideCharacterIfCameraClose() const;
	void CalculateAO_Pitch();
	float CalculateSpeed() const;
	void CalculateAimOffset(float DeltaTime);
	void SimProxiesTurn();
	void RotateInPlace(float DeltaTime);

public:
	
	void SetOverlappingWeapon(ASWeapon* Weapon);
	bool IsWeaponEquipped() const;
	bool IsAiming() const;

	FVector GetHitTarget() const;
	ASWeapon* GetEquippedWeapon() const;
	ESCombatState GetCombatState() const;

	FORCEINLINE void SetDisableGameplay(bool bDisable) { bDisableGameplay = bDisable; }

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsEliminated() const { return bEliminated; }
	FORCEINLINE bool IsGameplayDisabled() const { return bDisableGameplay; }
	FORCEINLINE bool IsServerControlled() const { return bIsServerControlled; };
	FORCEINLINE ASPlayerController* GetPlayerController() const { return ShooterPlayerController; }
	FORCEINLINE ASPlayerState* GetShooterPlayerState() const { return ShooterPlayerState; }
	FORCEINLINE USCombatComponent* GetCombatComponent() const { return CombatComponent; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE USLagCompensationComponent* GetLagCompensationComponent() const { return LagCompensationComponent; }
	FORCEINLINE TMap<FName, UShapeComponent*>& GetRewindColliders() { return RewindCapsules; }
	FORCEINLINE USAttributeSet* GetAttributeSet() const { return AttributeSet; };

};

