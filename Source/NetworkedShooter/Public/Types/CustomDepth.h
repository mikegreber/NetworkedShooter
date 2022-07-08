#pragma once

UENUM(BlueprintType)
enum class ECustomDepthColor : uint8
{
	CDC_None UMETA(DisplayName = "None"),
	CDC_Purple = 250 UMETA(DisplayName = "Purple"),
	CDC_Blue = 251 UMETA(DisplayName = "Blue"),
	CDC_Tan = 252 UMETA(DisplayName = "Tan"),
};

inline void SetCustomDepthColor(UPrimitiveComponent* Component, ECustomDepthColor Color)
{
	if (Component)
	{
		Component->SetRenderCustomDepth(Color != ECustomDepthColor::CDC_None);
		if (Color != ECustomDepthColor::CDC_None)
		{
			Component->SetCustomDepthStencilValue(static_cast<int32>(Color));
		}
	}
}