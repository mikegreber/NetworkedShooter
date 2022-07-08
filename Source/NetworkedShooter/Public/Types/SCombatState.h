#pragma once

UENUM(BlueprintType)
enum class ESCombatState : uint8
{
    ECS_Unoccupied UMETA(DisplayName = "Unoccupied"),
    ECS_Reloading UMETA(DisplayName = "Reloading"),
    ECS_ThrowingGrenade UMETA(DisplayName = "Throwing Grenade"),
    ECS_MAX UMETA(DisplayName = "DefaultMAX")
};