// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetManager/SAssetManager.h"
#include "AbilitySystemGlobals.h"

USAssetManager& USAssetManager::Get()
{
	USAssetManager* Singleton = Cast<USAssetManager>(GEngine->AssetManager);

	if (Singleton)
	{
		return *Singleton;
	}
	else
	{
		UE_LOG(LogTemp, Fatal, TEXT("Invalid AssetManager in DefaultEngine.ini, must be GDAssetManager!"));
		return *NewObject<USAssetManager>();	 // never calls this
	}
}

void USAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	UAbilitySystemGlobals::Get().InitGlobalData();
	UAbilitySystemGlobals::Get().ShouldReplicateActivationOwnedTags();
}
