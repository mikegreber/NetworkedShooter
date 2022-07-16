// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SCharacter.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SBuffComponent.h"
#include "Components/SCombatComponent.h"
#include "Components/SLagCompensationComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameMode/SGameMode.h"
#include "GameState/SGameState.h"
#include "HUD/SOverheadWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "Particles/ParticleSystemComponent.h"
#include "PlayerController/SPlayerController.h"
#include "PlayerState/SPlayerState.h"
#include "Sound/SoundCue.h"
#include "Weapon/SWeapon.h"

#define SetupHitCapsule(map, name, mesh) \
	UCapsuleComponent* name = CreateDefaultSubobject<UCapsuleComponent>(#name); \
	name->SetupAttachment(mesh, #name); \
	name->SetCollisionEnabled(ECollisionEnabled::NoCollision); \
	name->SetCollisionProfileName("RewindCollider"); \
	map.Emplace(FName(#name), name);

ASCharacter::ASCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SetupAttachment(GetMesh());
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>("OverheadWidget");
	OverheadWidget->SetupAttachment(RootComponent);

	CombatComponent = CreateDefaultSubobject<USCombatComponent>("CombatComponent");
	CombatComponent->SetIsReplicated(true);
	CombatComponent->SetComponentTickEnabled(false);
	
	BuffComponent = CreateDefaultSubobject<USBuffComponent>("BuffComponent");
	BuffComponent->SetIsReplicated(true);

	LagCompensationComponent = CreateDefaultSubobject<USLagCompensationComponent>("LagCompensationComponent");

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), "GrenadeSocket");
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachedGrenade->SetVisibility(false);
	
	// hit boxes for server side rewind
	SetupHitCapsule(RewindCapsules, head, GetMesh());
	SetupHitCapsule(RewindCapsules, pelvis, GetMesh());
	SetupHitCapsule(RewindCapsules, spine_02, GetMesh());
	SetupHitCapsule(RewindCapsules, spine_03, GetMesh());
	SetupHitCapsule(RewindCapsules, upperarm_l, GetMesh());
	SetupHitCapsule(RewindCapsules, upperarm_r, GetMesh());
	SetupHitCapsule(RewindCapsules, lowerarm_l, GetMesh());
	SetupHitCapsule(RewindCapsules, lowerarm_r, GetMesh());
	SetupHitCapsule(RewindCapsules, hand_l, GetMesh());
	SetupHitCapsule(RewindCapsules, hand_r, GetMesh());
	SetupHitCapsule(RewindCapsules, backpack, GetMesh());
	SetupHitCapsule(RewindCapsules, blanket, GetMesh()); blanket->SetupAttachment(GetMesh(), "backpack");
	SetupHitCapsule(RewindCapsules, thigh_l, GetMesh());
	SetupHitCapsule(RewindCapsules, thigh_r, GetMesh());
	SetupHitCapsule(RewindCapsules, calf_l, GetMesh());
	SetupHitCapsule(RewindCapsules, calf_r, GetMesh());
	SetupHitCapsule(RewindCapsules, foot_l, GetMesh());
	SetupHitCapsule(RewindCapsules, foot_r, GetMesh())
}	

void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ASCharacter, bDisableGameplay);
	DOREPLIFETIME(ASCharacter, Health);
	DOREPLIFETIME(ASCharacter, Shield);
	DOREPLIFETIME(ASCharacter, bIsServerControlled);
}

void ASCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if (CombatComponent) CombatComponent->SetOwnerCharacter(this);
	
	if (BuffComponent) BuffComponent->SetOwnerCharacter(this);

	if (LagCompensationComponent) LagCompensationComponent->SetOwnerCharacter(this);
	
	if (AttachedGrenade) AttachedGrenade->SetVisibility(false);
	
	if (HasAuthority()) OnTakeAnyDamage.AddDynamic(this, &ASCharacter::ReceiveDamage);
}



