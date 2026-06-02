// CombatComponent.h
// 戦闘統括コンポーネント
// プレイヤー・Ghost 共用

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

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

    bool IsAttacking() const { return bIsAttacking; }
    bool IsDodging()   const { return bIsDodging; }
    bool CanDodge()    const { return bCanDodge; }

protected:
    bool bIsAttacking = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
    UAnimMontage* AttackMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
    float DodgeForce = 6000.f;

    bool bHasAirDodged = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
    float DodgeDuration = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
    float DodgeCooldown = 0.5f;

    bool bIsDodging = false;
    bool bCanDodge = true;

    FTimerHandle DodgeTimerHandle;
    FTimerHandle DodgeCoolDownTimerHandle;

    // 攻撃フラグリセット用タイマー
    FTimerHandle AttackResetTimerHandle;

    void ResetDodgeCooldown();
    void EndDodge();

    // 攻撃フラグをリセットする（モンタージュ終了後に呼ばれる）
    void ResetAttack();

    float CachedGravityScale = 1.f;
    float CachedGroundFriction = 8.f;

public:
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void CheckHit();

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnHitEnemy, float);
    FOnHitEnemy OnHitEnemy;

private:
    TArray<AActor*> HitActorsThisAttack;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float AttackRadius = 80.f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float AttackRange = 120.f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float AttackDamage = 20.f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float EnergyGainPerHit = 15.f;
};