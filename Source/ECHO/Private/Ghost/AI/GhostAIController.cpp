//GhostAIController.cpp

#include "Ghost/AI/GhostAIController.h"
#include "Enemy/EnemyChara.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

AGhostAIController::AGhostAIController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGhostAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
        TEXT("GhostAIController: OnPossess呼ばれた"));
}

void AGhostAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UBlackboardComponent* BB = GetBlackboardComponent();
    if (!BB) return;

    APawn* OwnerPawn = GetPawn();
    if (!OwnerPawn) return;

    // 最寄りの敵を探してBlackboardに書き込む
    AActor* NearestEnemy = FindNearestEnemy();
    BB->SetValueAsObject(FName("TargetEnemy"), NearestEnemy);

    if (NearestEnemy)
    {
        float Distance = FVector::Dist(
            OwnerPawn->GetActorLocation(),
            NearestEnemy->GetActorLocation()
        );
        bool bInRange = Distance <= AttackRange;
        BB->SetValueAsBool(FName("IsInAttackRange"), bInRange);

        GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::White,
            FString::Printf(TEXT("距離: %.1f 攻撃範囲: %.1f InRange: %s"),
                Distance, AttackRange, bInRange ? TEXT("TRUE") : TEXT("FALSE")));
    }
    else
    {
        BB->SetValueAsBool(FName("IsInAttackRange"), false);
    }


}

AActor* AGhostAIController::FindNearestEnemy() const
{
    APawn* OwnerPawn = GetPawn();
    if (!OwnerPawn) return nullptr;

    TArray<AActor*> Enemies;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        AEnemyChara::StaticClass(),
        Enemies
    );

    AActor* Nearest = nullptr;
    float MinDist = FLT_MAX;

    for (AActor* Enemy : Enemies)
    {
        float Dist = FVector::Dist(
            OwnerPawn->GetActorLocation(),
            Enemy->GetActorLocation()
        );
        if (Dist < MinDist)
        {
            MinDist = Dist;
            Nearest = Enemy;
        }
    }

    return Nearest;
}
