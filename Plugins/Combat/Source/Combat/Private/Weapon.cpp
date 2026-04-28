// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

#include "CombatComponent.h"
#include "Combat/CombatType.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
	/**
	 * 必须已武器为根组件，才能正确附着在正确位置
	 * 因为武器的原点在把手，以武器为根组件创建的Actor原点才会在武器把手
	 */
	WeaponSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>("WeaponSkeletalMesh");
	WeaponSkeletalMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponSkeletalMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	WeaponSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = WeaponSkeletalMesh;
	
	AreaShape = CreateDefaultSubobject<USphereComponent>("AreaShape");
	AreaShape->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaShape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AreaShape->SetupAttachment(RootComponent);
	
	PickUpWidget = CreateDefaultSubobject<UWidgetComponent>("PickUpWidget");
	PickUpWidget->SetVisibility(false);
	PickUpWidget->SetupAttachment(RootComponent);
}

void AWeapon::SetWeaponState(const EWeaponState InWeaponState)
{
	WeaponState = InWeaponState;
	switch (WeaponState)
	{
	case EWeaponState::EWeaponState_Equipped:
		ShowPickUpWidget(false);
		AreaShape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWeaponState_Dropped:
		ShowPickUpWidget(true);
		AreaShape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		break;
	}
}

void AWeapon::ShowPickUpWidget(const bool bShowWidget) const
{
	if (PickUpWidget)
	{
		PickUpWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		AreaShape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		AreaShape->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		AreaShape->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereBeginOverlap);
		AreaShape->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlap);
	}
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeapon, WeaponState);
}

void AWeapon::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) return;
	UCombatComponent* CombatComponent = OtherActor->FindComponentByClass<UCombatComponent>();
	if (CombatComponent)
	{
		CombatComponent->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor) return;
	UCombatComponent* CombatComponent = OtherActor->FindComponentByClass<UCombatComponent>();
	if (CombatComponent)
	{
		CombatComponent->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWeaponState_Equipped:
		ShowPickUpWidget(false);
		AreaShape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWeaponState_Dropped:
		ShowPickUpWidget(true);
		AreaShape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		break;
	}
}