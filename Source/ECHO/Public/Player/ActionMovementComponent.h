//ActionMovementComponent.h
//アクションムーブメントコンポーネントヘッダー
//
//移動・物理の拡張コンポーネント
//ダッシュ、2段ジャンプ時の空中制御、着地時の硬直など

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ActionMovementComponent.generated.h"

UCLASS()
class ECHO_API UActionMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()

public:
    UActionMovementComponent();

    //キャラクターの移動における加速度と摩擦を設定する関数
    void SetMovementWeight(float NewAcceleration, float NewFriction);

    //ダッシュ状態への切り替え、最大歩行速度を切り替える関数
    void SetSprinting(bool _bIsSprinting);

    //現在ダッシュ中かどうか取得する関数
    bool IsSprinting() const { return bIsSprinting; }

    //着地後の硬直処理を開始する関数
    void StartLandingRecovery(bool bWasSprinting);

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // --- 歩行・ダッシュ速度設定 ---

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionMovement")
    float BaseWalkSpeed = 700.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionMovement")
    float SprintSpeed = 1150.f;

    //現在ダッシュ中かどうかのフラグ
    bool bIsSprinting = false;

public:
    // --- アクション状態フラグ ---

    //現在回避中かどうか
    bool bIsDodging = false;

    // --- ジャンプ・空中制御関連パラメータ ---

    //１段目ジャンプ時の空中横移動の効き具合
    UPROPERTY(EditAnywhere, Category = "Jump")
    float AirControlFirstJump = 0.2f;

    //２段目ジャンプ時の空中横移動の効き具合
    UPROPERTY(EditAnywhere, Category = "Jump")
    float AirControlSecondJump = 0.1f;

    //２段目ジャンプ発動時に与えられる上方向（Z）の初速
    UPROPERTY(EditAnywhere, Category = "Jump")
    float SecondJumpZVelocity = 400.f;

    //空中で入力方向に加える固定の力
    UPROPERTY(EditAnywhere, Category = "Jump")
    float AirPushForce = 1500.f;

    //空中の横方向最大速度
    UPROPERTY(EditAnywhere, Category = "Jump")
    float AirMaxHorizontalSpeed = 800.f;

    //  --- 着地硬直関連パラメータ ---

    //着地後の速度回復時間
    UPROPERTY(EditAnywhere, Category = "Landing")
    float LandingRecoveryTime = 0.2f;

    //着地硬直中の速度倍率
    UPROPERTY(EditAnywhere, Category = "Landing")
    float LandingSpeedMultiplier = 0.3f;

private:
    //現在着地硬直の回復処理中か
    bool bIsLandingRecovery = false;
    //着地硬直開始からの経過時間
    float LandingRecoveryElapsed = 0.f;
    //着地直前の元の最大速度
    float CachedMaxWalkSpeed = 0.f;

    //落下時の最大速度制限
    float MaxFallSpeed = 6000.f;
};