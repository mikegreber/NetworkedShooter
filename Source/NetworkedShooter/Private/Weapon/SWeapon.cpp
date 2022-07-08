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
#include "PlayerController/SPlayerController.h"
#include "Sound/SoundCue.h"
#include "Weapon/SBulletCasing.h"

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
	SetReplicateMovement(true);
}

void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASWeapon, WeaponState);
	DOREPLIFETIME(ASWeapon, Ammo);
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
		OwnerController = OwnerCharacter->GetPlayerController();
		if (OwnerController == nullptr)
		{
			// call again when controller is set in owner character
			OwnerCharacter->OnPlayerControllerSet.AddUObject(this, &ASWeapon::OnRep_Owner);
			return;
		}

		// make sure all references are set before firing
		bCanFire = OwnerCharacter && OwnerComponent && OwnerController;
		
		// bind UI delegates for local player
		if (IsLocallyControlled())
		{
			OnWeaponAmmoChanged.AddUniqueDynamic(OwnerController, &ASPlayerController::SetHUDWeaponAmmo);
			OnWeaponAmmoChanged.Broadcast(Ammo);
		}
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

		GetWorldTimerManager().ClearAllTimersForObject(this);
		
		bCanFire = false;
	}
}

bool ASWeapon::IsLocallyControlled() const
{
	return OwnerController && OwnerController->IsLocalController();
}

bool ASWeapon::CanFire() const
{
	return bCanFire && Ammo > 0 && OwnerComponent->CanFire();
}

void ASWeapon::Fire(FVector_NetQuantize HitTarget)
{
	if (CanFire())
	{
		bCanFire = false;
	
		if (bUseScatter) HitTarget = TraceEndWithScatter(HitTarget);
	
		LocalFire(HitTarget);
		ServerFire(HitTarget);
	
		StartFireTimer();
	}
}

void ASWeapon::LocalFire(const FVector_NetQuantize& HitTarget)
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

	if (HasAuthority())
	{
		SpendRound();
	}
}

void ASWeapon::ServerFire_Implementation(const FVector_NetQuantize& HitTarget)
{
	MulticastFire(HitTarget);
}

void ASWeapon::MulticastFire_Implementation(const FVector_NetQuantize& HitTarget)
{
	if ( ! IsLocallyControlled() ) LocalFire(const_cast<FVector_NetQuantize&>(HitTarget));
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

	if (IsEmpty())
	{
		OwnerComponent->ReloadWeapon();
	}
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

void ASWeapon::SpendRound()
{
	AddAmmo(-1);
}

void ASWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	OnRep_Ammo();
}

void ASWeapon::OnRep_Ammo() const
{
	OnWeaponAmmoChanged.Broadcast(Ammo);
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
