// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#define ECC_SkeletalMesh ECollisionChannel::ECC_GameTraceChannel1
#define ECC_RewindCollider ECollisionChannel::ECC_GameTraceChannel2
#define ECC_RewindTrace ECollisionChannel::ECC_GameTraceChannel3
#define ECC_Projectile ECollisionChannel::ECC_GameTraceChannel4

#define NET_ROLE_STRING_ACTOR FString(HasAuthority() ? "Server" : "Client") 
#define NET_ROLE_STRING_COMPONENT FString(GetOwner()->HasAuthority() ? "Server" : "Client")

#ifdef WITH_EDITOR
#define SERVER_ONLY() if (!GetOwner()->HasAuthority()) UE_LOG(LogTemp, Error, TEXT("%s called on client, should only be called from server!"), __FUNCTIONW__)
#define LOCALLY_CONTROLLED_ONLY(Pawn) if (!Pawn || !Pawn->IsLocallyControlled()) UE_LOG(LogTemp, Error, TEXT("%s called on remote, should only be called locally!"), __FUNCTIONW__)
#else
#define SERVER_ONLY() {}
#define LOCALLY_CONTROLLED_ONLY(Pawn) {}
#endif
