// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

enum class EWeaponState : uint8;
class UWidgetComponent;
class USphereComponent;

UCLASS()
class COMBAT_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();

	void ShowPickUpWidget(bool bShowWidget) const;
	USphereComponent* GetAreaShape() {return AreaShape;}
	
	void SetWeaponState(const EWeaponState InWeaponState);
	
	USkeletalMeshComponent* GetWeaponSkeletalMesh() {return WeaponSkeletalMesh;}
	
protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);
	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
private:
	UPROPERTY(VisibleAnywhere, Category="Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponSkeletalMesh;
	
	UPROPERTY(VisibleAnywhere, Category="Weapon")
	TObjectPtr<USphereComponent> AreaShape;
	
	UPROPERTY(VisibleAnywhere, Category="Weapon Widget")
	TObjectPtr<UWidgetComponent> PickUpWidget;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category="WeaponState")
	EWeaponState WeaponState;
	
	UFUNCTION()
	void OnRep_WeaponState();
};
