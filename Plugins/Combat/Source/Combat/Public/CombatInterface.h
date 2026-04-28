// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"

class AWeapon;

UINTERFACE(MinimalAPI)
class UCombatInterface : public UInterface
{
	GENERATED_BODY()
};


class COMBAT_API ICombatInterface
{
	GENERATED_BODY()

public:
	virtual bool IsEquippedWeapon() const = 0;
	
	virtual bool IsAiming() const = 0;
	
	virtual AWeapon* GetEquippedWeapon() const = 0;
};
