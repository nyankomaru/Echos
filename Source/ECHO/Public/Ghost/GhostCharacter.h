//GhostCharacter.h
//ゴーストキャラクターヘッダー
//
//見た目と当たり判定を持つActor
//侵食タイマーと残像ゲージの消費監視を持つ

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GhostType.h"
#include "Ghost/GhostPlaybackComponent.h"
#include "Ghost/AI/GhostAIComponent.h"
#include "GhostCharacter.generated.h"

UCLASS()
class ECHO_API AGhostCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGhostCharacter();

	//召喚時にRecorderComponentから呼ぶ初期化関数
	void InitGhost(const TArray<FGhostActionData>& InActions);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	UGhostAIComponent* GhostAIComponent;


	UPROPERTY(VisibleAnywhere)
	UGhostPlaybackComponent* PlaybackComponent;

	//再生する行動データ
	TArray<FGhostActionData> RecordedActions;

	//浸食タイマー(30秒で敵化)
	FTimerHandle CorruptionTimerHandle;

	//浸食タイマーが切れた時の処理
	void OnCorruptionTimeout();

	//浸食までの時間
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float CorruptionDuration;

public:
	//見た目確認ようにBPから色変えられるようにしておく
	UPROPERTY(EditAnywhere, Category = "Ghost")
	UMaterialInterface* GhostMaterial = nullptr;

};
