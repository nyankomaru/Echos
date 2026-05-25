//GhostActionHandlerBase.h
//ゴーストアクションハンドラーベースヘッダー

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Ghost/GhostType.h"
#include "GhostActionHandlerBase.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ECHO_API UGhostActionHandlerBase : public UActorComponent
{
	GENERATED_BODY()

public:
	//このハンドラが担当するアクションの種類
	virtual EGhostActionType GetHandledType() const
		PURE_VIRTUAL(UGhostActionHandlerBase::GetHandledType, return EGhostActionType::Move;);

	//アクションを実行する
	virtual void Execute(const FGhostActionData& Data, ACharacter* Owner)
		PURE_VIRTUAL(UGhostActionHandlerBase::Execute, );
};
