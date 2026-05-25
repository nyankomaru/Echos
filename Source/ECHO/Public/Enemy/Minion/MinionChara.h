#pragma once

#include "CoreMinimal.h"
#include "Enemy/EnemyChara.h"
#include "MinionChara.generated.h"

UCLASS()
class ECHO_API AMinionChara : public AEnemyChara
{
	GENERATED_BODY()

public:
	//コンストラクタ
	AMinionChara();

protected:
	//ゲーム開始時またはスポーン時に呼び出される関数
	virtual void BeginPlay() override;

public:
	//毎フレーム呼び出される関数
	virtual void Tick(float _deltaTime) override;

	//攻撃用関数
	virtual void Attack() override;
	
	//倒されたときの関数
	virtual void Die() override;

	//攻撃範囲を取得する関数
	FORCEINLINE float GetAttackRange() const { return m_attackRange; }

	//突進開始関数
	UFUNCTION(BlueprintCallable, Category = "Minion | Rush")
	void StartRush();

	//突進終了関数
	UFUNCTION(BlueprintCallable, Category = "Minion | Rush")
	void StopRush();

	//突進中かどうかを取得する関数
	UFUNCTION(BlueprintCallable, Category = "Minion | Rush")
	bool IsRushing() const { return m_isRushing; }

private:
	//突進のビヘイビアーを更新する関数
	void UpdateRushBehavior();

	//攻撃ヒット判定
	void PerformAttackTrace();

private:
	//通常の移動速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Minion | Speed")
	float m_normalSpeed;

	//突進中の移動速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Minion | Speed")
	float m_rushSpeed;

	//突進を開始する距離
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Minion | Rush")
	float m_rushTriggerDistance;

	//攻撃のヒット判定の半径
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Minion | Attack")
	float m_attackTraceRadius;

	//攻撃ヒット判定の長さ
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Minion | Attack")
	float m_attackTraceLength;

	//突進中かどうかを示すフラグ
	bool m_isRushing; 
};
