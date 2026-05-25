//CombatComponent.cpp
//コンバットコンポーネントソース

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

    //残像システムへの行動データの通知
    FGhostActionData Data;
    Data.Type = EGhostActionType::Attack;
    Data.Timestamp = GetWorld()->GetTimeSeconds();
    Data.Location = OwnerCharacter->GetActorLocation();
    Data.Rotation = OwnerCharacter->GetActorRotation();
    //アニメーターが識別しやすいよう「Attack1」「Attack2」といったタグ名を動的に生成
    Data.AnimationTag = FName(*FString::Printf(TEXT("Attack%d"), StepIndex + 1));
    OnGhostActionRecorded.Broadcast(Data);

    if (Step.Montage)
        OwnerCharacter->PlayAnimMontage(Step.Montage);
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

void UCombatComponent::ExecuteDodge()
{
    if (!bCanDodge) return;
    if (bIsAttacking) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
    if (!MoveComp) return;

    //入力方向を回避方向とする。入力がない場合は正面方向
    FVector DodgeDirection = MoveComp->GetLastInputVector();
    if (DodgeDirection.IsNearlyZero())
        DodgeDirection = OwnerCharacter->GetActorForwardVector();

    //残像システムへ回避アクションを通知
    FGhostActionData Data;
    Data.Type = EGhostActionType::Dodge;
    Data.Timestamp = GetWorld()->GetTimeSeconds();
    Data.Location = OwnerCharacter->GetActorLocation();
    Data.Rotation = OwnerCharacter->GetActorRotation();
    Data.Direction = DodgeDirection.GetSafeNormal();
    OnGhostActionRecorded.Broadcast(Data);

    //空中での回避制限チェック（一回まで）
    if (MoveComp->IsFalling())
    {
        if (bHasAirDodged) return;
        bHasAirDodged = true;
    }

    //回避クールダウンタイマーの開始
    bCanDodge = false;
    OwnerCharacter->GetWorldTimerManager().SetTimer(
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

    //回避持続タイマー開始
    OwnerCharacter->GetWorldTimerManager().SetTimer(
        DodgeTimerHandle, this,
        &UCombatComponent::EndDodge,
        DodgeDuration, false);

    //移動コンポーネントに現在回避中のフラグを立て、他の移動処理を排除
    UActionMovementComponent* ActionMoveComp = Cast<UActionMovementComponent>(MoveComp);
    if (ActionMoveComp) ActionMoveComp->bIsDodging = true;

    //キャラクターを強引に押し出す
    OwnerCharacter->LaunchCharacter(DodgeDirection.GetSafeNormal() * DodgeForce, true, true);
}

void UCombatComponent::EndDodge()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
    if (!MoveComp) return;

    UActionMovementComponent* ActionMoveComp =
        Cast<UActionMovementComponent>(MoveComp);
    if (ActionMoveComp) ActionMoveComp->bIsDodging = false;

    //一時的に無効化していたしていた重力と摩擦を元の値に復元
    MoveComp->GravityScale = CachedGravityScale;
    MoveComp->GroundFriction = CachedGroundFriction;

    //回避終了時にXY軸の速度を強制的に０にする、ピタッとキレのある操作感にする
    FVector Vel = MoveComp->Velocity;
    Vel.X = 0.f;
    Vel.Y = 0.f;
    MoveComp->Velocity = Vel;
}

void UCombatComponent::ResetDodgeCooldown()
{
    bCanDodge = true;
}

void UCombatComponent::ResetAirDodge()
{
    bHasAirDodged = false;
}