#pragma once

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWeaponState_Initial UMETA(DisplayName = "初始状态"),
	EWeaponState_Equipped UMETA(DisplayName = "已装备"),
	EWeaponState_Dropped UMETA(DisplayName = "掉落"),
	EWeaponState_Max UMETA(DisplayName = "默认最大值")
};
