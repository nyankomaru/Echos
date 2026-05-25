//AEnemyChar.h
//ダメージを受けるだけの仮エネミークラス

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KariEnemyChar.generated.h"

//ヒット時にプレイヤーへ通知するデリゲート
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyHit, float, Damage);

UCLASS()
class ECHO_API AEnemyChar : public ACharacter
{
    GENERATED_BODY()

public:
    AEnemyChar();

    //プレイヤーのCombatComponentから呼ぶ
    virtual float TakeDamage(
        float DamageAmount,
        const FDamageEvent& DamageEvent,
        AController* EventInstigator,
        AActor* DamageCauser) override;

    //ヒット時のデリゲート
    UPROPERTY(BlueprintAssignable)
    FOnEnemyHit OnEnemyHit;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category = "Enemy")
    float MaxHealth = 100.f;

    UPROPERTY(VisibleAnywhere, Category = "Enemy")
    float CurrentHealth;
};
