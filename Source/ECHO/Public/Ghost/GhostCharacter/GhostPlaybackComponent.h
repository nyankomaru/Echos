// GhostPlaybackComponent.h
// 記録データの再生と DelayedMirror を管理するコンポーネント


#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Ghost/Data/GhostTypes.h"
#include "GhostPlaybackComponent.generated.h"

class UGhostRecorderComponent;
class AGhostCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRecordedPlaybackFinished);

UCLASS(ClassGroup = (Ghost), meta = (BlueprintSpawnableComponent))
class ECHO_API UGhostPlaybackComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    //コンストラクタ
    UGhostPlaybackComponent();

    // 初期化 / モード切替
    void Initialize(const TArray<FGhostActionData>& Snapshot, UGhostRecorderComponent* Recorder);

    //敵モードへ切り替える関数
    void SetEnemyMode();

public:
    // AnimBP 用の状態取得
    UFUNCTION(BlueprintPure, Category = "Ghost|Playback")
    float GetSpeed() const { return AnimSpeed; }

    //攻撃のアニメ中かをどうかを取得する関数
    UFUNCTION(BlueprintPure, Category = "Ghost|Playback")
    bool GetIsAttacking() const { return bAnimAttacking; }

    UFUNCTION(BlueprintPure, Category = "Ghost|Playback")
    bool GetIsDodging() const { return bAnimDodging; }

    UFUNCTION(BlueprintPure, Category = "Ghost|Playback")
    EGhostPlaybackMode GetPlaybackMode() const { return CurrentMode; }

    // イベント
    UPROPERTY(BlueprintAssignable, Category = "Ghost|Playback")
    FOnRecordedPlaybackFinished OnRecordedPlaybackFinished;

    //DelayedMirror の遅延秒数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Playback",
        meta = (ClampMin = "0.5", ClampMax = "5.0"))
    float MirrorDelay = 2.f;

    // Move レコードが来なくなってから何秒後に停止するか
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Playback",
        meta = (ClampMin = "0.05", ClampMax = "1.0"))
    float MoveExpireTime = 0.2f;

    //回転補間速度
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Playback")
    float RotationInterpSpeed = 15.f;

    //攻撃ヒット判定の射程距離
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Attack")
    float FriendlyAttackRange = 120.f;

    //攻撃ヒット判定の球半径
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Attack")
    float FriendlyAttackRadius = 50.f;

    //味方フェーズの攻撃ダメージ 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Attack")
    float FriendlyAttackDamage = 5.f;

    // ExecuteAttack() からヒット判定を発火するまでの遅延秒数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Attack")
    float AttackHitDelay = 0.15f;

    UPROPERTY(BlueprintReadOnly, Category = "Ghost|Anim")
    float AnimSpeed = 0.f;       // AnimBPから直接 Get できる

    UPROPERTY(BlueprintReadOnly, Category = "Ghost|Anim")
    bool bAnimAttacking = false;

    UPROPERTY(BlueprintReadOnly, Category = "Ghost|Anim")
    bool bAnimDodging = false;

	//敵フェーズの行動パターン関連パラメータ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Surround")
    float SurroundRadius = 120.f;

	//プレイヤーに近づきすぎないようにするための最小距離
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Surround")
    float AttackStopRange = 140.f;

	//プレイヤーから離れすぎないようにするための最大距離
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Surround")
    float MinDistFromPlayer = 100.f;

	//敵がプレイヤーを見失ったと判断する距離
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Surround")
    float EnemySearchRadius = 800.f;

protected:
    //毎フレーム呼ばれる関数
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    //アクション実行関数
    void ExecuteAction(const FGhostActionData& Action);

    //移動実行関数
    void ExecuteMove(const FGhostActionData& Action);

    //攻撃実行関数
    void ExecuteAttack();

    //回避実行関数
    void ExecuteDodge(const FGhostActionData& Action);

    //ジャンプ実行関数
    void ExecuteJump();

    //味方フェーズ用：直接ヒット判定
    void PerformFriendlyAttackHit(AGhostCharacter* Ghost);

    //
    //void TickRecordedPlayback(float DeltaTime);

    //遅延してミラー
    //void TickDelayedMirror(float DeltaTime);

    //即時ミラー
    void TickImmediateMirror(float DeltaTime);

    //毎 フレーム で移動入力を維持する処理
    void TickMovement(float DeltaTime);

	//囲み移動の処理
    void TickSurroundMovement(float DeltaTime);

    //アニメーションのシンクロ
    void SyncAttackAnimFlag();

private:
    // モード
    EGhostPlaybackMode CurrentMode = EGhostPlaybackMode::ImmediateMirror;

    // RecordedPlayback
   // TArray<FGhostActionData> PlaybackQueue;
    //int32 PlaybackIndex = 0;
    //float PlaybackStartTime = 0.f;
    //float SnapshotStartTime = 0.f;

    //
    UPROPERTY()
    TObjectPtr<UGhostRecorderComponent> RecorderRef = nullptr;

    //遅延ミラー用
    float LastMirrorFetchTime = 0.f;

    //即時ミラー用：最後にフェッチしたタイムスタンプ
    float LastImmediateFetchTime = 0.f;

    //移動状態
    FVector CurrentMoveDirection = FVector::ZeroVector;

    //最後に Move レコードを受け取った時刻
    float LastMoveReceivedTime = -999.f;

    // 攻撃の連打防止
    bool bPrevAttackingCombat = false;

	//囲み移動用
    float SurroundAngleOffset = 0.f;
};