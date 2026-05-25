#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_EnemyAttack.generated.h"


UCLASS()
class ECHO_API UBTT_EnemyAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_EnemyAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory) override;
	

	virtual FString GetStaticDescription() const override;
};
