#pragma once

#include "CoreMinimal.h"
#include "Enemy/AIController/EnemyAIController.h"
#include "MinionAIController.generated.h"


class AMinionChara;

UCLASS()
class ECHO_API AMinionAIController : public AEnemyAIController
{
	GENERATED_BODY()

public:
	//コンストラクタ
	AMinionAIController();

protected:
	//ゲーム開発開始時の初期化関数
	virtual void BeginPlay() override;

	//Pawnをコントロールする際の初期化関数
	virtual void OnPossess(APawn* _inPawn) override;

	//Pawnのコントロールをやめるときの関数
	virtual void OnUnPossess() override;

	//毎フレーム呼び出される関数
	virtual void Tick(float _deltaTime) override;

	//ターゲットの知覚が更新されたときに呼び出される関数
	UFUNCTION()
	void OnMinionPerceptionUpdated(AActor* _actor, FAIStimulus _stimulus);
	
public:
	//ターゲットを設定する関数をオーバーライド
	virtual void SetTargetActor(AActor* _newTarget) override;

	//ターゲットをクリアする関数をオーバーライド
	virtual void ClearTarget() override;

	//死んだときに呼び出される関数
	UFUNCTION(BlueprintCallable, Category = "MinionAI")
	void NotifyDead();

private:
	//攻撃範囲内にいるか判定して、ブラックボードを更新する関数
	void UpdateAttackRangeBlackboard();

	//視野パラメータをスクラッパー用設定
	void SetupminionSight();

private:
	//現在ポーズしているキャラクタへの参照
	TWeakObjectPtr<AMinionChara> m_minionChara;

	//視野距離
	UPROPERTY(EditDefaultsOnly, Category = "MinionAI | Sight")
	float m_sightRadius;

	//視野の失われる距離
	UPROPERTY(EditDefaultsOnly, Category = "MinionAI | Sight")
	float m_loseSightRadius;

	//視野の周辺角度
	UPROPERTY(EditDefaultsOnly, Category = "MinionAI | Sight")
	float m_peripheralVisionAngle;

	//視野の刺激が古くなるまでの時間
	UPROPERTY(EditDefaultsOnly, Category = "MinionAI | Sight")
	float m_sightAge;

	//聴覚範囲
	UPROPERTY(EditDefaultsOnly, Category = "MinionAI | Hearing")
	float m_hearingRange;

	//攻撃範囲チェックの更新間隔
	UPROPERTY(EditDefaultsOnly, Category = "MinionAI | Attack")
	float m_attackRangeCheckInterval;

	//攻撃範囲内にいるか判定するためのタイマー
	float m_attackRangeTimer;
};
