#include "Ghost/AbilitySystem/GA_GhostAttack.h"
#include "Ghost/Data/GhostGameplayTags.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Player/CombatComponent.h"

UGA_GhostAttack::UGA_GhostAttack()
{
	ActivationBlockedTags.AddTag(GhostGameplayTags::Ghost_State_Attacking);

	// アビリティ実行中は Ghost_State_Attacking タグを付与する
	ActivationOwnedTags.AddTag(GhostGameplayTags::Ghost_State_Attacking);

	// ネットワーク不使用のためローカルのみで実行
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;

	// 1インスタンスで再利用
	InstancingPolicy = EGameplayAbilityInstancingPolicy::NonInstanced;
}

void UGA_GhostAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AGhostCharacter* Ghost = Cast<AGhostCharacter>(ActorInfo->AvatarActor.Get());
	if (!Ghost || !Ghost->CombatComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 攻撃処理を CombatComponent に委譲
	Ghost->CombatComponent->ExecuteAttack();

	// アビリティ自体はすぐ終了（攻撃状態の管理は CombatComponent が行う）
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UGA_GhostAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// CombatComponent が攻撃中なら新たに起動しない（連打防止）
	if (AGhostCharacter* Ghost = Cast<AGhostCharacter>(ActorInfo->AvatarActor.Get()))
	{
		if (Ghost->CombatComponent && Ghost->CombatComponent->IsAttacking())
		{
			return false;
		}
	}

	return true;
}
