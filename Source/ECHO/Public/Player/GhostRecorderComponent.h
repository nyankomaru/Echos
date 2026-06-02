#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Ghost/Data/GhostTypes.h"
#include "GhostRecorderComponent.generated.h"

class UCombatComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ECHO_API UGhostRecorderComponent : public UActorComponent
{
	GENERATED_BODY()

   public:
    UGhostRecorderComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;


    // データ取得 API
    /**
     * 直近 DurationSeconds 秒分のアクションをコピーして返す
     * 召喚時に RecordedPlayback 用スナップショットを取得するために使用
     */
    TArray<FGhostActionData> GetSnapshot(float DurationSeconds = 10.f) const;

    /**
     * SinceTimestamp より新しいアクションをすべて返す
     * DelayedMirror モードで PlaybackComponent が 2秒遅れで呼び出す
     */
    TArray<FGhostActionData> GetActionsSince(float SinceTimestamp) const;


    // 設定


    /** バッファ保持時間。この時間を超えたエントリは削除される（秒） */
    UPROPERTY(EditAnywhere, Category = "Ghost|Recorder",
        meta = (ClampMin = "5.0", ClampMax = "60.0"))
    float BufferDuration = 15.f;

    /** Move 記録間隔（秒） */
    UPROPERTY(EditAnywhere, Category = "Ghost|Recorder",
        meta = (ClampMin = "0.05", ClampMax = "0.5"))
    float MoveRecordInterval = 0.1f;

private:

    // リングバッファ
    UPROPERTY()
    TArray<FGhostActionData> Buffer;

    /** 古いエントリを BufferDuration に基づいて削除 */
    void TrimBuffer();

    /** アクションをバッファに追加する統一エントリポイント */
    void PushAction(EGhostActionType Type,
        const FVector& Direction,
        FName AnimTag = NAME_None);

    //キャッシュ
    UPROPERTY()
    TObjectPtr<ACharacter> CachedCharacter;

    UPROPERTY()
    TObjectPtr<UCombatComponent> CachedCombat;


    // エッジ検出用前フレーム状態
    bool bPrevAttacking = false;
    bool bPrevDodging = false;
    bool bPrevFalling = false;    // ジャンプ開始検出
    bool bPrevOnGround = true;     // 着地検出


    // Move 記録用タイマー
    float MoveRecordTimer = 0.f;
};
