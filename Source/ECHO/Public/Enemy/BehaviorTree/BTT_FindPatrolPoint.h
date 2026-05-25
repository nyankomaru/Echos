// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_FindPatrolPoint.generated.h"


UCLASS()
class ECHO_API UBTT_FindPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	//コンストラクタ
	UBTT_FindPatrolPoint();
	
	//タスク実行
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory) override;

	//タスクの説明
	virtual FString GetStaticDescription() const override;

public:
	//パトロールポイントを見つける半径
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = "100.0", ClampMax = "5000.0"))
	float m_patrolRadius;

	//パトロールポイントを保存するブラックボードキー
	UPROPERTY(EditAnywhere, Category = "Patrol")
	FBlackboardKeySelector m_patrolLocation;

	//前回のポイントからの最小距離
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = "0.0"))
	float m_minDistanceFromLastPoint;
};
