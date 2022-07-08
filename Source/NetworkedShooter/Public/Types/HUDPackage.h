#pragma once

#include "CoreMinimal.h"
#include "HUDPackage.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	UTexture2D* CrosshairsCenter = nullptr;
	UPROPERTY(EditAnywhere)
	UTexture2D* CrosshairsLeft = nullptr;
	UPROPERTY(EditAnywhere)
	UTexture2D* CrosshairsRight = nullptr;
	UPROPERTY(EditAnywhere)
	UTexture2D* CrosshairsTop = nullptr;
	UPROPERTY(EditAnywhere)
	UTexture2D* CrosshairsBottom = nullptr;
	float CrosshairSpread;
	FLinearColor CrosshairsColor = FColor::White;

	void SetCrosshairs(const FHUDPackage& Package)
	{
		CrosshairsCenter = Package.CrosshairsCenter;
		CrosshairsLeft = Package.CrosshairsLeft;
		CrosshairsRight = Package.CrosshairsRight;
		CrosshairsTop = Package.CrosshairsTop;
		CrosshairsBottom = Package.CrosshairsBottom;
	}
};