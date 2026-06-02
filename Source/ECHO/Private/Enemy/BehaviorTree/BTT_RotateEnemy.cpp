#include "Enemy/BehaviorTree/BTT_RotateEnemy.h"
#include "Enemy/EnemyChara.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

//コンストラクタ
UBTT_RotateEnemy::UBTT_RotateEnemy()
{
	NodeName = TEXT("Rotate Towards Target");
}

//実行時の処理
EBTNodeResult::Type UBTT_RotateEnemy::ExecuteTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory)
{
	//AIコントローラーと敵キャラクターを取得
	AAIController* aiCtrl = _ownerComp.GetAIOwner();
	if (!aiCtrl) { return EBTNodeResult::Failed; }

	//敵キャラクターが死亡している場合は失敗
	AEnemyChara* enemy = Cast<AEnemyChara>((aiCtrl->GetPawn()));
	if (!enemy) { return EBTNodeResult::Failed; }
	if (enemy->GetCurrentState() == EEnemyState::Dead) { return EBTNodeResult::Failed; }

	//ターゲットを取得
	AActor* target = enemy->GetCurrentTarget();
	if (!target) { return EBTNodeResult::Failed; }

	//ターゲットへの方向ベクトルを計算
	FVector toTarget = (target->GetActorLocation() - enemy->GetActorLocation()).GetSafeNormal();
	toTarget.Z = 0.f;	//水平方向のみ計算

	FVector forward = enemy->GetActorForwardVector();
	forward.Z = 0.f;

	//現在の角度差を計算
	float dot = FVector::DotProduct(forward.GetSafeNormal(), toTarget.GetSafeNormal());
	float angleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(dot, -1.f, 1.f)));

	//許容角度内なら成功
	if (angleDeg <= m_allowedAngleDegrees)
	{
		return EBTNodeResult::Succeeded;
	}

	//許容角度を超えている場合は回転する
	FRotator targetRot = toTarget.Rotation();
	targetRot.Pitch = 0.f;	//水平方向のみ回転
	targetRot.Roll = 0.f;

	//回転速度が0以下の場合は瞬間的に回転する
	if(m_rotationSpeed <= 0.f)
	{
		enemy->SetActorRotation(targetRot);
	}
	else
	{
		//回転速度に応じて徐々に回転する
		float deltaTime = GetWorld()->GetDeltaSeconds();

		//現在の回転と目標の回転を線形補間して新しい回転を計算
		FRotator currentRot = enemy->GetActorRotation();
		FRotator newRot = FMath::RInterpTo(currentRot, targetRot, deltaTime, m_rotationSpeed / 360.f);
		
		//新しい回転を敵キャラクターに適用
		enemy->SetActorRotation(newRot);
	}
	return EBTNodeResult::Succeeded;
}

//タスクの説明
FString UBTT_RotateEnemy::GetStaticDescription() const
{
	return FString::Printf(TEXT("Allowed Angle: %.1f"), m_allowedAngleDegrees);
}
