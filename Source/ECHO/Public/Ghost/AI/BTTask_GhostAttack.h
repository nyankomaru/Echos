//BTTask_GhostAttack.h

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_GhostAttack.generated.h"

UCLASS()
class ECHO_API UBTTask_GhostAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
    UBTTask_GhostAttack();

    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory) override;

private:
    // 攻撃後の待機時間（連続攻撃しないように）
    UPROPERTY(EditAnywhere, Category = "GhostAI")
    float AttackCooldown = 1.f;
};
