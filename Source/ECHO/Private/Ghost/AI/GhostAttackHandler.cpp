//GhostAttackHandler.cpp
//ゴーストアタックハンドラーソース

#include "Ghost/AI/GhostAttackHandler.h"
#include "GameFramework/Character.h"
#include "Enemy//EnemyChara.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

void UGhostAttackHandler::Execute(const FGhostActionData& Data, ACharacter* Owner)
{
    if (!Owner) return;

    Owner->SetActorRotation(Data.Rotation);

    if (AttackMontage)
    {
        Owner->PlayAnimMontage(AttackMontage);
    }

    //ヒット判定を追加
    FVector Start = Owner->GetActorLocation();
    FVector End = Start + Owner->GetActorForwardVector() * AttackRange;

    TArray<FHitResult> HitResults;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(AttackRadius);

    bool bHit = Owner->GetWorld()->SweepMultiByChannel(
        HitResults,
        Start,
        End,
        FQuat::Identity,
        ECC_Pawn,
        Sphere
    );

    if (!bHit) return;

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor || HitActor == Owner) continue;

        //敵にのみダメージを与える
        if (Cast<AEnemyChara>(HitActor))
        {
            UGameplayStatics::ApplyDamage(
                HitActor,
                AttackDamage,
                nullptr,
                Owner,
                UDamageType::StaticClass()
            );

            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
                TEXT("Ghost: 敵にダメージを与えた！"));
        }
    }
}

