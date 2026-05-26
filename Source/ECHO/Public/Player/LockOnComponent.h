//LockOnComponent.h
//ロックオンコンポーネントヘッダー
//
//ターゲット管理・トグル処理・自動解除

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLockOnChanged, AActor*);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ECHO_API ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULockOnComponent();

	//トグル
	void ToggleLockOn();

	//ロックオン中のみ右スティック入力で隣の敵に切り替え
	void TrySwitchTarget(float StickX);

	//敵が死亡した瞬間に呼ばれる
	UFUNCTION()
	void OnEnemyEliminated(AEnemyChara* EliminatedEnemy);

	//現在のターゲットを取得
	AActor* GetTarget() const { return CurrentTarget.Get(); }

	//ロックオン中かどうか
	bool IsLockedOn() const { return CurrentTarget.IsValid(); }

	//ターゲット変更通知デリゲート
	FOnLockOnChanged OnLockOnChanged;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	//現在のターゲット
	TWeakObjectPtr<AActor> CurrentTarget;

	//ロックオン可能な最大距離
	float LockOnRange = 1500.f;

	//ロックオン自動解除距離
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float LockOnBreakRange = 2000.f;

	//右スティック切り替えの入力基準値
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float SwitchThreshold = 0.7f;

	//切り替え後の入力クールダウン
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float SwitchCooldown = 0.4f;

	//切り替えクールダウンの残り時間
	float SwitchCooldownTimer = 0.f;

	//スティックが基準値を超えているか
	bool bSwitchInputActive = false;

	//画面中央に最も近い敵を探す
	AActor* FindBestTarget() const;

	//現在のターゲットから見て右か左にいる次の敵を探す
	AActor* FindNextTarget(float Direction) const;

	//ロックオンを解除する
	void ClearLockOn();

	//ターゲットが有効かチェック
	bool IsTargetValid(AActor* Target) const;
};
