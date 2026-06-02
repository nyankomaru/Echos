// Fill out your copyright notice in the Description page of Project Settings.
#include "Ghost/AbilitySystem/GhostAbilitySystemComponent.h"

UGhostAbilitySystemComponent::UGhostAbilitySystemComponent()
{
}

void UGhostAbilitySystemComponent::GiveAbilityIfNotExists(TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!AbilityClass) return;

	// ‚·‚Å‚É“¯ƒNƒ‰ƒX‚ª•t—^Ï‚Ý‚È‚ç‰½‚à‚µ‚È‚¢
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetClass() == AbilityClass)
		{
			return;
		}
	}

	FGameplayAbilitySpec Spec(AbilityClass, 1);
	GiveAbility(Spec);
}