void ASCharacter::BeginPlay()
{
	Super::BeginPlay();

	ShooterGameMode = GetWorld()->GetAuthGameMode<ASGameMode>();

	SetShooterPlayerState(GetPlayerState());
	SetGameState(GetWorld()->GetGameState());
	
	if (HasAuthority()) SpawnDefaultWeapon();
}

void ASCharacter::SpawnDefaultWeapon() const
{
	// only call on server when using ASGameMode
	if (Cast<ASGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (DefaultWeaponClass && CombatComponent && !bEliminated)
		{
			if (UWorld* World = GetWorld())
			{
				ASWeapon* Weapon = World->SpawnActor<ASWeapon>(DefaultWeaponClass);
				Weapon->bDestroyWeaponOnKilled = true;
				CombatComponent->EquipWeapon(Weapon);
			}
		}
	}
}

void ASCharacter::MulticastGainedTheLead_Implementation()
{
	if (CrownSystem)
	{
		if (!CrownComponent)
		{
			CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
				CrownSystem,
				GetMesh(),
				FName("Crown"),
				FVector(0.f,0.f,0.f),
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				false
			);
		}

		if (CrownComponent)
		{
			CrownComponent->Activate();
		}
	}
	
}

void ASCharacter::MulticastLostTheLead_Implementation()
{
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
}

// called on server only
void ASCharacter::PossessedBy(AController* NewController)
{
	UE_LOG(LogTemp, Warning, TEXT("%s %s"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR, *GetNameSafe(this))
	ASPlayerController* OldController = ShooterPlayerController;
	Super::PossessedBy(NewController);
	SetPlayerController(NewController, OldController);
	SetShooterPlayerState(GetPlayerState());
}

// called on clients only
void ASCharacter::OnRep_Controller()
{
	UE_LOG(LogTemp, Warning, TEXT("%s %s"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR, *GetNameSafe(this))
	ASPlayerController* OldController = ShooterPlayerController;
	Super::OnRep_Controller();
	SetPlayerController(Controller, OldController);
	SetShooterPlayerState(GetPlayerState());
}

void ASCharacter::SetPlayerController(AController* NewController, AController* OldController)
{
	ShooterPlayerController = Cast<ASPlayerController>(NewController);
	if (ShooterPlayerController != OldController)
	{
		
		CombatComponent->SetPlayerController(ShooterPlayerController);
		LagCompensationComponent->SetPlayerController(ShooterPlayerController);

		bIsServerControlled = ShooterPlayerController && ShooterPlayerController->HasLocalAuthority();
		
		OnPlayerControllerSet.Broadcast(ShooterPlayerController);
		OnPlayerControllerSet.Clear();
	}
}

void ASCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	SetShooterPlayerState(GetPlayerState());
}

void ASCharacter::CheckLead()
{
	if (ShooterPlayerState && GameState)
	{
		if (GameState->TopScoringPlayers.Contains(ShooterPlayerState))
		{
			MulticastGainedTheLead();
		}
	}
}

void ASCharacter::SetTeamColor(ETeam NewTeam)
{
	if (!GetMesh()) return;
	
	switch (NewTeam)
	{
	case ETeam::ET_NoTeam:
		{
			GetMesh()->SetMaterial(0, OriginalMaterialInstance);
			DissolveMaterialInstance = BlueDissolveMaterialInstance;
			break;
		}
	case ETeam::ET_RedTeam:
		{
			GetMesh()->SetMaterial(0, RedMaterialInstance);
			DissolveMaterialInstance = RedDissolveMaterialInstance;
			break;
		}
	case ETeam::ET_BlueTeam:
		{
			GetMesh()->SetMaterial(0, BlueMaterialInstance);
			DissolveMaterialInstance = BlueDissolveMaterialInstance;
			break;
		}
	default: ;
	}
}

void ASCharacter::SetShooterPlayerState(APlayerState* NewPlayerState)
{
	const ASPlayerState* OldPlayerState = ShooterPlayerState;
	ShooterPlayerState = Cast<ASPlayerState>(NewPlayerState);
	if (ShooterPlayerState && ShooterPlayerState != OldPlayerState)
	{
		CheckLead();

		SetTeamColor(ShooterPlayerState->GetTeam());
		
		OnPlayerStateSet.Broadcast(ShooterPlayerState);
		OnPlayerStateSet.Clear();
	}
}

void ASCharacter::SetGameState(AGameStateBase* NewGameState)
{
	GameState = Cast<ASGameState>(NewGameState);
	if (GameState)
	{
		GetWorld()->GameStateSetEvent.RemoveAll(this);
		CheckLead();
	}
	else
	{
		GetWorld()->GameStateSetEvent.AddUObject(this, &ASCharacter::SetGameState);
	}
}

void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);
}

void ASCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	
	if (GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled())
	{
		CalculateAimOffset(DeltaTime);
		HideCharacterIfCameraClose();
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicateMovement();
		}
		CalculateAO_Pitch();
	}
}



void ASCharacter::OnRep_ReplicateMovement()
{
	Super::OnRep_ReplicateMovement();
	
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void ASCharacter::Eliminated(bool bPlayerLeftGame)
{
	SERVER_ONLY();

	OnEliminated.Broadcast();
	MulticastEliminated(bPlayerLeftGame);
}

void ASCharacter::MulticastEliminated_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;
	bEliminated = true;
	
	PlayEliminatedMontage();

	// start dissolve effect
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
		StartDissolve();
	}

	// disable character movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	bDisableGameplay = true;
	if (CombatComponent) CombatComponent->FireButtonPressed(false);
	
	// disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// spawn elimination bot
	if (EliminationBotEffect)
	{
		const FVector EliminationBotSpawnPoint = GetActorLocation() + FVector(0.f, 0.f, 200.f);
		EliminationBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			EliminationBotEffect,
			EliminationBotSpawnPoint,
			GetActorRotation()
		);
	}
	if (EliminationBotSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EliminationBotSound,
			GetActorLocation()
		);
	}

	if (IsLocallyControlled() &&
		CombatComponent &&
		CombatComponent->bAiming &&
		CombatComponent->EquippedWeapon &&
		CombatComponent->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		ShowSniperScopeWidget(false);
	}

	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}

	GetWorldTimerManager().SetTimer(
		EliminatedTimer, 
		this,
		&ASCharacter::EliminatedTimerFinished,
		EliminatedDelay
	);
}

void ASCharacter::EliminatedTimerFinished()
{
	if (ASGameMode* GameMode = GetWorld()->GetAuthGameMode<ASGameMode>(); GameMode && !bLeftGame)
	{
		GameMode->RequestRespawn(this, Controller);
	}
	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}

void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASCharacter::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ASCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ASCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ASCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ASCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction("ThrowGrenade", IE_Pressed, this, &ASCharacter::GrenadeButtonPressed);
	PlayerInputComponent->BindAction("SwapWeapons", IE_Pressed, this, &ASCharacter::SwapWeaponsButtonPressed);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ASCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::LookUp);
}

void ASCharacter::Destroyed()
{
	Super::Destroyed();

	if (EliminationBotComponent) EliminationBotComponent->DestroyComponent();

	const ASGameMode* GM = Cast<ASGameMode>(UGameplayStatics::GetGameMode(this));
	const bool bMatchNotInProgress = GM && GM->GetMatchState() != MatchState::InProgress;
	
	if (CombatComponent && CombatComponent->EquippedWeapon && bMatchNotInProgress)
	{
		CombatComponent->EquippedWeapon->Destroy();
	}
}

void ASCharacter::PlayFireMontage(bool bAiming)
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && FireWeaponMontage)
		{
			AnimInstance->Montage_Play(FireWeaponMontage);
			AnimInstance->Montage_JumpToSection(bAiming ? "RifleAim" : "RifleHip");
		}
	}
}

