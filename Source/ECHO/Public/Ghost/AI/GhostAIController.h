#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GhostAIController.generated.h"


UCLASS()
class ECHO_API AGhostAIController : public AAIController
{
	GENERATED_BODY()
	
public:
    AGhostAIController();

    virtual void OnPossess(APawn* InPawn) override;
    virtual void Tick(float DeltaTime) override;

    // -----------------------------------------------------------------------
    // 設定
    // -----------------------------------------------------------------------

    /** 攻撃を実行する距離（この範囲内に入ったら攻撃） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GhostAI")
    float AttackRange = 150.f;

    /** 攻撃間隔（秒）。連打防止 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GhostAI")
    float AttackCooldown = 1.2f;

private:
    /** ターゲット（プレイヤー）を取得する */
    APawn* FindPlayerPawn() const;

    /** 攻撃を試みる */
    void TryAttack();

    // -----------------------------------------------------------------------
    // 内部状態
    // -----------------------------------------------------------------------
    UPROPERTY()
    TObjectPtr<APawn> TargetPawn;

    float AttackTimer = 0.f;

};
