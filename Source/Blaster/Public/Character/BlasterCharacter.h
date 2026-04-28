// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatInterface.h"
#include "GameFramework/Character.h"
#include "BlasterCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UCombatComponent;
class UWidgetComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * Blaster 游戏角色类
 * 功能：角色移动、视角控制、跳跃、下蹲、武器装备、瞄准等核心逻辑
 * 继承：ACharacter（角色基类）+ ICombatInterface（武器交互接口）
 */
UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public ICombatInterface
{
    GENERATED_BODY()

public:
    // 构造函数：初始化组件、移动、碰撞等基础属性
    ABlasterCharacter();
    
    // 武器接口实现
    /** 判断当前角色是否装备了武器 */
    virtual bool IsEquippedWeapon() const override;

    /** 判断当前角色是否正在瞄准 */
    virtual bool IsAiming() const override;
    
    /** 获取已装备的武器 */
    virtual AWeapon*  GetEquippedWeapon() const override;
    
    //瞄准偏移
    float GetAimOffsetYaw() const {return AimOffsetYaw;}
    float GetAimOffsetPitch() const {return AimOffsetPitch;}
    
protected:
    /** 游戏开始时调用，用于初始化输入、状态等 */
    virtual void BeginPlay() override;
    
    virtual void Tick(float DeltaTime) override;

    /** 网络复制属性注册：多人游戏同步变量用 */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** 绑定输入：按键、鼠标、摇杆映射到函数 */
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    
    // 角色行为：移动、视角、跳跃
    /** 执行移动（根据左右、前后输入） */
    virtual void DoMove(float Right, float Forward);

    /** 执行视角旋转 */
    virtual void DoLook(float Yaw, float Pitch);

    /** 瞄准偏移 */
    void AimOffset(float DeltaTime);
    
    /** 开始跳跃 */
    virtual void DoJumpStart();

    /** 停止跳跃 */
    virtual void DoJumpEnd();

    /** 切换下蹲/站立 */
    void Crouched();
    
    // 武器与瞄准
    /** 装备/切换武器 */
    void EquipWeapon();

    /** 开始瞄准 */
    void StartAim();

    /** 停止瞄准 */
    void StopAim();

    /** 更新向后移动的速度 */
    void UpdateWalkSpeed(bool bBackward, bool bHasWeapon) const;
    
    /** 服务器更新向后移动的速度 */
    UFUNCTION(Server, Reliable)
    void Server_UpdateWalkSpeed(const bool bBackward, const bool bHasWeapon);
private:
    // 组件
    /** 弹簧臂组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
    TObjectPtr<USpringArmComponent> SpringArm;
    
    /** 摄像机组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
    TObjectPtr<UCameraComponent> Camera;
    
    /** 头顶UI组件：显示名字、状态、血条等 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
    TObjectPtr<UWidgetComponent> OverheadWidget;

    /** 战斗组件：管理装备、切换、开火、瞄准 */
    UPROPERTY(Replicated, VisibleAnywhere, Category = "CombatComponent")
    TObjectPtr<UCombatComponent> CombatComponent;

	/** 默认摄像机偏移 */
	FVector DefaultSocketOffset;
	
	/** 瞄准时摄像机偏移目标值 */
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	FVector ZoomedSocketOffset = FVector(0.f, 60.f, 50.f);
	
    // 输入回调函数
    /**
     * 移动输入回调：WASD触发
     * @param Value 输入的二维向量（X=左右，Y=前后）
     */
    void Move(const FInputActionValue& Value);

    /**
     * 视角输入回调：鼠标移动触发
     * @param Value 鼠标移动的二维向量
     */
    void Look(const FInputActionValue& Value);
    
    // 输入资源（在编辑器中指定）
    /** 默认输入映射上下文：绑定整套操作规则 */
    UPROPERTY(EditDefaultsOnly, Category="Input", meta=(AllowPrivateAccess = "true"))
    UInputMappingContext* DefaultMappingContext;

    /** 跳跃输入动作 */
    UPROPERTY(EditDefaultsOnly, Category="Input", meta=(AllowPrivateAccess = "true"))
    UInputAction* JumpAction;

    /** 移动输入动作 */
    UPROPERTY(EditDefaultsOnly, Category="Input", meta=(AllowPrivateAccess = "true"))
    UInputAction* MoveAction;

    /** 鼠标视角控制输入动作 */
    UPROPERTY(EditDefaultsOnly, Category="Input", meta=(AllowPrivateAccess = "true"))
    UInputAction* MouseLookAction;

    /** 装备武器输入动作 */
    UPROPERTY(EditDefaultsOnly, Category="Input", meta=(AllowPrivateAccess = "true"))
    UInputAction* EquipWeaponAction;

    /** 下蹲输入动作 */
    UPROPERTY(EditDefaultsOnly, Category="Input", meta=(AllowPrivateAccess = "true"))
    UInputAction* CrouchAction;

    /** 瞄准输入动作 */
    UPROPERTY(EditDefaultsOnly, Category="Input", meta=(AllowPrivateAccess = "true"))
    UInputAction* AimAction;
    
    float AimOffsetYaw;
    float AimOffsetPitch;
    
    FRotator StartingAimRotation;
};