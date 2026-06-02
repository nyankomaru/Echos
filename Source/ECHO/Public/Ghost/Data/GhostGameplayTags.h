#pragma once

#include "CoreMinimal.h"


#include "NativeGameplayTags.h"


namespace GhostGameplayTags
{
    // Ability タグ：アビリティの識別・アクティブ化に使用
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ghost_Ability_Attack)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ghost_Ability_Dodge)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ghost_Ability_SummonGhost)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ghost_Ability_Corrupt)

    // State タグ：実行中アビリティのブロッキング制御に使用
     UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ghost_State_Attacking)
     UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ghost_State_Dodging)
     UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ghost_State_Corrupted)
     UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ghost_State_Enemy)
}