void ASCharacter::PlayReloadMontage()
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && ReloadMontage)
		{
			AnimInstance->Montage_Play(ReloadMontage);
			FName SectionName;
			switch(CombatComponent->EquippedWeapon->GetWeaponType())
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

void ASCharacter::PlayEliminatedMontage()
{
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ASCharacter::PlayThrowGrenadeMontage()
{
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ASCharacter::PlayHitReactMontage()
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && HitReactMontage)
		{
			AnimInstance->Montage_Play(HitReactMontage);
			AnimInstance->Montage_JumpToSection("FromFront");
		}
	}
}



void ASCharacter::Jump()
{
	SERVER_ONLY();

	if (bDisableGameplay) return;
	
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ASCharacter::MoveForward(float Value)
{
	if (bDisableGameplay) return;
	
	if (Controller && Value)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ASCharacter::MoveRight(float Value)
{
	if (bDisableGameplay) return;
	
	if (Controller && Value)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ASCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ASCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ASCharacter::EquipButtonPressed()
{
	if (bDisableGameplay) return;

	if (OverlappingWeapon)
	{
		ServerEquipButtonPressed();
	}
}

void ASCharacter::ServerEquipButtonPressed_Implementation()
{
	if (CombatComponent && OverlappingWeapon)
	{
		CombatComponent->EquipWeapon(OverlappingWeapon);
	}
}

void ASCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay) return;
	
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ASCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return;
	
	if (CombatComponent)
	{
		CombatComponent->ReloadWeapon();
	}
}

void ASCharacter::AimButtonPressed()
{
	if (bDisableGameplay) return;

	if (CombatComponent)
	{
		CombatComponent->SetAiming(true);
	}
}

void ASCharacter::AimButtonReleased()
{
	if (bDisableGameplay) return;
	
	if (CombatComponent)
	{
		CombatComponent->SetAiming(false);
	}
}

void ASCharacter::FireButtonPressed()
{
	if (bDisableGameplay) return;

	if (CombatComponent)
	{
		CombatComponent->FireButtonPressed(true);
	}
}

void ASCharacter::FireButtonReleased()
{
	if (bDisableGameplay) return;
	
	if (CombatComponent)
	{
		CombatComponent->FireButtonPressed(false);
	}
}

void ASCharacter::GrenadeButtonPressed()
{
	if (bDisableGameplay) return;
	
	if (CombatComponent)
	{
		CombatComponent->ThrowGrenade();
	}
}

void ASCharacter::SwapWeaponsButtonPressed()
{
	if (bDisableGameplay) return;
	
	if (CombatComponent)
	{
		CombatComponent->SwapWeapons();
	}
}

float ASCharacter::CalculateSpeed() const
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0;
	return Velocity.Size();
}

void ASCharacter::CalculateAimOffset(float DeltaTime)
{
	if (CombatComponent && CombatComponent->EquippedWeapon == nullptr) return;

	// static FRotator StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
	
	const float Speed = CalculateSpeed();
	const bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) // standing still on ground
	{
		bRotateRootBone = true;
		const FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		const FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning) InterpAO_Yaw = AO_Yaw;
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	
	if (Speed > 0.f || bIsInAir) // running or jumping
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void ASCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;

	// correct for rotation replication compression
	if (AO_Pitch > 90.f) AO_Pitch -= 360.f;

	// accomplishes the same thing
	// FVector2d InRange(270.f, 360.f);
	// FVector2d OutRange(-90.f, 90.f);
	// AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
}

void ASCharacter::SimProxiesTurn()
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		bRotateRootBone = false;
		if (CalculateSpeed() == 0.f)
		{
			ProxyRotationLastFrame = ProxyRotation;
			ProxyRotation = GetActorRotation();
			const float ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;
			
			if (ProxyYaw > TurnThreshold)
			{
				TurningInPlace = ETurningInPlace::ETIP_Right;
			}
			else if (ProxyYaw < -TurnThreshold)
			{
				TurningInPlace = ETurningInPlace::ETIP_Left;
			}
			else
			{
				TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			}
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
	}
}

