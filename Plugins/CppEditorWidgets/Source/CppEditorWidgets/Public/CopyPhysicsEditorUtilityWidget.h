// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "CopyPhysicsEditorUtilityWidget.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class CPPEDITORWIDGETS_API UCopyPhysicsEditorUtilityWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, Category="Modify")
	UPhysicsAsset* FromPhysicsAsset;

	UPROPERTY(EditAnywhere, Category="Modify")
	TSubclassOf<ACharacter> ToModify;
	
	UPROPERTY(meta = (BindWidget))
	class UButton* ExecuteButton;
public:
	UFUNCTION(BlueprintCallable)
	void ExecuteButtonPressed();

protected:
	virtual void NativePreConstruct() override;

	virtual void NativeConstruct() override;
};

#endif