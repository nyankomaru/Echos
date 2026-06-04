#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GhostAIController.generated.h"

//前方宣言
class AGhostCharacter;

UCLASS()
class ECHO_API AGhostAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	//コンストラクタ
    AGhostAIController();

protected:
	//開始時に呼ばれる関数
    virtual void OnPossess(APawn* InPawn) override;

	//毎フレーム呼ばれる関数
    virtual void Tick(float DeltaTime) override;

    //攻撃を実行する距離
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GhostAI")
    float AttackRange = 150.f;

    //攻撃間隔（秒）。連打防止
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GhostAI")
    float AttackCooldown = 2.f;

private:
    //ターゲット（プレイヤー）を取得する
    APawn* FindPlayerPawn() const;

    //攻撃を試みる
    void TryAttack();

    float SurroundAngleOffset = 0.f;  // 囲み攻撃用オフセット角度

    //敵化後、プレイヤーへの直接ヒット判定
    void PerformEnemyAttackHit(AGhostCharacter* Ghost);

    // 内部状態
    UPROPERTY()
    TObjectPtr<APawn> TargetPlayer;

    float AttackTimer = 0.f;

private:
    //敵化後の攻撃射程距離
    UPROPERTY(EditAnywhere, Category = "Ghost|AI|Attack")
    float EnemyAttackRange = 150.f;

    //敵化後の攻撃判定球半径
    UPROPERTY(EditAnywhere, Category = "Ghost|AI|Attack")
    float EnemyAttackRadius = 60.f;

    //敵化後のプレイヤーへのダメージ量
    UPROPERTY(EditAnywhere, Category = "Ghost|AI|Attack")
    float EnemyAttackDamage = 20.f;

    // 移動制御
    bool bIsMovingToTarget = false;
    FVector LastTargetLocation = FVector::ZeroVector;
};
