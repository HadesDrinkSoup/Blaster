// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BlasterAnimInstance.generated.h"

enum class ETurningInPlace : uint8;
class AWeapon;
class ICombatInterface;
class ABlasterCharacter;

/**
 * 角色动画实例类
 * 作用：驱动角色动画蓝图，获取角色运动、武器、状态等信息，控制动画切换
 */
UCLASS()
class BLASTER_API UBlasterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	/** 动画实例初始化时调用（只执行一次） */
	virtual void NativeInitializeAnimation() override;
	
	/** 动画每帧更新，用于更新动画所需的状态变量 */
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	/** 持有当前动画的角色对象 */
	UPROPERTY(BlueprintReadOnly, Category= "Character", meta=(AllowPrivateAccess=true))
	TObjectPtr<ABlasterCharacter> BlasterCharacter;

	/** 武器接口：用于获取装备、瞄准状态 */
	TObjectPtr<ICombatInterface> CombatInterface;

	/** 角色地面移动速度（控制移动动画） */
	UPROPERTY(BlueprintReadOnly, Category= "Character", meta=(AllowPrivateAccess=true))
	float GroundSpeed;

	/** 角色移动方向（相对于朝向），用于控制方向混合动画 */
	UPROPERTY(BlueprintReadOnly, Category= "Character", meta=(AllowPrivateAccess=true))
	float Direction;
	
	/** 角色瞄准偏移左右旋转 */
	UPROPERTY(BlueprintReadOnly, Category= "Character", meta=(AllowPrivateAccess=true))
	float AimOffsetYaw;
	
	/** 角色瞄准偏移上下旋转 */
	UPROPERTY(BlueprintReadOnly, Category= "Character", meta=(AllowPrivateAccess=true))
	float AimOffsetPitch;

	/** 是否在空中（跳跃/下落） */
	UPROPERTY(BlueprintReadOnly, Category= "Character", meta=(AllowPrivateAccess=true))
	bool bIsInAir;

	/** 是否正在加速移动（WASD按住） */
	UPROPERTY(BlueprintReadOnly, Category= "Character", meta=(AllowPrivateAccess=true))
	bool bIsAccelerating;

	/** 是否处于下蹲状态 */
	UPROPERTY(BlueprintReadOnly, Category= "Character", meta=(AllowPrivateAccess=true))
	bool bIsCrouched;

	/** 是否装备了武器 */
	UPROPERTY(BlueprintReadOnly, Category= "Weapon", meta=(AllowPrivateAccess=true))
	bool bIsEquippedWeapon;
	
	/** 装备的武器 */
	UPROPERTY(BlueprintReadOnly, Category= "Weapon", meta=(AllowPrivateAccess=true))
	TObjectPtr<AWeapon> EquippedWeapon;
	
	/** 手部ik的左手变换 */
	UPROPERTY(BlueprintReadOnly, Category= "Weapon", meta=(AllowPrivateAccess=true))
	FTransform LeftHandTransform;

	/** 是否正在瞄准 */
	UPROPERTY(BlueprintReadOnly, Category= "Weapon", meta=(AllowPrivateAccess=true))
	bool bIsAiming;
};