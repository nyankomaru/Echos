
#include "Ghost/AbilitySystem/GA_GhostDodge.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Player/CombatComponent.h"
#include "Ghost/Data/GhostGameplayTags.h"

UGA_GhostDodge::UGA_GhostDodge()
{
	ActivationBlockedTags.AddTag(GhostGameplayTags::Ghost_State_Dodging);
	ActivationOwnedTags.AddTag(GhostGameplayTags::Ghost_State_Dodging);

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::NonInstanced;
}

void UGA_GhostDodge::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AGhostCharacter* Ghost = Cast<AGhostCharacter>(ActorInfo->AvatarActor.Get());
	if (!Ghost || !Ghost->CombatComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ‰ñ”ðˆ—‚ð CombatComponent ‚ÉˆÏ÷
	Ghost->CombatComponent->ExecuteDodge();

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}