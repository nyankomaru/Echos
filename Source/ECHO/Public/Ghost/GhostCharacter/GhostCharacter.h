// GhostCharacter.h
// 分身キャラクター本体
// 全ファイルとの整合性を取った最終版

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "AIController.h"
#include "Ghost/Data/GhostTypes.h"
#include "GhostCharacter.generated.h"

class UGhostPlaybackComponent;
class UGhostEnemyComponent;
class UGhostAbilitySystemComponent;
class UGhostRecorderComponent;
class UCombatComponent;

UCLASS()
class ECHO_API AGhostCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AGhostCharacter();

protected:
    virtual void BeginPlay() override;

public:
    // -----------------------------------------------------------------------
    // IAbilitySystemInterface
    // -----------------------------------------------------------------------
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // -----------------------------------------------------------------------
    // 初期化（召喚後に呼ばれる）
    // -----------------------------------------------------------------------
    void InitializeGhost(
        const TArray<FGhostActionData>& Snapshot,
        UGhostRecorderComponent* Recorder);

    // -----------------------------------------------------------------------
    // 敵化処理
    // GhostEnemyComponent::Corrupt() から呼ばれる
    // ① メッシュマテリアルを EnemyMaterial に変更
    // ② GhostAIControllerClass をスポーンして Possess
    // -----------------------------------------------------------------------
    void ActivateEnemyAI();

    // -----------------------------------------------------------------------
    // コンポーネント（public にして PlaybackComponent / CombatComponent を
    // GhostEnemyComponent / GhostPlaybackComponent から参照できるようにする）
    // -----------------------------------------------------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Components")
    UGhostPlaybackComponent* PlaybackComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Components")
    UCombatComponent* CombatComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Components")
    UGhostEnemyComponent* EnemyComponent;

    // -----------------------------------------------------------------------
    // GAS
    // ※ "AbilitySystemComponent" は IAbilitySystemInterface のマクロ展開と
    //   衝突するため GhostASC という名前にしている
    // -----------------------------------------------------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|GAS")
    UGhostAbilitySystemComponent* GhostASC;

    // -----------------------------------------------------------------------
    // BP 設定
    // -----------------------------------------------------------------------

    /** 攻撃アビリティ（BP_GhostCharacter の Class Defaults で設定） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ghost|GAS")
    TSubclassOf<UGameplayAbility> AttackAbilityClass;

    /** 回避アビリティ（BP_GhostCharacter の Class Defaults で設定） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ghost|GAS")
    TSubclassOf<UGameplayAbility> DodgeAbilityClass;

    /** 敵化時にスポーンする AIController */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ghost|AI")
    TSubclassOf<AAIController> GhostAIControllerClass;

    /**
     * 敵化時にメッシュへ適用するマテリアル（赤い半透明を推奨）
     * 未設定の場合はスキップ（エラーにはならない）
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ghost|Visual")
    UMaterialInterface* EnemyMaterial;

private:
    /** GAS 二重初期化防止フラグ */
    bool bGASInitialized = false;

    void GrantDefaultAbilities();
};