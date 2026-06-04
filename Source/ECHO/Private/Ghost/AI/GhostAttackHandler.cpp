//GhostAttackHandler.cpp
//ゴーストアタックハンドラーソース

#include "Ghost/AI/GhostAttackHandler.h"
#include "GameFramework/Character.h"
#include "Enemy//EnemyChara.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

//アクション実行関数
void UGhostAttackHandler::Execute(const FGhostActionData& Data, ACharacter* Owner)
{
	//オーナーが有効かチェック
    if (!Owner) return;

	//位置と向きをスナップショットに合わせてテレポート
    Owner->SetActorRotation(Data.Rotation);

	//モンタージュ再生
    if (AttackMontage)
    {
        Owner->PlayAnimMontage(AttackMontage);
    }

    //ヒット判定を追加
    FVector Start = Owner->GetActorLocation();
    FVector End = Start + Owner->GetActorForwardVector() * AttackRange;

	//当たったアクターを格納する配列
    TArray<FHitResult> HitResults;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(AttackRadius);

    bool bHit = Owner->GetWorld()->SweepMultiByChannel(HitResults, Start, End, FQuat::Identity, ECC_Pawn, Sphere);

    if (!bHit) return;

    for (const FHitResult& Hit : HitResults)
    {
		//当たったアクターを取得
        AActor* HitActor = Hit.GetActor();
        if (!HitActor || HitActor == Owner) continue;

        //敵にのみダメージを与える
 /*       if (Cast<AEnemyChara>(HitActor))
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
        }*/

        UGameplayStatics::ApplyDamage(HitActor, AttackDamage, nullptr, Owner, UDamageType::StaticClass());

        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, FString::Printf(TEXT("GhostAttackHandler: %s にダメージ %.1f"), *HitActor->GetName(), AttackDamage));
    }
}

