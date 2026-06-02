#pragma once

#include "CoreMinimal.h"
#include "Enemy/EnemyChara.h"
#include "MidBoss.generated.h"

UCLASS()
class ECHO_API AMidBoss : public AEnemyChara
{
	GENERATED_BODY()
	
public:
	//コンストラクタ
	AMidBoss();

protected:
	//ゲーム開始時に呼ばれる関数
	virtual void BeginPlay() override;

public:
	//毎フレーム呼ばれる関数
	virtual void Tick(float _deltaTime) override;

	//攻撃関数
	virtual void Attack() override;

	//死亡関数
	virtual void Die() override;

	UFUNCTION(BlueprintCallable, Category = "MidBoss")
	int32 GetCurrentPhase() const { return m_currentPhase; }

	//通常の近接攻撃
	UFUNCTION(BlueprintCallable, Category = "MidBoss | Attack")
	void PerformSlashAttack();

	//ワイヤートラップを設置する攻撃
	UFUNCTION(BlueprintCallable, Category = "MidBoss | Attack")
	void PlaceWireTrap();

	//突進攻撃
	UFUNCTION(BlueprintCallable, Category = "MidBoss | Attack")
	void PerformCharageAttack();

private:
	//フェーズを更新する関数
	void UpdatePhase();

	//フェーズが変わったときの処理
	void OnPhaseChanged(int32 _newPhase);

private:
	//現在のフェーズ
	int32 m_currentPhase = 1; 

	//フェーズ2に移行する体力の閾値
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MidBoss | Phase", meta = (AllowPrivateAccess = "true"))
	float m_phaseTwoThreshold = 60.f;

	//フェーズ3に移行する体力の閾値
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MidBoss | Phase", meta = (AllowPrivateAccess = "true"))
	float m_phaseThreeThreshold = 30.f;

	//スラッシュ攻撃の範囲
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MidBoss | Attack", meta = (AllowPrivateAccess = "true"))
	float m_slashRadius = 80.f;

	//スラッシュ攻撃の長さ
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MidBoss | Attack", meta = (AllowPrivateAccess = "true"))
	float m_slashLength = 200.f;

	//ワイヤートラップのダメージ
	UPROPERTY(EditDefaultsOnly, Category = "MidBoss | Attack")
	float m_wireDamage = 20.f;

	//ワイヤーの範囲半径
	UPROPERTY(EditDefaultsOnly, Category = "MidBoss | Attack")
	float m_wireRadius = 150.f;

	//突進攻撃のダメージ倍率
	UPROPERTY(EditDefaultsOnly, Category = "MidBoss | Attack")
	float m_chargeDamageMultiplier  = 2.f;

	//突進攻撃の距離
	UPROPERTY(EditDefaultsOnly, Category = "MidBoss | Attack")
	float m_chargeDistance = 600.f;

	//移動速度
	UPROPERTY(EditDefaultsOnly, Category = "MidBoss | Attack")
	float m_phaseThreeMoveSpeed = 500.f;

	//クールダウンタイマー
	UPROPERTY(EditDefaultsOnly, Category = "MidBoss | Attack")
	float m_phaseThreeCooldown = 0.6f;

	//攻撃のクールダウンフラグ
	bool m_bChargeCooldown = false;

	//突進攻撃のクールダウンタイマー
	FTimerHandle m_chargeCooldownTimer;
};
