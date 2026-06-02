// CombatComponent.cpp

#include "Player/CombatComponent.h"
#include "Player/ActionMovementComponent.h"
#include "Enemy/EnemyChara.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::ExecuteAttack()
{
    if (bIsAttacking)
    {
        //コンボ受付が開いていたら次の段数へ移行
        if (bComboWindowOpen)
        {
            int32 NextIndex = CurrentComboIndex + 1;
            if (NextIndex < ComboSteps.Num())
                ExecuteComboStep(NextIndex);
        }
        else
        {
            //受付窓口が開く前にボタンが押されたら先行入力をONにする
            bComboInputBuffered = true;
        }
        return;
    }

    ExecuteComboStep(0);
}

void UCombatComponent::ExecuteComboStep(int32 StepIndex)
{
    if (!ComboSteps.IsValidIndex(StepIndex)) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    const FComboStepData& Step = ComboSteps[StepIndex];

    //ステートの更新
    CurrentComboIndex = StepIndex;
    bIsAttacking = true;
    bComboWindowOpen = false;
    bComboInputBuffered = false;
    HitActorsThisAttack.Empty();

    //以前のコンボリセットタイマーをクリア、再セット
    GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(
        ComboResetTimerHandle,
        this,
        &UCombatComponent::OnComboResetTimeout,
        Step.ComboResetTime,
        false
    );

    if (Step.Montage) {
        OwnerCharacter->PlayAnimMontage(Step.Montage);
    }
}

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

void UCombatComponent::CloseComboWindow()
{
    bComboWindowOpen = false;
}

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

void UCombatComponent::CheckHit()
{
    if (!ComboSteps.IsValidIndex(CurrentComboIndex)) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    const FComboStepData& Step = ComboSteps[CurrentComboIndex];

    //自身の位置からキャラクターの前方へリーチの分だけ伸ばした線分を作成
    FVector Start = OwnerCharacter->GetActorLocation();
    FVector End = Start + OwnerCharacter->GetActorForwardVector() * Step.HitRange;

    TArray<FHitResult> HitResults;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(Step.HitRadius);

    //デバッグ用
    DrawDebugSphere(GetWorld(), Start, Step.HitRadius, 12, FColor::Yellow, false, 1.f);
    DrawDebugSphere(GetWorld(), End, Step.HitRadius, 12, FColor::Red, false, 1.f);
    DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.f);

    //当たり判定用の球体を移動させる
    bool bHit = GetWorld()->SweepMultiByChannel(
        HitResults, Start, End,
        FQuat::Identity, ECC_Pawn, Sphere);

    if (!bHit) return;

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();

        //自身を無視、他アクタに重複して当たらないようにする
        if (!HitActor || HitActor == OwnerCharacter) continue;
        if (HitActorsThisAttack.Contains(HitActor)) continue;
        HitActorsThisAttack.Add(HitActor);

        //UE標準の汎用ダメージシステムを適応
        UGameplayStatics::ApplyDamage(
            HitActor, Step.Damage,
            OwnerCharacter->GetController(),
            OwnerCharacter,
            UDamageType::StaticClass());

        //吹き飛ばし力が設定されている場合ノックバック処理
        if (Step.LaunchForce > 0.f)
        {
            if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
            {
                //自分から敵への方向ベクトルわ算出
                FVector LaunchDir = (HitActor->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
                LaunchDir.Z = 0.2f; //少し斜め上に打ち上げる
                //敵の移動速度をリセット
                HitCharacter->LaunchCharacter(LaunchDir * Step.LaunchForce, true, true);
            }
        }

        //ヒット対象が敵であった場合デリゲートを介してキャラクター側にエネルギーを送る
        if (Cast<AEnemyChara>(HitActor))
            OnHitEnemy.Broadcast(Step.Damage * 0.5f);
    }
}

//// -----------------------------------------------------------------------
//// ExecuteAttack
//// bIsAttaking を true にして攻撃開始。
//// モンタージュ再生時間後に ResetAttack() でフラグをリセット。
//// -----------------------------------------------------------------------
//void UCombatComponent::ExecuteAttack()
//{
//    if (bIsAttacking) return;
//
//    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
//    if (!OwnerCharacter) return;
//
//    HitActorsThisAttack.Empty();
//    bIsAttacking = true;
//
//    if (AttackMontage)
//    {
//        // モンタージュを1回だけ再生し、正確な再生時間を取得
//        const float MontageDuration = OwnerCharacter->PlayAnimMontage(AttackMontage);
//        const float ResetDelay = (MontageDuration > 0.f) ? MontageDuration : 0.8f;
//
//        // 再生終了後に攻撃フラグをリセットするタイマーを設定
//        GetWorld()->GetTimerManager().SetTimer(
//            AttackResetTimerHandle,
//            this,
//            &UCombatComponent::ResetAttack,
//            ResetDelay,
//            false
//        );
//    }
//    else
//    {
//        bIsAttacking = false;
//    }
//
//    GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Attack!"));
//}

