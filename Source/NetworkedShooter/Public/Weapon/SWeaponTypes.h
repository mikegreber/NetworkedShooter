﻿#pragma once

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