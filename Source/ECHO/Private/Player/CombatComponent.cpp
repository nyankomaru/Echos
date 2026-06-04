//修正済み
//記録システムを追加

#include "Player/CombatComponent.h"
#include "Player/ActionMovementComponent.h"
#include "Enemy/EnemyChara.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

//コンストラクタ
UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

//コンボの段階を実行する関数
void UCombatComponent::ExecuteComboStep(int32 StepIndex)
{
	//指定された段数が有効かチェック
    if (!ComboSteps.IsValidIndex(StepIndex)) return;

	//オーナーキャラクターの取得
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

	//段階データの取得
    const FComboStepData& Step = ComboSteps[StepIndex];

    //ステートの更新
    CurrentComboIndex = StepIndex;
    bIsAttacking = true;
    bComboWindowOpen = false;
    bComboInputBuffered = false;
    HitActorsThisAttack.Empty();

    // 追加：このコンボ段が始まったことを通知
    OnComboStepStarted.Broadcast(StepIndex, Step);

    //以前のコンボリセットタイマーをクリア、再セット
    GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
    GetWorld()->GetTimerManager().SetTimer( ComboResetTimerHandle, this, &UCombatComponent::OnComboResetTimeout, Step.ComboResetTime, false);

	//アニメーションの再生
    if (Step.Montage) 
    {
        OwnerCharacter->PlayAnimMontage(Step.Montage);
    }
}

//コンボ受付開始関数
void UCombatComponent::OpenComboWindow()
{
    bComboWindowOpen = true;

    //ボタン連打されたら次のコンボを即座に発動させる
    if (bComboInputBuffered)
    {
        bComboInputBuffered = false;
        int32 NextIndex = CurrentComboIndex + 1;
        if (NextIndex < ComboSteps.Num())
            ExecuteComboStep(NextIndex);
    }
}

//コンボ受付終了関数
void UCombatComponent::CloseComboWindow()
{
    bComboWindowOpen = false;
}

//コンボリセットタイムアウト関数
void UCombatComponent::OnComboResetTimeout()
{
    //猶予時間に入力がなければコンボステートを初期化
    CurrentComboIndex = 0;
    bIsAttacking = false;
    bComboWindowOpen = false;
    bComboInputBuffered = false;
    HitActorsThisAttack.Empty();
    GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
}

//攻撃実行関数
void UCombatComponent::ExecuteAttack()
{
    // すでに攻撃中の場合
    if (bIsAttacking)
    {
        // コンボ受付窓口が開いているなら、次のステップへ
        if (bComboWindowOpen)
        {
            int32 NextIndex = CurrentComboIndex + 1;
            if (ComboSteps.IsValidIndex(NextIndex))
            {
                ExecuteComboStep(NextIndex);
            }
        }
        else
        {
            // 窓口が開いていない場合は先行入力(バッファ)として記録
            bComboInputBuffered = true;
        }
        // 攻撃中は何もしないか次のコンボへ移行するため、ここで終了
        return;
    }

    // 攻撃中でない場合は、コンボの最初(0番目)から開始
    ExecuteComboStep(0);

    GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Attack Start!"));
}

//攻撃フラグリセット関数
void UCombatComponent::ResetAttack()
{
    bIsAttacking = false;
    HitActorsThisAttack.Empty();
}

