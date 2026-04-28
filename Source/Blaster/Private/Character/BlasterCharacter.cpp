// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/BlasterCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "CombatComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

// 构造函数：初始化角色组件、移动属性
ABlasterCharacter::ABlasterCharacter()
{
	// 开启每帧更新
	PrimaryActorTick.bCanEverTick = true;

	//弹簧臂组件
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 500.f;
	DefaultSocketOffset = SpringArm->SocketOffset;
	SpringArm->SetRelativeRotation(FRotator(-20.f, 0.f, 0.f));
	SpringArm->bUsePawnControlRotation = true;
	
	//摄像机组件
	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(SpringArm);
	
	// 创建头顶UI组件（用于显示名字、状态等）
	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>("OverheadWidget");
	OverheadWidget->SetupAttachment(RootComponent);

	// 创建武器组件
	CombatComponent = CreateDefaultSubobject<UCombatComponent>("WeaponComponent");
	
	// 设置最大移动速度
	GetCharacterMovement()->MaxWalkSpeed = CombatComponent->GetBaseWalkSpeed();
	
	// 默认状态旋转朝向运动
	GetCharacterMovement()->bOrientRotationToMovement = true;
	
	// 开启角色下蹲功能
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	// 设置下蹲时的角色半高
	GetCharacterMovement()->SetCrouchedHalfHeight(80.f);
	// 设置下蹲时的最大移动速度
	GetCharacterMovement()->MaxWalkSpeedCrouched = 200.f;
	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(50.f);
}

// 游戏开始时执行
void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 获取玩家控制器，添加输入映射上下文（绑定按键操作）
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			// 添加默认的输入映射，让WASD、鼠标等生效
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	AimOffset(DeltaTime);

	// 瞄准时平滑调整摄像机
	if (SpringArm)
	{
		const float TargetLength = IsAiming() ? 200.f : 500.f;
		SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetLength, DeltaTime, 10.f);

		const FVector TargetOffset = IsAiming() ? ZoomedSocketOffset : DefaultSocketOffset;
		SpringArm->SocketOffset = FMath::VInterpTo(SpringArm->SocketOffset, TargetOffset, DeltaTime, 10.f);
	}
}

// 网络复制属性注册（用于多人游戏同步数据）
void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 同步武器组件到所有客户端
	DOREPLIFETIME(ABlasterCharacter, CombatComponent);
}

// 绑定玩家输入（按键、鼠标、摇杆）
void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (const TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 跳跃：按下跳 / 松开停止跳
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		
		// 移动：WASD触发
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Move);
		
		// 视角：鼠标移动
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Look);
		
		// 下蹲：按下切换下蹲/站立
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ABlasterCharacter::Crouched);
		
		// 切换武器：按下执行装备
		EnhancedInputComponent->BindAction(EquipWeaponAction, ETriggerEvent::Started, this, &ABlasterCharacter::EquipWeapon);
		
		// 瞄准：按下开镜 / 松开关镜
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ABlasterCharacter::StartAim);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this,  &ABlasterCharacter::StopAim);
	}
}

/**
 * 执行角色移动
 * @param Right  左右移动输入值
 * @param Forward 前后移动输入值
 */
