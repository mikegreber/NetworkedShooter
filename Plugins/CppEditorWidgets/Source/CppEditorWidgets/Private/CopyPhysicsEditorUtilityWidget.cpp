// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_EDITOR

#include "CopyPhysicsEditorUtilityWidget.h"
#include "Components/Button.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

void UCopyPhysicsEditorUtilityWidget::ExecuteButtonPressed()
{
	if (FromPhysicsAsset && ToModify)
	{
		if (const ACharacter* Character = Cast<ACharacter>(ToModify->ClassDefaultObject))
		{
			TMap<FName, USkeletalBodySetup*> BodyMap;
			for (auto BodyPtr : FromPhysicsAsset->SkeletalBodySetups)
			{
				if (USkeletalBodySetup* Body = BodyPtr.Get())
				{
					BodyMap.Emplace(Body->BoneName, Body);
				}
			}
			
			TArray<UActorComponent*> CapsuleComponents;
			Character->GetComponents(UCapsuleComponent::StaticClass(), CapsuleComponents);
			TMap<FName, UCapsuleComponent*> CapsuleMap;
			for (UActorComponent* Component : CapsuleComponents)
			{
				if (BodyMap.Contains(Component->GetFName()))
				{
					UE_LOG(LogTemp, Warning, TEXT("FOUND MATCH: %s"), *Component->GetFName().ToString())
					for (const FKSphylElem Sphyl : BodyMap[Component->GetFName()]->AggGeom.SphylElems)
					{
						UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Component);
						Capsule->SetRelativeLocation(Sphyl.Center);
						Capsule->SetRelativeRotation(Sphyl.Rotation);
						Capsule->SetCapsuleRadius(Sphyl.Radius);
						Capsule->SetCapsuleHalfHeight((Sphyl.Length + Sphyl.Radius)/2);
						break;
					}
				}
			}
		}
	}
}

void UCopyPhysicsEditorUtilityWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void UCopyPhysicsEditorUtilityWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ExecuteButton->OnClicked.AddUniqueDynamic(this, &UCopyPhysicsEditorUtilityWidget::ExecuteButtonPressed);
}

#endif