//回避実行関数
void UCombatComponent::ExecuteDodge()
{
	//回避可能か、すでに回避中でないかをチェック
    if (!bCanDodge || bIsDodging) return;

	//攻撃中は回避できない
    if (bIsAttacking) return;

	//オーナーキャラクターと移動コンポーネントの取得
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

	//移動コンポーネントの取得
    UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
    if (!MoveComp) return;

	//回避状態開始
    bIsDodging = true;

    //入力方向を回避方向とする。入力がない場合は正面方向
    FVector DodgeDirection = MoveComp->GetLastInputVector();
    if (DodgeDirection.IsNearlyZero())
    {
        DodgeDirection = OwnerCharacter->GetActorForwardVector();
    }

    //空中での回避制限チェック（一回まで）
    if (MoveComp->IsFalling())
    {
        if (bHasAirDodged) return;
        bHasAirDodged = true;
    }

    //回避クールダウンタイマーの開始
    bCanDodge = false;
    GetWorld()->GetTimerManager().SetTimer(DodgeCoolDownTimerHandle, this, &UCombatComponent::ResetDodgeCooldown, DodgeCooldown, false);

    //完全に水平方向へ滑らせる
    DodgeDirection.Z = 0.f;

    //元の重力と摩擦を0にする
    CachedGravityScale = MoveComp->GravityScale;
    CachedGroundFriction = MoveComp->GroundFriction;

	//重力と摩擦を0にして、回避中は完全に水平に移動させる
    MoveComp->GravityScale = 0.f;
    MoveComp->GroundFriction = 0.f;

	//現在の速度をリセット
    MoveComp->Velocity = FVector::ZeroVector;

    GetWorld()->GetTimerManager().SetTimer(DodgeTimerHandle, this, &UCombatComponent::EndDodge, DodgeDuration, false);

    //キャラクターを強引に押し出す
    OwnerCharacter->LaunchCharacter(DodgeDirection.GetSafeNormal() * DodgeForce, true, true);
}

//回避終了関数
void UCombatComponent::EndDodge()
{
	//オーナーキャラクターと移動コンポーネントの取得
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
    {
		//重力と摩擦を元に戻す
        UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
        MoveComp->GravityScale = CachedGravityScale;
        MoveComp->GroundFriction = CachedGroundFriction;

		//回避後の速度を完全に水平にする
        FVector Vel = MoveComp->Velocity;
        Vel.X = 0.f;
        Vel.Y = 0.f;
        MoveComp->Velocity = Vel;
    }
	//回避状態終了
    bIsDodging = false;
}

//回避クールダウンリセット関数
void UCombatComponent::ResetDodgeCooldown()
{
    bCanDodge = true;
}

//空中回避リセット関数
void UCombatComponent::ResetAirDodge()
{
    bHasAirDodged = false;
}

//攻撃ヒット判定関数
void UCombatComponent::CheckHit()
{
	//現在のコンボ段階のデータが有効かチェック
    if (!ComboSteps.IsValidIndex(CurrentComboIndex)) return;

	//オーナーキャラクターの取得
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

	//現在の段階のデータを取得
    const FComboStepData& Step = ComboSteps[CurrentComboIndex];

	//攻撃判定の開始点と終了点を計算
    FVector Start = OwnerCharacter->GetActorLocation();
    FVector End = Start + OwnerCharacter->GetActorForwardVector() * Step.HitRange;

	//ヒット結果を格納する配列と、球形のコリジョン形状を作成
    TArray<FHitResult> HitResults;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(Step.HitRadius);

    // デバッグ表示エディタ上のみ
#if WITH_EDITOR
    DrawDebugSphere(GetWorld(), Start, Step.HitRadius, 12, FColor::Yellow, false, 1.f);
    DrawDebugSphere(GetWorld(), End, Step.HitRadius, 12, FColor::Red, false, 1.f);
    DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.f);
#endif WITH_EDITOR
	
    //SweepMultiByChannelを使用して、球形の攻撃判定を行う
    bool bHit = GetWorld()->SweepMultiByChannel(HitResults, Start, End, FQuat::Identity, ECC_Pawn, Sphere);
    if (!bHit) return;

	//ヒットしたアクターに対して処理を行う
    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor || HitActor == OwnerCharacter) continue;
        if (HitActorsThisAttack.Contains(HitActor)) continue;

        HitActorsThisAttack.Add(HitActor);

        // ダメージ適用
        UGameplayStatics::ApplyDamage(
            HitActor, Step.Damage, // Step.Damageを使用
            OwnerCharacter->GetController(),
            OwnerCharacter,
            UDamageType::StaticClass());

        // ノックバック処理
        if (Step.LaunchForce > 0.f)
        {
            if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
            {
                FVector LaunchDir = (HitActor->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
                LaunchDir.Z = 0.2f;
                HitCharacter->LaunchCharacter(LaunchDir * Step.LaunchForce, true, true);
            }
        }

        // 敵へのヒット時のエネルギー加算
        if (Cast<AEnemyChara>(HitActor))
        {
            // EnergyGainPerHitまたはStepDataに基づいた値を送る
            OnHitEnemy.Broadcast(Step.Damage * 0.5f);
        }
    }
}
