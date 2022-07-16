#pragma once

#define TRACE_LENGTH 80'000.f

namespace CUSTOM_DEPTH
{
	const int32 PURPLE = 250;
	const int32 BLUE = 251;
	const int32 TAN = 252;
	
}

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	EWT_SubmachineGun UMETA(DisplayName = "SubmachineGun"),
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),
	EWT_SniperRifle UMETA(DisplayName = "Sniper Rifle"),
	EWT_GrenadeLauncher UMETA(DisplayName = "Grenade Launcher"),
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),

	EWT_DefaultMAX UMETA(DisplayName = "DefaultMAX"),
};

struct FHit
{
	FHit() : Head(0), Body(0) {}
	FHit(const FName BoneName) : Head(BoneName == "Head"), Body(BoneName != "Head") {}
	
	int32 Head;
	int32 Body;

	FHit& operator+=(const FHit& Hit)
	{
		Head += Hit.Head;
		Body += Hit.Body;
		return *this;
	}
};