void ABlasterCharacter::DoMove(const float Right, const float Forward)
{
	if (GetController())
	{
		// 获取控制器朝向，只取Yaw（水平方向）
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		
		// 根据视角计算前后、左右方向
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		
		// 添加移动输入
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

/**
 * 执行视角旋转
 * @param Yaw   水平旋转（左右看）
 * @param Pitch 垂直旋转（上下看）
 */
void ABlasterCharacter::DoLook(const float Yaw, const float Pitch)
{
	if (GetController())
	{
		// 左右旋转视角
		AddControllerYawInput(Yaw);
		// 上下俯仰视角
		AddControllerPitchInput(Pitch);
	}
}

/**
 * 计算角色瞄准偏移（Aim Offset），用于控制角色上半身瞄准动画
 * 站立不动时：角色身体不随视角旋转，上半身做瞄准偏移
 * 移动/跳跃时：角色身体随视角旋转，不使用瞄准偏移
 */
void ABlasterCharacter::AimOffset(float DeltaTime)
{	
	if (CombatComponent && !CombatComponent->IsWeaponEquipped()) return;

	// 获取角色当前速度，并忽略Z轴（只计算地面移动速度）
	FVector GroundVelocity = GetVelocity();
	GroundVelocity.Z = 0.f;
	// 计算地面移动速度大小
	const float GroundSpeed = GroundVelocity.Size();
	// 判断角色是否在空中（跳跃/坠落）
	const bool bIsAir = GetCharacterMovement()->IsFalling();

	// 情况1：角色静止站立 + 不在空中 → 启用瞄准偏移，身体不跟随视角旋转
	if (GroundSpeed <= 10.f && !bIsAir)
	{
		// 获取当前控制器瞄准的Yaw旋转（忽略Pitch/Roll）
		const FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		// 计算当前瞄准方向与初始瞄准方向的旋转差值
		const FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		// 将差值赋值给瞄准偏移Yaw，控制上半身动画
		AimOffsetYaw = DeltaAimRotation.Yaw;
		// 禁用角色Yaw轴跟随控制器旋转（身体不动，上半身动）
		bUseControllerRotationYaw = false;
	}
	// 情况2：角色移动 或 在空中 → 关闭瞄准偏移，身体跟随视角旋转
	else
	{
		// 重置初始瞄准旋转为当前视角
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		// 重置Yaw瞄准偏移
		AimOffsetYaw = 0.f;
		// 启用角色Yaw轴跟随控制器旋转（身体跟随视角）
		bUseControllerRotationYaw = true;
	}

	// 获取控制器瞄准Pitch值，用于垂直方向瞄准偏移
	AimOffsetPitch = GetBaseAimRotation().Pitch;

	// 非本地控制角色（服务器/其他客户端）：修正Pitch角度范围（270~360 → -90~0）
	if (AimOffsetPitch > 90.f && !IsLocallyControlled())
	{
		// 输入角度范围：UE网络传输中俯视会变成270~360
		const FVector2D InRange(270.f, 360.f);
		// 输出角度范围：映射为动画可用的-90~0度
		const FVector2D OutRange(-90.f, 0.f);
		// 角度映射并 clamped 限制范围
		AimOffsetPitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AimOffsetPitch);
	}
}

// 开始跳跃（外部调用接口）
void ABlasterCharacter::DoJumpStart()
{
	Jump();
}

// 结束跳跃（外部调用接口）
void ABlasterCharacter::DoJumpEnd()
{
	StopJumping();
}

// 移动输入回调：处理WASD输入
void ABlasterCharacter::Move(const FInputActionValue& Value)
{
	// 获取二维移动输入（X=左右，Y=前后）
	const FVector2D MovementVector = Value.Get<FVector2D>();
	
	// 执行实际移动
	DoMove(MovementVector.X, MovementVector.Y);

	// 是否向后移动
	const bool bBackward = MovementVector.Y < 0.f;
	// 本地立即更新向后的移动速度
	UpdateWalkSpeed(bBackward, IsEquippedWeapon());
	// 服务器更新向后的移动速度
	Server_UpdateWalkSpeed(bBackward, IsEquippedWeapon());
}

// 视角输入回调：处理鼠标移动
void ABlasterCharacter::Look(const FInputActionValue& Value)
{
	// 获取鼠标移动向量
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	
	// 执行视角旋转（Y轴取反，符合常规FPS操作）（X=左右，-Y=上下）
	DoLook(LookAxisVector.X, -LookAxisVector.Y);
}

// 下蹲/站立 切换
void ABlasterCharacter::Crouched()
{
	//未装备武器不可蹲下
	if (!IsEquippedWeapon()) return;
	if (bIsCrouched)
	{
		// 当前已下蹲 → 站起来
		UnCrouch();
	}
	else
	{
		// 当前站立 → 下蹲
		Crouch();
	}
}

// 装备武器按键回调
void ABlasterCharacter::EquipWeapon()
{
	if (CombatComponent)
		// 调用武器组件的装备方法，绑定到武器插槽
		CombatComponent->EquipButtonPressed(FName("WeaponSocket"));
	
}

// 开始瞄准
void ABlasterCharacter::StartAim()
{
	CombatComponent->SetAiming(true);
}

// 停止瞄准
void ABlasterCharacter::StopAim()
{
	CombatComponent->SetAiming(false);
}

/**
 * 更新移动速度
 * @param bBackward 是否向后移动
 * @param bHasWeapon 是否拥有武器
 */
void ABlasterCharacter::UpdateWalkSpeed(const bool bBackward, const bool bHasWeapon) const
{
	if (!GetCharacterMovement())
		return;

	// 瞄准时走瞄准速度，不区分前后
	if (IsAiming())
	{
		GetCharacterMovement()->MaxWalkSpeed = CombatComponent->GetAimWalkSpeed();
		return;
	}

	// 非瞄准状态：后退且持枪时减速
	GetCharacterMovement()->MaxWalkSpeed = (bBackward && bHasWeapon) ? CombatComponent->GetAimWalkSpeed() : CombatComponent->GetBaseWalkSpeed();
}

/**
 * 服务器更新向后移动的速度
 * @param bBackward 是否向后移动
 * @param bHasWeapon 是否拥有武器
 */
void ABlasterCharacter::Server_UpdateWalkSpeed_Implementation(const bool bBackward, const bool bHasWeapon)
{
	UpdateWalkSpeed(bBackward,bHasWeapon);
}

// 判断：是否已装备武器
bool ABlasterCharacter::IsEquippedWeapon() const
{
	return CombatComponent && CombatComponent->IsWeaponEquipped();
}

// 判断：是否正在瞄准
bool ABlasterCharacter::IsAiming() const
{
	return CombatComponent && CombatComponent->IsAiming();
}

AWeapon* ABlasterCharacter::GetEquippedWeapon() const
{
	if (!CombatComponent) 
		return nullptr;
	return CombatComponent->GetEquippedWeapon();
}
