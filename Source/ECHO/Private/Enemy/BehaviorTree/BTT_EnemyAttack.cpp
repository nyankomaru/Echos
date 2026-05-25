
#include "Enemy/BehaviorTree/BTT_EnemyAttack.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Enemy/EnemyChara.h"
#include "AIController.h"

//コンストラクタ
UBTT_EnemyAttack::UBTT_EnemyAttack()
{
	//BTエディタ上で表示されるノードの名前を設定
	NodeName = TEXT("Enemy Attack");
}

//BTTaskNodeのExecuteTask関数をオーバーライド
EBTNodeResult::Type UBTT_EnemyAttack::ExecuteTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory)
{

	//AIControllerを取得
	AAIController* controller = _ownerComp.GetAIOwner();
	if (!controller) { return EBTNodeResult::Failed; }

	//EnemyCharaを取得
	AEnemyChara* enemy = Cast<AEnemyChara>(controller->GetPawn());
	if (!enemy) { return EBTNodeResult::Failed; }

	//死んでいたら失敗
	if (enemy->GetCurrentState() == EEnemyState::Dead) { return EBTNodeResult::Failed; }

	//Attack関数を呼び出す
	enemy->Attack();

	//成功
	return EBTNodeResult::Succeeded;
}

//BTエディタ上でノードの説明を表示するための関数をオーバーライド
FString UBTT_EnemyAttack::GetStaticDescription() const
{
	return FString::Printf(TEXT("Enemy Attack"));
}
