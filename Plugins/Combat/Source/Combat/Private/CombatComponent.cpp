// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatComponent.h"
#include "Weapon.h"
#include "Combat/CombatType.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

// 组件构造函数：设置默认属性
UCombatComponent::UCombatComponent()
{
	// 关闭每帧更新，提升性能
	PrimaryComponentTick.bCanEverTick = false;
	// 开启组件默认网络复制（多人游戏必备）
	SetIsReplicatedByDefault(true);
	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 200.f;
}

// 游戏开始时初始化
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

// 注册需要网络同步的变量
void UCombatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 仅同步给武器所有者（减少网络流量）
	DOREPLIFETIME_CONDITION(UCombatComponent, OverlappingWeapon, COND_OwnerOnly);
	// 同步当前装备的武器
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	// 同步瞄准状态
	DOREPLIFETIME(UCombatComponent, bIsAiming);
}


/**
 * 执行武器装备
 * @param Weapon 要装备的武器
 * @param SocketName 绑定到角色的插槽名
 */
void UCombatComponent::EquipWeapon(AWeapon* Weapon, const FName& SocketName)
{
	if (!Weapon || !GetOwner()) 
		return;
	
	if (!OwnerCharacter) 
		return;
	
	// 设置当前装备的武器
	EquippedWeapon = Weapon;
	// 将武器状态设置为“已装备”
	EquippedWeapon->SetWeaponState(EWeaponState::EWeaponState_Equipped);
	// 设置武器的所有者为角色
	EquippedWeapon->SetOwner(OwnerCharacter); 
	// 装备后清空重叠武器
	OverlappingWeapon = nullptr;

	// 装备武器后：关闭角色自动朝向移动方向，开启相机朝向控制
	OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	OwnerCharacter->bUseControllerRotationYaw = true;
	
	// 将武器绑定到角色指定的骨骼插槽上
	const USkeletalMeshSocket* WeaponSocket = OwnerCharacter->GetMesh()->GetSocketByName(SocketName);
	if (WeaponSocket)
		EquippedWeapon->AttachToComponent(OwnerCharacter->GetMesh(),FAttachmentTransformRules::SnapToTargetIncludingScale,SocketName);
	
}

