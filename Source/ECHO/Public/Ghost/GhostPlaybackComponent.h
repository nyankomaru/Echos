//GhostPlaybackComponent.h
//ゴーストプレイバックコンポーネントヘッダー
//
//渡された記録データを順番に再生する
//再生が終わったらループするか待機するか選べるようにする

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GhostType.h"
#include "GhostPlaybackComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ECHO_API UGhostPlaybackComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGhostPlaybackComponent();

	//GhostCharacterのInitGhostから呼ぶ
	void StartPlayback(const TArray<FGhostActionData>& InActions);

protected:
	virtual void BeginPlay() override;

private:
	//再生する行動データ
	TArray<FGhostActionData> Actions;

	//現在再生中のインデックス
	int32 CurrentIndex;

	//再生時間時のワールド時刻
	float PlaybackStartTime;

	//記録開始時のワールド時刻
	float RecordStartTime;

	//次のアクションを実行するタイマー
	FTimerHandle PlaybackTimerHandle;

	//次のアクションのスケジュールする
	void ScheduleNextAction();
	
	//アクションを実行する
	void ExecuteCurrentAction();

	//各アクションの実行
/*	void PlayMove(const FGhostActionData& Data);
	void PlayAttack(const FGhostActionData& Data);
	void PlayDodge(const FGhostActionData& Data);	*/	
};
