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
    // IAbilitySystemInterface
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // 初期化（召喚後に呼ばれる）
    void InitializeGhost(const TArray<FGhostActionData>& Snapshot, UGhostRecorderComponent* Recorder);

    // 敵化処理
    void ActivateEnemyAI();

    //Playback コンポーネント
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Components")
    UGhostPlaybackComponent* PlaybackComponent;

	//戦闘関連のコンポーネント
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Components")
    UCombatComponent* CombatComponent;

	//敵モードの追加コンポーネント
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Components")
    UGhostEnemyComponent* EnemyComponent;

    //GAS
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|GAS")
    UGhostAbilitySystemComponent* GhostASC;

    //攻撃アビリティ
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ghost|GAS")
    TSubclassOf<UGameplayAbility> AttackAbilityClass;

    //回避アビリティ
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ghost|GAS")
    TSubclassOf<UGameplayAbility> DodgeAbilityClass;

    //敵化時にスポーンする AIController
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ghost|AI")
    TSubclassOf<AAIController> GhostAIControllerClass;

    
    //敵化時にメッシュへ適用するマテリアル
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ghost|Visual")
    UMaterialInterface* EnemyMaterial;

private:
    //GAS 二重初期化防止フラグ
    bool bGASInitialized = false;

    void GrantDefaultAbilities();
};