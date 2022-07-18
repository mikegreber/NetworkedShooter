// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SWeapon.h"

#include "Character/SCharacter.h"
#include "Components/SCombatComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "PlayerController/SPlayerController.h"
#include "PlayerState/SPlayerState.h"
#include "Sound/SoundCue.h"
#include "Weapon/SBulletCasing.h"

#if WITH_EDITOR
TAutoConsoleVariable<int32> ASWeapon::CVarDebugWeaponTrace(TEXT("ns.weapon.tracehit"), 0, TEXT("Debug WeaponTraceHit"), ECVF_Cheat);
#endif
#if !UE_BUILD_SHIPPING
TAutoConsoleVariable CVarDebugCanFire(TEXT("ns.weapon.canfire"), false, TEXT("Debug CanFire()"), ECVF_Cheat);
#endif

ASWeapon::ASWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCustomDepthColor(WeaponMesh, DroppedOutlineColor);
	
	RootComponent = WeaponMesh;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComponent->SetupAttachment(RootComponent);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);

	bReplicates = true;
	bNetUseOwnerRelevancy = true;
	SetReplicateMovement(true);
}

void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASWeapon, WeaponState);
	DOREPLIFETIME_CONDITION(ASWeapon, bUseServerSideRewind, COND_OwnerOnly);
}

void ASWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &ASWeapon::OnSphereBeginOverlap);
	SphereComponent->OnComponentEndOverlap.AddDynamic(this, &ASWeapon::OnSphereEndOverlap);

	ShowPickupWidget(false);
}

void ASWeapon::SetOwner(AActor* NewOwner)
{
	Super::SetOwner(NewOwner);
	OnRep_Owner();
}

void ASWeapon::OnRep_Owner()
{
	if (Owner)
	{
		OwnerCharacter = Cast<ASCharacter>(Owner);
		OwnerComponent = OwnerCharacter->GetCombatComponent();
		SetPlayerState(OwnerCharacter->GetPlayerState<ASPlayerState>());
		SetPlayerController(OwnerCharacter->GetController<ASPlayerController>());
		
		// make sure all references are set before firing
		bCanFire = OwnerCharacter && OwnerComponent && OwnerController && OwnerPlayerState;
	}
	else
	{
		// clear delegates
		if (OwnerController && OwnerController->IsLocalController())
		{
			OnWeaponAmmoChanged.Broadcast(0);
			OnWeaponAmmoChanged.Clear();
		}
		
		// clear owner references
		OwnerCharacter = nullptr;
		OwnerController = nullptr;
		OwnerComponent = nullptr;
		OwnerPlayerState = nullptr;

		GetWorldTimerManager().ClearAllTimersForObject(this);
		
		bCanFire = false;
	}
}

void ASWeapon::SetPlayerState(ASPlayerState* NewPlayerState)
{
	OwnerPlayerState = NewPlayerState;
	if (OwnerPlayerState)
	{
		// make sure all references are set before firing
		bCanFire = OwnerCharacter && OwnerComponent && OwnerController && OwnerPlayerState;
	}
	else
	{
		if (OwnerCharacter) OwnerCharacter->OnPlayerStateSet.AddUniqueDynamic(this, &ASWeapon::SetPlayerState);
	}
}

void ASWeapon::SetPlayerController(ASPlayerController* NewController)
{
	OwnerController = NewController;
	if (OwnerController)
	{
		// bind UI delegates for local player
		if (IsLocallyControlled())
		{
			OnWeaponAmmoChanged.AddUniqueDynamic(OwnerController, &ASPlayerController::SetHUDWeaponAmmo);
			OnWeaponAmmoChanged.Broadcast(Ammo);
		}
		
		// make sure all references are set before firing
		bCanFire = OwnerCharacter && OwnerComponent && OwnerController && OwnerPlayerState;
	}
	else
	{
		if (OwnerCharacter) OwnerCharacter->OnPlayerControllerSet.AddUObject(this, &ASWeapon::SetPlayerController);
	}
}

bool ASWeapon::CanUseServerSideRewind() const
{
	return bUseServerSideRewind && !OwnerPlayerState->HasHighPing();
}

bool ASWeapon::CanFire() const
{
#if !UE_BUILD_SHIPPING
	if (GEngine && CVarDebugCanFire.GetValueOnGameThread())
	{
		if (!bCanFire) GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Red, FString::Printf( TEXT("%s bCanFire false"), __FUNCTIONW__));
		if (!(Ammo > 0)) GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Red, FString::Printf( TEXT("%s Ammo: %d"), __FUNCTIONW__, Ammo));
		if (!(OwnerComponent->CanFire())) GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Red, FString::Printf( TEXT("%s Owner Component->CanFire() false"), __FUNCTIONW__));
	}
#endif
	
	return bCanFire && Ammo > 0 && OwnerComponent->CanFire();
}

void ASWeapon::Fire(FVector_NetQuantize HitTarget)
{
	if (CanFire())
	{
		bCanFire = false;
	
		if (bUseScatter) HitTarget = TraceEndWithScatter(HitTarget);

		LocalFire(GetWeaponMesh()->GetSocketTransform("MuzzleFlash"), HitTarget);
		ServerFire(HitTarget);
	
		StartFireTimer();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("CanFire Failed"))
	}
}

