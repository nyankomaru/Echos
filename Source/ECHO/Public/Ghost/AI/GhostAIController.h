//GhostAIController.h

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

    //Tickで敵との距離を監視してBlackboardを更新
    virtual void Tick(float DeltaTime) override;

private:
    //攻撃範囲
    UPROPERTY(EditAnywhere, Category = "GhostAI")
    float AttackRange = 150.f;

    //最寄りの敵を探す
    AActor* FindNearestEnemy() const;
};