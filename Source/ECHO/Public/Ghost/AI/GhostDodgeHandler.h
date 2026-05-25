//GhostDodgeHandleer.h
//ゴーストドッジハンドラーヘッダー

#pragma once

#include "CoreMinimal.h"
#include "Ghost/AI/GhostActionHandlerBase.h"
#include "GhostDodgeHandler.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ECHO_API UGhostDodgeHandler : public UGhostActionHandlerBase
{
    GENERATED_BODY()

public:
    virtual EGhostActionType GetHandledType() const override
    {
        return EGhostActionType::Dodge;
    }

    virtual void Execute(const FGhostActionData& Data, ACharacter* Owner) override;

private:
    //重力を戻すタイマー
    FTimerHandle DodgeEndTimer;
};