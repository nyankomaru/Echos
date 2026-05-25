//CombatComponent.h
//コンバットコンポーネントヘッダー
//
//戦闘統括コンポーネント
//コンボの状態管理、現在装備中の武器
//回避、アクショントリガー、無敵時間

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Ghost/GhostType.h"
#include "CombatComponent.generated.h"

//分身システムに攻撃・回避アクションを通知するためのデリゲート
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGhostAction, const FGhostActionData&);
//敵にヒットした際、キャラクタークラスにダメージを通知するデリゲート
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHitEnemyDelegate, float);

USTRUCT(BlueprintType)
struct FComboStepData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    UAnimMontage* Montage = nullptr;

    //攻撃判定の球体の半径
    UPROPERTY(EditAnywhere)
    float HitRadius = 60.f;

    //攻撃判定を前方へ伸ばす長さ
    UPROPERTY(EditAnywhere)
    float HitRange = 100.f;

    //敵に与える基本ダメージ
    UPROPERTY(EditAnywhere)
    float Damage = 20.f;

    //吹き飛ばし力（0なら吹き飛ばしなし）
    UPROPERTY(EditAnywhere)
    float LaunchForce = 0.f;

    //コンボ入力受付開始タイミング
    UPROPERTY(EditAnywhere)
    float ComboWindowStart = 0.5f;

    //コンボリセットまでの時間
    UPROPERTY(EditAnywhere)
    float ComboResetTime = 1.0f;
};


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ECHO_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatComponent();

    //攻撃ボタン入力時の関数
    void ExecuteAttack();
    //回避ボタン入力時の関数
    void ExecuteDodge();
    //キャラクターの着地時などに、空中回避の使用済みフラグをリセットする関数
    void ResetAirDodge();

    //AnimationNotifyから呼ばれる、当たり判定
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void CheckHit();

    //AnimationNotifyから呼ばれる、コンボの次の段階への移行を受け付ける関数
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void OpenComboWindow();

    //AnimationNotifyから呼ばれる、コンボ受付終了関数
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void CloseComboWindow();

    FOnGhostAction OnGhostActionRecorded;
    FOnHitEnemyDelegate OnHitEnemy;

protected:
    //BPからコンボルートや各段数のパラメータを設定する配列
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
    TArray<FComboStepData> ComboSteps;

    //回避時に押し出す瞬発力な力
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
    float DodgeForce = 6000.f;

    //回避アクションの持続時間
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
    float DodgeDuration = 0.1f;

    //次に回避が使用可能になるまでのクールダウン
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
    float DodgeCooldown = 0.5f;

private:
    // --- コンボ制御の内容状態変数 ---
    int32 CurrentComboIndex = 0;        //現在コンボの何段目か
    bool bIsAttacking = false;          //現在攻撃モーション中か
    bool bComboWindowOpen = false;      //現在次のコンボ入力を受ける状態か
    bool bComboInputBuffered = false;   //受付窓口が開く前に攻撃ボタンが押された場合の「先行入力」

    // --- タイマーハンドル ---
    FTimerHandle ComboResetTimerHandle;
    FTimerHandle DodgeTimerHandle;
    FTimerHandle DodgeCoolDownTimerHandle;

    // --- 回避制御の内部状態変数 ---
    bool bCanDodge = true;              //現在回避可能かどうか
    bool bHasAirDodged = false;         //空中回避を既に実行したか
    float CachedGravityScale = 1.f;     //回避前のキャラクターの重力保持用
    float CachedGroundFriction = 8.f;   //回避前のキャラクターの地面摩擦保持用

    //多段ヒット防止、当たった敵を記録するリスト
    TArray<AActor*> HitActorsThisAttack;

    //指定されたコンボ段数のアニメーション再生とデータ初期化を行う関数
    void ExecuteComboStep(int32 StepIndex);
    //コンボ入力が間に合わずにタイマーが満了した際のリセット関数
    void OnComboResetTimeout();
    //回避時間が終了した際に物理状態を元に戻す関数
    void EndDodge();
    //クールダウンタイマー終了時に回避状態に戻す関数
    void ResetDodgeCooldown();
};