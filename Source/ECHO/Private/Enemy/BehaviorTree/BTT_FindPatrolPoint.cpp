#include "Enemy/BehaviorTree/BTT_FindPatrolPoint.h"

#include "AIController.h"
#include "NavigationSystem.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"


UBTT_FindPatrolPoint::UBTT_FindPatrolPoint() : m_patrolRadius(1000.0f), m_minDistanceFromLastPoint(200.0f)
{
	//タスクの名前を設定
	NodeName = TEXT("Find Patrol Point");
	
	//ブラックボードキーのフィルタを設定
	m_patrolLocation.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTT_FindPatrolPoint, m_patrolLocation));
}

//タスクの実行
EBTNodeResult::Type UBTT_FindPatrolPoint::ExecuteTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory)
{
	//ブラックボードにパトロールポイントを取得
	UBlackboardComponent* blackboardComp = _ownerComp.GetBlackboardComponent();

	//AIコントローラーを取得
	AAIController* aiController = _ownerComp.GetAIOwner();
	
	//AIコントローラーからポーンを取得
	APawn* pawn = aiController ? aiController->GetPawn() : nullptr;

	//ブラックボードとポーンが有効か確認
	if (!blackboardComp || !pawn)
	{
		return EBTNodeResult::Failed;
	}
	
	//ナビゲーションシステムを取得
	UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(GetWorld());

	if (!navSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("Navigation system が見つかりません。ナヴィゲーションメッシュをレベルにあるか"));
		return EBTNodeResult::Failed;
	}

	//ランダムなポイントを生成
	FNavLocation navLocation;

	//現在の位置を取得
	FVector origin = pawn->GetActorLocation();

	//前回のポイントを取得
	FVector lastPatrolPoint = blackboardComp->GetValueAsVector(m_patrolLocation.SelectedKeyName);
	
	//ポイントが見つかるまで繰り返す
	const int maxAttempts = 10;
	bool bFoundPoint = false;

	for (int i = 0; i < maxAttempts; ++i)
	{
		//ランダムなポイントを生成
		if (!navSystem->GetRandomReachablePointInRadius(origin, m_patrolRadius, navLocation))
		{
			//ポイントが見つからなかった場合は次の試行へ
			continue;
		}

		//ポイントが見つかった場合、前回のポイントからの距離を確認
		if (m_minDistanceFromLastPoint > 0.0f)
		{
			//前回のポイントからの距離を計算
			float distanceFromLastPoint = FVector::Dist(navLocation.Location, lastPatrolPoint);
			
			//前回のポイントから十分に離れているか確認
			if (distanceFromLastPoint < m_minDistanceFromLastPoint)
			{
				//十分に離れていない場合は次の試行へ
				continue;
			}
		}
		//ブラックボードに新しいパトロールポイントを保存
		bFoundPoint = true;
		break;
	}

	if (!bFoundPoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("有効なパトロールポイントが見つかりませんでした。半径を広げるか、ナヴィゲーションメッシュを確認してください"));
		return EBTNodeResult::Failed;
	}

	blackboardComp->SetValueAsVector(m_patrolLocation.SelectedKeyName, navLocation.Location);
	UE_LOG(LogTemp, Verbose, TEXT("新しいパトロールポイントを見つけました: %s"), *navLocation.Location.ToString());

	return EBTNodeResult::Succeeded;
}

//タスクの説明
FString UBTT_FindPatrolPoint::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: 半径=%.1f, ランダム地点=%.1f, キー=%s"), *NodeName, m_patrolRadius, m_minDistanceFromLastPoint, *m_patrolLocation.SelectedKeyName.ToString());
}