void ASCharacter::OnKilled(AController* InstigatorController)
{
	if (ASGameMode* GameMode = GetWorld()->GetAuthGameMode<ASGameMode>())
	{
		GameMode->PlayerEliminated(
			this,
			ShooterPlayerController,
			Cast<ASPlayerController>(InstigatorController)
		);
	}
}

void ASCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	if (!ShooterGameMode || bEliminated) return;

	Damage = ShooterGameMode->CalculateDamage(InstigatorController, Controller, Damage);
	
	const float LastShield = Shield;
	Shield = ClampWithOverflow(Shield - Damage, 0.f, MaxShield, Damage);
	OnRep_Shield(LastShield);
	
	const float LastHealth = Health;
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	OnRep_Health(LastHealth);

	if (Health == 0.f)
	{
		OnKilled(InstigatorController);
	}
}

void ASCharacter::TurnInPlace(float DeltaSeconds)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaSeconds, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ASCharacter::HideCharacterIfCameraClose() const
{
	if (IsLocallyControlled())
	{
		const bool bHideCharacter = (FollowCamera->GetComponentLocation() - GetActorLocation()).SizeSquared() < CameraThreshold * CameraThreshold;
		
		GetMesh()->SetVisibility(!bHideCharacter);
		if (CombatComponent)
		{
			if (CombatComponent->EquippedWeapon && CombatComponent->EquippedWeapon->GetWeaponMesh())
			{
				CombatComponent->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = bHideCharacter;
			}
				
			if (CombatComponent->SecondaryWeapon && CombatComponent->SecondaryWeapon->GetWeaponMesh())
			{
				CombatComponent->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = bHideCharacter;
			}
		}
	}
}

void ASCharacter::OnRep_Health(float LastHealth)
{
	if (IsLocallyControlled())
	{
		OnHealthChanged.Broadcast(Health, MaxHealth);
	}

	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

void ASCharacter::OnRep_Shield(float LastShield)
{
	if (IsLocallyControlled())
	{
		OnShieldChanged.Broadcast(Shield, MaxShield);
	}

	if (Shield < LastShield)
	{
		PlayHitReactMontage();
	}
}

void ASCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ASCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ASCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}



void ASCharacter::ServerLeaveGame_Implementation()
{
	ASGameMode* GameMode = GetWorld()->GetAuthGameMode<ASGameMode>();
	
	if (GameMode && ShooterPlayerState)
	{
		GameMode->PlayerLeftGame(ShooterPlayerState);
	}
}

void ASCharacter::SetOverlappingWeapon(ASWeapon* Weapon)
{
	ASWeapon* LastWeapon = OverlappingWeapon;
	OverlappingWeapon = Weapon;

	// call on server player characters only
	if (IsLocallyControlled())
	{
		OnRep_OverlappingWeapon(LastWeapon);
	}
}

void ASCharacter::OnRep_OverlappingWeapon(ASWeapon* LastWeapon)
{
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
	
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
}

bool ASCharacter::IsWeaponEquipped() const
{
	return CombatComponent && CombatComponent->EquippedWeapon;
}

bool ASCharacter::IsAiming() const
{
	return CombatComponent && CombatComponent->bAiming;
}

ASWeapon* ASCharacter::GetEquippedWeapon() const
{
	if (CombatComponent)
	{
		return CombatComponent->EquippedWeapon;
	}

	return nullptr;
}

FVector ASCharacter::GetHitTarget() const
{
	if (CombatComponent)
	{
		return CombatComponent->HitTarget;
	}
	return FVector();
}

void ASCharacter::SetHealth(float Amount)
{
	const float LastHealth = Health;
	Health = Amount;
	OnRep_Health(LastHealth);
}

void ASCharacter::SetShield(float Amount)
{
	const float LastShield = Shield;
	Shield = Amount;
	OnRep_Shield(LastShield);
}

ESCombatState ASCharacter::GetCombatState() const
{
	if (CombatComponent)
	{
		return CombatComponent->CombatState;
	}
	return ESCombatState::ECS_MAX;
}