//ラヤマジの追加分一旦コメントアウト
//// -----------------------------------------------------------------------
//// ResetAttack
//// モンタージュ終了後に呼ばれ、攻撃フラグを解除する
//// -----------------------------------------------------------------------
//void UCombatComponent::ResetAttack()
//{
//    bIsAttacking = false;
//    HitActorsThisAttack.Empty();
//}

// -----------------------------------------------------------------------
// ExecuteDodge
// -----------------------------------------------------------------------
void UCombatComponent::ExecuteDodge()
{
    if (!bCanDodge || m_bIsDodging) return;

    if (bIsAttacking) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
    if (!MoveComp) return;

    m_bIsDodging = true;

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
    GetWorld()->GetTimerManager().SetTimer(
        DodgeCoolDownTimerHandle, this,
        &UCombatComponent::ResetDodgeCooldown,
        DodgeCooldown, false);

    //完全に水平方向へ滑らせる
    DodgeDirection.Z = 0.f;
    //元の重力と摩擦を0にする
    CachedGravityScale = MoveComp->GravityScale;
    CachedGroundFriction = MoveComp->GroundFriction;

    MoveComp->GravityScale = 0.f;
    MoveComp->GroundFriction = 0.f;

    MoveComp->Velocity = FVector::ZeroVector;

    GetWorld()->GetTimerManager().SetTimer(
        DodgeTimerHandle, this,
        &UCombatComponent::EndDodge,
        DodgeDuration, false);

    //キャラクターを強引に押し出す
    OwnerCharacter->LaunchCharacter(
        DodgeDirection.GetSafeNormal() * DodgeForce, true, true);
}

void UCombatComponent::EndDodge()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
    {
        UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
        MoveComp->GravityScale = CachedGravityScale;
        MoveComp->GroundFriction = CachedGroundFriction;

        FVector Vel = MoveComp->Velocity;
        Vel.X = 0.f;
        Vel.Y = 0.f;
        MoveComp->Velocity = Vel;
    }
    m_bIsDodging = false;
}

void UCombatComponent::ResetDodgeCooldown()
{
    bCanDodge = true;
}

void UCombatComponent::ResetAirDodge()
{
    bHasAirDodged = false;
}

//ラヤマジ追加分一旦コメントアウト
//// -----------------------------------------------------------------------
//// CheckHit
//// -----------------------------------------------------------------------
//void UCombatComponent::CheckHit()
//{
//    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
//    if (!OwnerCharacter) return;
//
//    FVector Start = OwnerCharacter->GetActorLocation();
//    FVector End = Start + OwnerCharacter->GetActorForwardVector() * AttackRange;
//
//    TArray<FHitResult> HitResults;
//    FCollisionShape Sphere = FCollisionShape::MakeSphere(AttackRadius);
//
//    DrawDebugSphere(GetWorld(), Start, AttackRadius, 12, FColor::Yellow, false, 1.f);
//    DrawDebugSphere(GetWorld(), End, AttackRadius, 12, FColor::Red, false, 1.f);
//    DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.f);
//
//    bool bHit = GetWorld()->SweepMultiByChannel(
//        HitResults, Start, End, FQuat::Identity, ECC_Pawn, Sphere);
//
//    if (!bHit) return;
//
//    for (const FHitResult& Hit : HitResults)
//    {
//        AActor* HitActor = Hit.GetActor();
//        if (!HitActor || HitActor == OwnerCharacter) continue;
//        if (HitActorsThisAttack.Contains(HitActor)) continue;
//
//        HitActorsThisAttack.Add(HitActor);
//
//        UGameplayStatics::ApplyDamage(
//            HitActor, AttackDamage,
//            OwnerCharacter->GetController(),
//            OwnerCharacter,
//            UDamageType::StaticClass());
//
//        if (Cast<AEnemyChara>(HitActor))
//        {
//            OnHitEnemy.Broadcast(EnergyGainPerHit);
//        }
//    }
//}