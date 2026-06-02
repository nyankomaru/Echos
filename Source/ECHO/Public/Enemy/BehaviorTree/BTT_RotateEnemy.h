#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_RotateEnemy.generated.h"


UCLASS()
class ECHO_API UBTT_RotateEnemy : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	//コンストラクタ
	UBTT_RotateEnemy();

	//実行時の処理
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory) override;

	//タスクの説明
	virtual FString GetStaticDescription() const override;

public:
	//最大角度
	UPROPERTY(EditAnywhere, Category = "Rotate",meta = (ClampMin = "5.0", ClampMax = "90.0"))
	float m_allowedAngleDegrees = 45.f;

	//回転速度
	UPROPERTY(EditAnywhere, Category = "Rotate",meta = (ClampMin = "0.0"))
	float m_rotationSpeed = 360.f;
};
