#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GhostManagerComponent.generated.h"

//前方宣言
class AGhostCharacter;
class UGhostRecorderComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ECHO_API UGhostManagerComponent : public UActorComponent
{
	GENERATED_BODY()

    public:
    //コンストラクタ
    UGhostManagerComponent();

	//開始時に呼ばれる関数
    virtual void BeginPlay() override;

    //召喚を試みる。エネルギーと最大数を確認してから実行する。
    UFUNCTION(BlueprintCallable, Category = "Ghost|Manager")
    bool TrySummon(float Cost);

    /** 現在の召喚数 */
    UFUNCTION(BlueprintPure, Category = "Ghost|Manager")
    int32 GetActiveCount() const { return ActiveGhosts.Num(); }

    //ポーンするクラス
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Manager")
    TSubclassOf<AGhostCharacter> GhostCharacterClass;

    //同時召喚可能な最大数
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Manager",
        meta = (ClampMin = "1", ClampMax = "5"))
    int32 MaxGhostCount = 5;

    //スナップショットとして取得する過去秒数
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Manager",
        meta = (ClampMin = "3.0", ClampMax = "15.0"))
    float SnapshotDuration = 10.f;

    //召喚位置のオフセット
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Manager")
    FVector SpawnOffset = FVector(-100.f, 80.f, 0.f);

private:
    // アクティブ Ghost 管理
    UPROPERTY()
    TArray<TObjectPtr<AGhostCharacter>> ActiveGhosts;

    //死亡・無効になった Ghost をリストから除去
    void CleanupDeadGhosts();
    
    // キャッシュ
    UPROPERTY()
    TObjectPtr<UGhostRecorderComponent> RecorderRef;
};
