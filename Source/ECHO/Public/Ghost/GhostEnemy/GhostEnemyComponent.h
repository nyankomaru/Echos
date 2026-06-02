// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Ghost/Data/GhostTypes.h"
#include "GhostEnemyComponent.generated.h"

// 状態変化通知デリゲート
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGhostStateChanged, EGhostLifecycleState, NewState);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ECHO_API UGhostEnemyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UGhostEnemyComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // -----------------------------------------------------------------------
    // 外部 API
    // -----------------------------------------------------------------------

    /** 召喚時に呼んでタイマーをスタートさせる */
    void StartLifetimeTimer();

    /** 現在の状態を返す */
    UFUNCTION(BlueprintPure, Category = "Ghost|Enemy")
    EGhostLifecycleState GetLifecycleState() const { return CurrentState; }

    /** 残り時間（秒）を返す（UI 用） */
    UFUNCTION(BlueprintPure, Category = "Ghost|Enemy")
    float GetRemainingTime() const;

    /** 正規化した残り時間（0?1）を返す（UI ゲージ用） */
    UFUNCTION(BlueprintPure, Category = "Ghost|Enemy")
    float GetRemainingTimeNormalized() const;

    // -----------------------------------------------------------------------
    // イベント
    // -----------------------------------------------------------------------

    /** Friendly/Warning/Corrupted/Enemy/Dead のいずれかに変化したとき発火 */
    UPROPERTY(BlueprintAssignable, Category = "Ghost|Enemy")
    FOnGhostStateChanged OnStateChanged;

    // -----------------------------------------------------------------------
    // 設定
    // -----------------------------------------------------------------------

    /** 敵化までの時間（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Enemy",
        meta = (ClampMin = "5.0", ClampMax = "120.0"))
    float LifetimeSeconds = 30.f;

    /** Warning 演出を開始する残り秒数 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Enemy",
        meta = (ClampMin = "1.0", ClampMax = "10.0"))
    float WarningThreshold = 3.f;

private:
    EGhostLifecycleState CurrentState = EGhostLifecycleState::Friendly;

    float ElapsedTime = 0.f;
    bool  bTimerActive = false;

    /** 状態を変更してデリゲートを発火する */
    void SetState(EGhostLifecycleState NewState);

    /** 敵化処理の実行（AIController スポーン・チーム変更） */
    void Corrupt();
};
