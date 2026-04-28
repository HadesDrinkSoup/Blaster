// Fill out your copyright notice in the Description page of Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class AWeapon;
class ACombat;

/**
 * 武器组件：依附于角色，负责武器的装备、切换、瞄准、重叠检测、网络同步
 * 属于角色核心战斗组件
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COMBAT_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	/** 构造函数：初始化组件默认属性 */
	UCombatComponent();

	/** 获取当前正在重叠的武器（只读） */
	const AWeapon* GetOverlappingWeapon() const {return OverlappingWeapon;}

	/** 是否正在瞄准 */
	bool IsAiming() const {return bIsAiming;}

	/** 是否已经装备了武器 */
	bool IsWeaponEquipped() const {return EquippedWeapon != nullptr;}
	
	/** 获取已装备的武器 */
	AWeapon* GetEquippedWeapon() const {return EquippedWeapon;}

protected:
	/** 游戏开始时初始化 */
	virtual void BeginPlay() override;

	/** 注册需要网络同步的属性（多人游戏必备） */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** 拥有战斗组件的Character */
	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter;
	
	/** 是否正在瞄准（同步到所有客户端） */
	UPROPERTY(Replicated)
	bool bIsAiming;

	/** 基础移动速度 */
	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess=true))
	float BaseWalkSpeed;
	/** 瞄准移动速度 */
	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess=true))
	float AimWalkSpeed;
	
	/**
	 * 当前装备的武器
	 * 同步 + 变化时自动调用 OnRep_EquippedWeapon
	 */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<AWeapon> EquippedWeapon;

	/**
	 * 当前重叠（可拾取）的武器
	 * 同步 + 变化时自动调用 OnRep_OverlappingWeapon
	 */
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	TObjectPtr<AWeapon> OverlappingWeapon;

	/** 执行武器装备（服务器权威调用） */
	void EquipWeapon(AWeapon* Weapon, const FName& SocketName);

public:
	/** 获取瞄准移动速度 */
	float GetAimWalkSpeed() const {return AimWalkSpeed;}

	/** 获取基础移动速度 */
	float GetBaseWalkSpeed() const {return BaseWalkSpeed;}

	/** 设置当前重叠的可拾取武器（本地客户端调用） */
	void SetOverlappingWeapon(AWeapon* Weapon);

	/** 设置瞄准状态（本地客户端调用） */
	void SetAiming(const bool bInIsAiming);

	/** 装备按键按下时调用（本地输入入口） */
	void EquipButtonPressed(const FName& SocketName);

private:

	/** 服务器同步回调：当EquippedWeapon被同步到客户端时调用 */
	UFUNCTION()
	void OnRep_EquippedWeapon();

	/** 服务器同步回调：当OverlappingWeapon被同步到客户端时调用 */
	UFUNCTION()
	void OnRep_OverlappingWeapon(const AWeapon* OldOverlappingWeapon );

	/** 服务器可靠调用，服务器装备武器的实际执行函数，本地不能直接装备，必须发给服务器执行*/
	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(AWeapon* Weapon,const FName& SocketName);

	// 服务器可靠调用，设置瞄准状态的实际执行函数
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(const bool bInIsAiming);
};