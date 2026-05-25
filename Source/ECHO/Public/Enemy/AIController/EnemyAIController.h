#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyAIController.generated.h"

//前方宣言
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UBehaviorTreeComponent; 
class UBlackboardComponent;

UCLASS()
class ECHO_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

protected:
	//ゲーム開発開始時の初期化関数
	virtual void BeginPlay() override;

	//Pawnをコントロールする際の初期化関数
	virtual void OnPossess(APawn* _inPawn) override;

	//ターゲットの知覚が更新されたときに呼び出される関数
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* _actor, FAIStimulus _stimulus);

public:
	//ターゲットを設定する関数
	virtual void SetTargetActor(AActor* _actor);

	//ターゲットをクリアする関数
	virtual void ClearTarget();

protected:
	//AIの知覚コンポーネントへの参照
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "AI")
	UAIPerceptionComponent* m_pAIPerceptionComp;

	//視覚の設定への参照
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "AI")
	UAISenseConfig_Sight* m_pSightConfig;

	//聴覚の設定への参照
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "AI")
	UAISenseConfig_Hearing* m_pHearingConfig;

	//ビヘイビアツリーコンポーネントへの参照
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "AI")
	UBehaviorTreeComponent* m_pBehaviorTreeComp;

	//ブラックボードコンポーネントへの参照
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "AI")
	UBlackboardComponent* m_pBlackboardComp;
};
