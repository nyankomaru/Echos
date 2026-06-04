#include "Ghost/AI/GhostAIController.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostCharacter/GhostPlaybackComponent.h"
#include "Ghost/AI/GhostAttackHandler.h"
#include "Player/CombatComponent.h"
#include "Ghost/Data/GhostTypes.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

//コンストラクタ
AGhostAIController::AGhostAIController()
{
    PrimaryActorTick.bCanEverTick = true;
}

//ポゼッション時の処理
void AGhostAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // ポゼッション完了時にプレイヤーをターゲットとして設定
    TargetPlayer = FindPlayerPawn();

    SurroundAngleOffset = FMath::RandRange(0.f, 360.f);
}

//毎フレーム呼ばれる関数
void AGhostAIController::Tick(float DeltaTime)
{
	//親クラスの Tick を呼び出す
    Super::Tick(DeltaTime);

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

    // ターゲットが無効なら再検索
    if (!IsValid(TargetPlayer))
    {
        TargetPlayer = FindPlayerPawn();
        if (!TargetPlayer) return;
    }

	//ターゲットとの距離を計算
    const FVector EnemyLocation = ControlledPawn->GetActorLocation();
    const FVector TargetLocation = TargetPlayer->GetActorLocation();
    const float   DistToTarget = FVector::Dist(EnemyLocation, TargetLocation);

    // 移動：攻撃射程外なら接近
    if (DistToTarget > AttackRange)
    {
        // 前回の目標との距離が一定以上離れた場合だけ再経路探索
        const float TargetMovedDist = FVector::Dist(TargetLocation, LastTargetLocation);

        if (TargetMovedDist > 100.f || !bIsMovingToTarget)
        {
            MoveToActor(TargetPlayer, AttackRange * 0.8f);
            LastTargetLocation = TargetLocation;
            bIsMovingToTarget = true;
        }
    }
    else
    {
        // 射程内に入ったら移動停止
        if (bIsMovingToTarget)
        {
            StopMovement();
            bIsMovingToTarget = false;
        }
    }

    // 攻撃：射程内ならクールダウンを消費して攻撃
    AttackTimer += DeltaTime;
    if (DistToTarget <= AttackRange && AttackTimer >= AttackCooldown)
    {
        TryAttack();
        AttackTimer = 0.f;
    }
}

//ターゲット（プレイヤー）を取得する
APawn* AGhostAIController::FindPlayerPawn() const
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    return PC ? PC->GetPawn() : nullptr;
}

//攻撃を試みる
void AGhostAIController::TryAttack()
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

	//攻撃処理は GhostCharacter の CombatComponent に任せる
    AGhostCharacter* Ghost = Cast<AGhostCharacter>(ControlledPawn);
    if (!Ghost || !Ghost->CombatComponent) return;

    if (!IsValid(TargetPlayer)) return;

    // プレイヤーの方向を向く
    FVector Dir = (TargetPlayer->GetActorLocation() - Ghost->GetActorLocation()).GetSafeNormal();
    Dir.Z = 0.f;
    if (!Dir.IsNearlyZero())
    {
        Ghost->SetActorRotation(Dir.Rotation());
    }

    //攻撃中は新たに起動しない
    if (Ghost->CombatComponent->IsAttacking()) return;

    //モンタージュ再生
    Ghost->CombatComponent->ExecuteAttack();

    // ヒット判定（遅延あり）
    FTimerHandle HitTimer;
    GetWorld()->GetTimerManager().SetTimer(
        HitTimer,
        [this, Ghost]()
        {
            if (IsValid(this) && IsValid(Ghost) && IsValid(TargetPlayer))
            {
                PerformEnemyAttackHit(Ghost);
            }
        }, 0.15f, false);
}

void AGhostAIController::PerformEnemyAttackHit(AGhostCharacter* Ghost)
{
	// Ghost と TargetPawn が有効でない場合は何もしない
    if (!IsValid(Ghost)) return;
    if (!IsValid(TargetPlayer)) return;

	//攻撃範囲内のアクターを取得
    const FVector Start = Ghost->GetActorLocation();
    const FVector End = Start + Ghost->GetActorForwardVector() * EnemyAttackRange;

	//当たった結果を格納する配列
    TArray<FHitResult> HitResults;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(EnemyAttackRadius);

	//当たったかフラグ
    bool bHit = GetWorld()->SweepMultiByChannel(HitResults, Start, End, FQuat::Identity, ECC_Pawn, Sphere);
    if (!bHit) return;

	//ヒットしたアクターに対して処理を行う
    for (const FHitResult& Hit : HitResults)
    {
		// ヒットしたアクターを取得
        AActor* HitActor = Hit.GetActor();
        if (!IsValid(HitActor)) continue;

		//プレイヤー以外は無視
        if (HitActor == Ghost) continue;
        if (HitActor != TargetPlayer) continue; // プレイヤーのみ

		// ダメージ適用
        UGameplayStatics::ApplyDamage(HitActor, EnemyAttackDamage, this, Ghost,  UDamageType::StaticClass());

		// デバッグ表示
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, FString::Printf(TEXT("[Ghost敵] プレイヤーにダメージ %.1f ！"), EnemyAttackDamage));
    }
}
