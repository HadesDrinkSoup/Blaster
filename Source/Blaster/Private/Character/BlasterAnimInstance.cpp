// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/BlasterAnimInstance.h"
#include "KismetAnimationLibrary.h"
#include "Weapon.h"
#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

/**
 * 动画实例初始化（只执行一次）
 * 用于获取动画所属的角色对象，并缓存角色和武器接口
 */
void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// 获取当前动画实例依附的 Pawn（角色），并转为 BlasterCharacter
	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());

	// 如果角色有效，将其转为武器接口，方便后续获取武器状态
	if (BlasterCharacter)
		CombatInterface = Cast<ICombatInterface>(BlasterCharacter);
}

/**
 * 动画每帧更新函数
 * 实时更新角色运动、状态、武器信息，供动画蓝图使用
 * @param DeltaSeconds 帧间隔时间
 */
void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 如果角色指针为空，重新获取一次
	if (!BlasterCharacter) 
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());

	// 角色无效则直接退出，避免空指针崩溃
	if (!BlasterCharacter)
		return;
	
	// 计算角色地面移动速度（忽略Z轴，只算水平速度）
	FVector GroundVelocity = BlasterCharacter->GetVelocity();
	GroundVelocity.Z = 0.f;
	GroundSpeed = GroundVelocity.Size();
	
	// 计算角色移动方向（相对于自身朝向）
	// 用于动画蓝图的方向混合（前/后/左/右）
	const FRotator GroundRotation = BlasterCharacter->GetActorRotation();
	Direction = UKismetAnimationLibrary::CalculateDirection(GroundVelocity, GroundRotation);
	
	// 更新角色运动状态
	// 是否在空中（跳跃/下落）
	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
	// 是否正在加速（按住WASD）
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f;
	// 是否处于下蹲状态
	bIsCrouched = BlasterCharacter->bIsCrouched;
	
	// 通过武器接口获取武器相关状态（解耦）
	if (CombatInterface)
	{
		// 是否装备武器
		bIsEquippedWeapon = CombatInterface->IsEquippedWeapon();
		// 是否正在瞄准
		bIsAiming = CombatInterface->IsAiming();
		// 获取装备的武器
		EquippedWeapon = CombatInterface->GetEquippedWeapon();
	}
	else
	{
		// 接口无效时，默认状态为未装备、未瞄准
		bIsEquippedWeapon = false;
		bIsAiming = false;
	}
	
	//瞄准偏移
	AimOffsetYaw = BlasterCharacter->GetAimOffsetYaw();
	AimOffsetPitch = BlasterCharacter->GetAimOffsetPitch();
	
	//左手ik
	if (bIsEquippedWeapon && EquippedWeapon && EquippedWeapon->GetWeaponSkeletalMesh() && BlasterCharacter->GetMesh())
	{
		// 获取武器上左手插槽(LeftHandSocket)的世界空间变换信息
		LeftHandTransform = EquippedWeapon->GetWeaponSkeletalMesh()->GetSocketTransform(FName("LeftHandSocket"), RTS_World);

		// 声明用于接收骨骼空间转换结果的位置和旋转量
		FVector OutPosition;
		FRotator OutRotation;

		// 将武器左手插槽的世界空间位置，转换到角色右手骨骼(hand_r)的局部空间
		// 目的：让左手IK精准贴合武器握把位置
		BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);

		// 把转换后的骨骼空间位置赋值给左手IK变换
		LeftHandTransform.SetLocation(OutPosition);

		// 把转换后的骨骼空间旋转赋值给左手IK变换（转为四元数）
		LeftHandTransform.SetRotation(FQuat(OutRotation));
	}
}