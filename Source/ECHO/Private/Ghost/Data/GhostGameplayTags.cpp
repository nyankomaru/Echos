#include "Ghost/Data/GhostGameplayTags.h"

namespace GhostGameplayTags
{
    UE_DEFINE_GAMEPLAY_TAG(Ghost_Ability_Attack, "Ghost.Ability.Attack")
    UE_DEFINE_GAMEPLAY_TAG(Ghost_Ability_Dodge, "Ghost.Ability.Dodge")
    UE_DEFINE_GAMEPLAY_TAG(Ghost_Ability_SummonGhost, "Ghost.Ability.SummonGhost")
    UE_DEFINE_GAMEPLAY_TAG(Ghost_Ability_Corrupt, "Ghost.Ability.Corrupt")

    UE_DEFINE_GAMEPLAY_TAG(Ghost_State_Attacking, "Ghost.State.Attacking")
    UE_DEFINE_GAMEPLAY_TAG(Ghost_State_Dodging, "Ghost.State.Dodging")
    UE_DEFINE_GAMEPLAY_TAG(Ghost_State_Corrupted, "Ghost.State.Corrupted")
    UE_DEFINE_GAMEPLAY_TAG(Ghost_State_Enemy, "Ghost.State.Enemy")
}