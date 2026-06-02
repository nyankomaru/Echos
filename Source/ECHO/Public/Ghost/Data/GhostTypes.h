// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GhostTypes.generated.h"

//記録対象の行動種別
UENUM(BlueprintType)
enum class EGhostActionType : uint8
{
    Move    UMETA(DisplayName = "Move"),
    Attack  UMETA(DisplayName = "Attack"),
    Dodge   UMETA(DisplayName = "Dodge"),
    Jump    UMETA(DisplayName = "Jump"),
    Land    UMETA(DisplayName = "Land"),
};

// GhostPlaybackComponent の再生モード
UENUM(BlueprintType)
enum class EGhostPlaybackMode : uint8
{
    /** 召喚直後：過去10秒の記録を再生 */
    RecordedPlayback UMETA(DisplayName = "Recorded Playback"),

    /** 記録再生終了後：プレイヤー行動を2秒遅延でミラー */
    DelayedMirror    UMETA(DisplayName = "Delayed Mirror"),

    /** 敵化後：AIに制御を移譲（再生停止） */
    EnemyMode        UMETA(DisplayName = "Enemy Mode"),
};

// 分身の生存・状態遷移
UENUM(BlueprintType)
enum class EGhostLifecycleState : uint8
{
    Friendly  UMETA(DisplayName = "Friendly"),   // 味方として戦闘支援
    Warning   UMETA(DisplayName = "Warning"),    // 敵化3秒前。演出開始
    Corrupted UMETA(DisplayName = "Corrupted"),  // 敵化完了。AIに切り替え中
    Enemy     UMETA(DisplayName = "Enemy"),      // 完全敵化。プレイヤーを狙う
    Dead      UMETA(DisplayName = "Dead"),       // 撃破済み
};

// 1アクションの記録データ
USTRUCT(BlueprintType)
struct FGhostActionData
{
    GENERATED_BODY()

    /** 行動の種類 */
    UPROPERTY(BlueprintReadWrite)
    EGhostActionType Type = EGhostActionType::Move;

    /** 記録時のワールド座標 */
    UPROPERTY(BlueprintReadWrite)
    FVector Location = FVector::ZeroVector;

    /** 入力方向（正規化済み）。Move/Dodge時に使用 */
    UPROPERTY(BlueprintReadWrite)
    FVector Direction = FVector::ZeroVector;

    /** 記録時のワールド回転 */
    UPROPERTY(BlueprintReadWrite)
    FRotator Rotation = FRotator::ZeroRotator;

    /** 記録時刻（GetWorld()->GetTimeSeconds()） */
    UPROPERTY(BlueprintReadWrite)
    float Timestamp = 0.f;

    /**
     * アニメーション識別タグ（将来拡張用）
     * Attack種別（通常/コンボ2段目等）を区別する際に使用予定
     */
    UPROPERTY(BlueprintReadWrite)
    FName AnimationTag = NAME_None;
};