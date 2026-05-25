//GhostRecorderComponent.h
//ゴーストレコーダーコンポーネントヘッダー
//
//アクションイベントをリングバッファで記録（直近30秒分）
//召喚時に記録データのスナップショットを渡す
//CombatComponentのフック箇所から通知を受け取る

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GhostType.h"
#include "GhostRecorderComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ECHO_API UGhostRecorderComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGhostRecorderComponent();

	//CombatComponentから呼ぶ記録関数
	void RecordAction(const FGhostActionData& ActionData);

	//召喚時に呼ぶ、直近の記録データのコピーを返す
	TArray<FGhostActionData> GetRecordedActions() const;

protected:
	virtual void BeginPlay() override;

private:
	//リングバッファ本体
	TArray<FGhostActionData>ActionBuffer;

	//保持する件数
	UPROPERTY(EditAnywhere, Category = "Ghost")
	int32 MaxRecordCount;

	//古いデータを削除する
	//void PurgeOldActions();
};
