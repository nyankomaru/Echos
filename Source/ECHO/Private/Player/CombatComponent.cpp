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

// -----------------------------------------------------------------------
// ExecuteAttack
// bIsAttaking を true にして攻撃開始。
// モンタージュ再生時間後に ResetAttack() でフラグをリセット。
// -----------------------------------------------------------------------
void UCombatComponent::ExecuteAttack()
{
    if (bIsAttacking) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    HitActorsThisAttack.Empty();
    bIsAttacking = true;

    if (AttackMontage)
    {
        // モンタージュを1回だけ再生し、正確な再生時間を取得
        const float MontageDuration = OwnerCharacter->PlayAnimMontage(AttackMontage);
        const float ResetDelay = (MontageDuration > 0.f) ? MontageDuration : 0.8f;

        // 再生終了後に攻撃フラグをリセットするタイマーを設定
        GetWorld()->GetTimerManager().SetTimer(
            AttackResetTimerHandle,
            this,
            &UCombatComponent::ResetAttack,
            ResetDelay,
            false
        );
    }
    else
    {
        bIsAttacking = false;
    }

    GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Attack!"));
}

// -----------------------------------------------------------------------
// ResetAttack
// モンタージュ終了後に呼ばれ、攻撃フラグを解除する
// -----------------------------------------------------------------------
void UCombatComponent::ResetAttack()
{
    bIsAttacking = false;
    HitActorsThisAttack.Empty();
}

// -----------------------------------------------------------------------
// ExecuteDodge
// -----------------------------------------------------------------------
void UCombatComponent::ExecuteDodge()
{
    if (!bCanDodge) return;

    bIsDodging = true;

    if (bIsAttacking) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
    if (!MoveComp) return;

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
    bIsDodging = false;
}

void UCombatComponent::ResetDodgeCooldown()
{
    bCanDodge = true;
}

void UCombatComponent::ResetAirDodge()
{
    bHasAirDodged = false;
}

// -----------------------------------------------------------------------
// CheckHit
// -----------------------------------------------------------------------
void UCombatComponent::CheckHit()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    FVector Start = OwnerCharacter->GetActorLocation();
    FVector End = Start + OwnerCharacter->GetActorForwardVector() * AttackRange;

    TArray<FHitResult> HitResults;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(AttackRadius);

    DrawDebugSphere(GetWorld(), Start, AttackRadius, 12, FColor::Yellow, false, 1.f);
    DrawDebugSphere(GetWorld(), End, AttackRadius, 12, FColor::Red, false, 1.f);
    DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.f);

    bool bHit = GetWorld()->SweepMultiByChannel(
        HitResults, Start, End, FQuat::Identity, ECC_Pawn, Sphere);

    if (!bHit) return;

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor || HitActor == OwnerCharacter) continue;
        if (HitActorsThisAttack.Contains(HitActor)) continue;

        HitActorsThisAttack.Add(HitActor);

        UGameplayStatics::ApplyDamage(
            HitActor, AttackDamage,
            OwnerCharacter->GetController(),
            OwnerCharacter,
            UDamageType::StaticClass());

        if (Cast<AEnemyChara>(HitActor))
        {
            OnHitEnemy.Broadcast(EnergyGainPerHit);
        }
    }
}