void ASWeapon::LocalFire(const FTransform& MuzzleTransform, const FVector_NetQuantize& HitTarget, bool bIsRewindFire, int8 Seed)
{
	if (!bIsRewindFire)
	{
		if (FireAnimation)
		{
			WeaponMesh->PlayAnimation(FireAnimation, false);
		}

		if (CasingClass)
		{
			if (const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName("AmmoEject"))
			{
				const FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(GetWeaponMesh());
			
				if (UWorld* World = GetWorld())
				{
					World->SpawnActor<ASBulletCasing>(
						CasingClass,
						SocketTransform
					);
				}	
			}
		}

		AddAmmo(-1);
	}
	
	if (Seed != 0) FMath::RandInit(Seed);
}

void ASWeapon::ServerFire_Implementation(const FVector_NetQuantize& HitTarget)
{
	MulticastFire(HitTarget);
}

void ASWeapon::MulticastFire_Implementation(const FVector_NetQuantize& HitTarget)
{
	if (!IsLocallyControlled()) LocalFire(GetWeaponMesh()->GetSocketTransform("MuzzleFlash"), HitTarget);
}

void ASWeapon::ServerFireWithSeed_Implementation(const FVector_NetQuantize& HitTarget, int8 Seed)
{
	MulticastFireWithSeed(HitTarget, Seed);
}

void ASWeapon::MulticastFireWithSeed_Implementation(const FVector_NetQuantize& HitTarget, int8 Seed)
{
	if (!IsLocallyControlled()) LocalFire(GetWeaponMesh()->GetSocketTransform("MuzzleFlash"), HitTarget, false, Seed);
}

void ASWeapon::StartFireTimer()
{
	GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&ASWeapon::FireTimerFinished,
		FireDelay
	);
}

void ASWeapon::FireTimerFinished()
{
	bCanFire = true;

	if (bAutomatic && OwnerComponent->GetFireButtonPressed())
	{
		Fire(OwnerCharacter->GetHitTarget());
	}

	if (IsEmpty()) OwnerComponent->ReloadWeapon();
}

void ASWeapon::Equip(ACharacter* Character)
{
	if (Character)
	{
		SetOwner(Character);
		SetWeaponState(EWeaponState::EWS_Equipped);
		AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "RightHandSocket");
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		
		if (EquipSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				EquipSound,
				GetActorLocation()
			);
		}
	}
}

void ASWeapon::Holster()
{
	SetWeaponState(EWeaponState::EWS_Holstered);
}

void ASWeapon::Drop()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
}

void ASWeapon::SetWeaponState(EWeaponState NewState)
{
	WeaponState = NewState;
	OnRep_WeaponState();
}

void ASWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		{
			OnEquipped();
			break;
		}
	case EWeaponState::EWS_Dropped:
		{
			OnDropped();
			break;
		}
	case EWeaponState::EWS_Holstered:
		{
			OnHolstered();
			break;
		}
	default: ;
	}
}

void ASWeapon::OnEquipped() const
{
	ShowPickupWidget(false);
	
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(bUseWeaponPhysics);

	if (bUseWeaponPhysics)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
	else
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetCustomDepthColor(WeaponMesh, ECustomDepthColor::CDC_None);
}

void ASWeapon::OnDropped()
{
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->DetachFromComponent({EDetachmentRule::KeepWorld, true});
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	SetOwner(nullptr);

	SetCustomDepthColor(WeaponMesh, DroppedOutlineColor);
}

void ASWeapon::OnHolstered() const
{
	ShowPickupWidget(false);
	
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(bUseWeaponPhysics);

	if (bUseWeaponPhysics)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
	else
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetCustomDepthColor(WeaponMesh, SecondaryOutlineColor);
}

void ASWeapon::SetAmmo(int32 NewAmmo)
{
	int32 Overflow;
	Ammo = ClampWithOverflow(NewAmmo, 0, MagCapacity, Overflow);
	if (Overflow) UE_LOG(LogTemp, Error, TEXT("%s %s Overflow! This should not happen! %d %d"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR, NewAmmo, LocalAmmoDelta);
	OnWeaponAmmoChanged.Broadcast(Ammo);
}

void ASWeapon::AddAmmo(int16 AmmoToAdd)
{
	SetAmmo(Ammo + AmmoToAdd);
	if (HasAuthority()) ClientAddAmmo(Ammo, AmmoToAdd);
	else LocalAmmoDelta += AmmoToAdd;
}

void ASWeapon::ClientAddAmmo_Implementation(int16 ServerAmmo, int16 ServerChangeAmount)
{
	if (HasAuthority()) return;

	LocalAmmoDelta -= ServerChangeAmount;
	
	if (Ammo != (ServerAmmo + LocalAmmoDelta)) UE_LOG(LogTemp, Error, TEXT("Incorrect Value for Local Prediction"))
	SetAmmo(ServerAmmo + LocalAmmoDelta);
}

FVector ASWeapon::TraceEndWithScatter(const FVector& HitTarget) const
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash"); 
	if (!MuzzleFlashSocket) return FVector();
	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	return TraceEndWithScatter(TraceStart, HitTarget);
}

FVector ASWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget) const
{
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + (ToTargetNormalized * DistanceToSphere);
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	const FVector EndLoc = SphereCenter + RandVec;
	const FVector ToEndLoc = EndLoc - TraceStart;
	
	// DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	// DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Orange, true);
	// DrawDebugLine(
	// 	GetWorld(),
	// 	TraceStart,
	// 	FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()),
	// 	FColor::Cyan,
	// 	true
	// );
	
	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}

void ASWeapon::ShowPickupWidget(bool bShowWidget) const
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void ASWeapon::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ASCharacter* Character = Cast<ASCharacter>(OtherActor))
	{
		Character->SetOverlappingWeapon(this);
	}
}

void ASWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) 
{
	if (ASCharacter* Character = Cast<ASCharacter>(OtherActor))
	{
		Character->SetOverlappingWeapon(nullptr);
	}
}
