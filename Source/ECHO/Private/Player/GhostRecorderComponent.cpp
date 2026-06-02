
#include "Player/GhostRecorderComponent.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Ghost/Data/GhostTypes.h"
#include "Player/CombatComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UGhostRecorderComponent::UGhostRecorderComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UGhostRecorderComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedCharacter = Cast<ACharacter>(GetOwner());
    CachedCombat = GetOwner()->FindComponentByClass<UCombatComponent>();

    if (!CachedCombat)
    {
        // CombatComponent が見つからない場合の処理
        return;
    }
}

void UGhostRecorderComponent::TickComponent(
    float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!CachedCharacter) return;

    UCharacterMovementComponent* CMC = CachedCharacter->GetCharacterMovement();
    if (!CMC) return;

    const FVector Velocity = CMC->Velocity;
    const FVector MoveDir = Velocity.SizeSquared2D() > 1.f
        ? Velocity.GetSafeNormal2D()
        : FVector::ZeroVector;
    const bool bOnGround = CMC->IsMovingOnGround();
    const bool bFalling = CMC->IsFalling();
    const bool bNowAttacking = CachedCombat ? CachedCombat->IsAttacking() : false;
    const bool bNowDodging = CachedCombat ? CachedCombat->IsDodging() : false;

    // ------------------------------------------------------------------
    // Move：入力方向があるときだけ間引き記録
    // ------------------------------------------------------------------
    if (MoveDir.SizeSquared() > 0.01f)
    {
        MoveRecordTimer += DeltaTime;
        if (MoveRecordTimer >= MoveRecordInterval)
        {
            MoveRecordTimer = 0.f;
            PushAction(EGhostActionType::Move, MoveDir);
        }
    }
    else
    {
        MoveRecordTimer = 0.f;
    }

    // ------------------------------------------------------------------
    // Attack：立ち上がりエッジ検出
    // ------------------------------------------------------------------
    if (!bPrevAttacking && bNowAttacking)
    {
        const FVector Forward = CachedCharacter->GetActorForwardVector();
        PushAction(EGhostActionType::Attack, Forward);
    }

    // ------------------------------------------------------------------
    // Dodge：立ち上がりエッジ検出
    // ------------------------------------------------------------------
    if (!bPrevDodging && bNowDodging)
    {
        PushAction(EGhostActionType::Dodge, MoveDir.IsNearlyZero()
            ? CachedCharacter->GetActorForwardVector()
            : MoveDir);
    }

    // ------------------------------------------------------------------
    // Jump：地上→空中 の遷移
    // ------------------------------------------------------------------
    if (!bPrevFalling && bFalling && Velocity.Z > 0.f)
    {
        PushAction(EGhostActionType::Jump, CachedCharacter->GetActorUpVector());
    }

    // ------------------------------------------------------------------
    // Land：空中→地上 の遷移
    // ------------------------------------------------------------------
    if (!bPrevOnGround && bOnGround)
    {
        PushAction(EGhostActionType::Land, FVector::DownVector);
    }

    // 前フレーム状態を保存
    bPrevAttacking = bNowAttacking;
    bPrevDodging = bNowDodging;
    bPrevFalling = bFalling;
    bPrevOnGround = bOnGround;

    // 古いバッファを定期削除（毎フレームだと重いので Trim は 1秒おきに）
    // シンプルに毎フレーム呼んでも TrimBuffer 内の条件で無駄処理は最小
    TrimBuffer();
}

// -----------------------------------------------------------------------
// PushAction
// -----------------------------------------------------------------------
void UGhostRecorderComponent::PushAction(
    EGhostActionType Type, const FVector& Direction, FName AnimTag)
{
    if (!CachedCharacter) return;

    FGhostActionData Data;
    Data.Type = Type;
    Data.Location = CachedCharacter->GetActorLocation();
    Data.Rotation = CachedCharacter->GetActorRotation();
    Data.Direction = Direction;
    Data.Timestamp = GetWorld()->GetTimeSeconds();
    Data.AnimationTag = AnimTag;

    Buffer.Add(Data);
}

// -----------------------------------------------------------------------
// TrimBuffer
// -----------------------------------------------------------------------
void UGhostRecorderComponent::TrimBuffer()
{
    if (Buffer.Num() == 0) return;

    const float Cutoff = GetWorld()->GetTimeSeconds() - BufferDuration;

    // 先頭から古いエントリを削除
    int32 RemoveCount = 0;
    for (const FGhostActionData& Entry : Buffer)
    {
        if (Entry.Timestamp < Cutoff) RemoveCount++;
        else break;
    }

    if (RemoveCount > 0)
    {
        Buffer.RemoveAt(0, RemoveCount, /*bAllowShrinking=*/false);
    }
}

// -----------------------------------------------------------------------
// GetSnapshot
// -----------------------------------------------------------------------
TArray<FGhostActionData> UGhostRecorderComponent::GetSnapshot(float DurationSeconds) const
{
    const float Cutoff = GetWorld()->GetTimeSeconds() - DurationSeconds;

    TArray<FGhostActionData> Result;
    for (const FGhostActionData& Entry : Buffer)
    {
        if (Entry.Timestamp >= Cutoff)
        {
            Result.Add(Entry);
        }
    }
    return Result;
}

// -----------------------------------------------------------------------
// GetActionsSince
// -----------------------------------------------------------------------
TArray<FGhostActionData> UGhostRecorderComponent::GetActionsSince(float SinceTimestamp) const
{
    TArray<FGhostActionData> Result;
    for (const FGhostActionData& Entry : Buffer)
    {
        if (Entry.Timestamp > SinceTimestamp)
        {
            Result.Add(Entry);
        }
    }
    return Result;
}