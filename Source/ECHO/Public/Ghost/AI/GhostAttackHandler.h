//GhostAttackHandler.h
//ゴーストアタックハンドラーヘッダー

#pragma once

#include "CoreMinimal.h"
#include "Ghost/AI/GhostActionHandlerBase.h"
#include "Engine/EngineTypes.h"
#include "GhostAttackHandler.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ECHO_API UGhostAttackHandler : public UGhostActionHandlerBase
{
    GENERATED_BODY()

public:
    virtual EGhostActionType GetHandledType() const override
    {
        return EGhostActionType::Attack;
    }

    virtual void Execute(const FGhostActionData& Data, ACharacter* Owner) override;

    //BPで指定するモンタージュ
    UPROPERTY(EditAnywhere, Category = "Ghost")
    UAnimMontage* AttackMontage;

private:
    // ヒット判定の範囲
    UPROPERTY(EditAnywhere, Category = "Ghost")
    float AttackRadius = 60.f;

    // ヒット判定の距離
    UPROPERTY(EditAnywhere, Category = "Ghost")
    float AttackRange = 100.f;

    // ダメージ量
    UPROPERTY(EditAnywhere, Category = "Ghost")
    float AttackDamage = 10.f;
};