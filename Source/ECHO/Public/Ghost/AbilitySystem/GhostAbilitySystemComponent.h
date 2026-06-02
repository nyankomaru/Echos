// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GhostAbilitySystemComponent.generated.h"

/**
 * 
 */
UCLASS()
class ECHO_API UGhostAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:
	UGhostAbilitySystemComponent();

	/**
	 * アビリティをクラス指定で付与する（重複付与を防ぐ）
	 * @param AbilityClass 付与するアビリティクラス
	 */
	void GiveAbilityIfNotExists(TSubclassOf<UGameplayAbility> AbilityClass);
};
