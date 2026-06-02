#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossSpawnTrigger.generated.h"

//前方宣言
class UBoxComponent;
class AWaveManager;

UCLASS()
class ECHO_API ABossSpawnTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABossSpawnTrigger();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	//トリガーとして使うボックスコンポーネント
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trigger")
	UBoxComponent* m_triggerBox;

	//スポーンマネージャーへの参照
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
	AWaveManager* m_waveManager;

	//プレイヤーがトリガーに入ったときにスポーンさせるかどうかのフラグ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
	bool m_bSpawnOnEnter = true;
private:
	//プレイヤーがトリガーに入ったときの処理
	UFUNCTION()
	void OnPlayerEntered(UPrimitiveComponent* _overlappedComp, AActor* _otherActor, UPrimitiveComponent* _otherComp, int32 _otherBodyIndex, bool _bFromSweep, const FHitResult& _sweepResult);

private:
	//一度スポーンしたら二度とスポーンさせないためのフラグ
	bool m_bHasTriggered = false;

};