void UCombatComponent::PlayFireMontage()
{
	if (!EquippedWeapon) return;
	const TObjectPtr<UAnimInstance> AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
	if (AnimInstance && FireMontage)
	{
		AnimInstance->Montage_Play(FireMontage);
		const bool IsCrouching = OwnerCharacter->GetCharacterMovement()->IsCrouching();
		const FName SectionName = IsCrouching ? FName("CrouchFire") : FName("StandFire");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

/**
 * 设置当前重叠的武器（本地客户端调用）
 * @param Weapon 重叠的武器对象
 */
void UCombatComponent::SetOverlappingWeapon(AWeapon* Weapon)
{
	// 保存旧的重叠武器
	const AWeapon* OldOverlappingWeapon = OverlappingWeapon;
	// 更新为新的重叠武器
	OverlappingWeapon = Weapon;

	// 仅本地玩家显示/隐藏拾取UI
	const APawn* OwnerPawn = Cast<APawn>(OwnerCharacter);
	if (OwnerPawn && OwnerPawn->IsLocallyControlled())
	{
		// 旧武器：隐藏拾取提示
		if (OldOverlappingWeapon)
			OldOverlappingWeapon->ShowPickUpWidget(false);
		// 新武器：显示拾取提示
		if (OverlappingWeapon)
			OverlappingWeapon->ShowPickUpWidget(true);
	}
}

/**
 * 设置瞄准状态（本地客户端调用）
 * @param bPressed true=开始瞄准 / false=停止瞄准
 */
void UCombatComponent::AimButtonPressed(const bool& bPressed)
{
	/**
	 * 客户端预测 + 服务器确认
	 * 瞄准：无副作用、可预测，客户端先行，服务器确认，本地和 RPC 同时调用
	 */
	// 本地立即设置瞄准状态（提升手感）
	bIsAiming = bPressed;
	// 发送请求给服务器同步状态
	ServerAimButtonPressed(bPressed);
	if (OwnerCharacter)
	{
		OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}


/**
 * 装备按键按下时调用（本地输入入口）
 * @param SocketName 装备到哪个插槽
 */
void UCombatComponent::EquipButtonPressed(const FName& SocketName)
{
	// 没有可重叠武器则直接返回
	if (!OverlappingWeapon) 
		return;

	/**
	 * 严格服务器权威
	 * 装备：有副作用、需绝对一致，客户端只请求，服务器执行，不能同时在客户端执行
	 */
	// 如果是服务器，直接装备
	if (GetOwner()->HasAuthority())
	{
		EquipWeapon(OverlappingWeapon, SocketName);
	}
	// 如果是客户端，发送RPC请求服务器装备
	else
	{
		ServerEquipWeapon(OverlappingWeapon, SocketName);
	}
}

void UCombatComponent::FireButtonPressed(const bool& bPressed)
{
	bIsFire = bPressed;
	if (bPressed)
	{
		ServerFireButtonPressed(bPressed);
	}
}

/**
 * 服务器同步回调：当 EquippedWeapon 发生变化时调用
 * 用于客户端刷新武器显示状态
 */
void UCombatComponent::OnRep_EquippedWeapon()
{
	if (OwnerCharacter && EquippedWeapon)
	{
		/**
		 * bOrientRotationToMovement 和 bUseControllerRotationYaw 没有 Replicated 标记
		 * 修改它们时，修改只会在执行修改的那台机器上生效
		 * 所以装备武器时让客户端同步这些非复制属性，确保表现与服务器一致
		 */
		OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		OwnerCharacter->bUseControllerRotationYaw = true;
	}
}

/**
 * 服务器同步回调：当 OverlappingWeapon 发生变化时调用
 * @param OldOverlappingWeapon 变化前的旧武器
 */
void UCombatComponent::OnRep_OverlappingWeapon(const AWeapon* OldOverlappingWeapon)
{
	const APawn* OwnerPawn = Cast<APawn>(OwnerCharacter);
	if (OwnerPawn && OwnerPawn->IsLocallyControlled())
	{
		if (OldOverlappingWeapon)
			OldOverlappingWeapon->ShowPickUpWidget(false);
		if (OverlappingWeapon)
			OverlappingWeapon->ShowPickUpWidget(true);
	}
}


/**
 * 服务器装备武器的实际执行函数
 * 本地不能直接装备，必须发给服务器执行
 * @param Weapon 要装备的武器
 * @param SocketName 绑定到角色的插槽名 
 */
void UCombatComponent::ServerEquipWeapon_Implementation(AWeapon* Weapon, const FName& SocketName)
{
	EquipWeapon(Weapon, SocketName);
}

/**
 * 设置瞄准状态
 * 瞄准状态必须由服务器权威同步
 * @param bPressed 瞄准按键是否按下
 */
void UCombatComponent::ServerAimButtonPressed_Implementation(const bool bPressed)
{
	bIsAiming = bPressed;
	if (OwnerCharacter)
	{
		OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

/**
 * 设置开火状态
 * @param bPressed 开火按键是否按下
 */
void UCombatComponent::ServerFireButtonPressed_Implementation(const bool bPressed)
{
	if (OwnerCharacter && EquippedWeapon && bIsAiming)
	{
		MulticastFire();
	}
}

/**
 * 服务器向所有端同步开火动画、特效
 * 画蒙太奇 (PlayFireMontage) 是纯本地操作，播放后不会自动复制到客户端
 */
void UCombatComponent::MulticastFire_Implementation()
{
	PlayFireMontage();
	EquippedWeapon->Fire(); // 注意：这里 Fire 是纯表现（特效、音效）
}