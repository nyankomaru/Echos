#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveManager.generated.h"

class AEnemyPool;
class AEnemyChara;

UENUM(BlueprintType)
enum class EAttackRole : uint8
{
	//待機中
	Waiting UMETA(DisplayName = "Waiting"),
	//攻撃役
	Attacker UMETA(DisplayName = "Attacker"),
	//包囲役
	Encircler UMETA(DisplayName = "Encircler"),
};


//BBキー名
namespace WaveBBKey
{
	static const FName bIsActiveAttacker(TEXT("bIsActiveAttacker"));
	static const FName EncirclePosition(TEXT("EncirclePosition"));
}

UCLASS()
class ECHO_API AWaveManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWaveManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:

	void Tick(float _deltaTime) override;

	//Waveを開始する関数
	UFUNCTION(BlueprintCallable, Category = "WaveManager")
	void StartWave();

	//敵が死んだ時呼ばれる関数
	UFUNCTION(BlueprintCallable, Category = "WaveManager")
	void NotifyEnemyDefeated(AEnemyChara* _defeatedEnemy);

	//中間ボスを指定位置にスポーン
	UFUNCTION(BlueprintCallable, Category = "WaveManager")
	void SpawnMidBoss();

	//ラストボスを指定位置にスポーン
	UFUNCTION(BlueprintCallable, Category = "WaveManager")
	void SpawnFinalBoss();

	//現在生存している敵の数を取得する関数
	UFUNCTION(BlueprintCallable, Category = "WaveManager")
	int32 GetAliveEnemyCount() const;

protected:
	//エネミープールを参照するためのプロパティ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WaveManager")
	AEnemyPool* m_pEnemyPool;

	//1ウェーブあたりの敵の数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WaveManager", meta = (ClampMin = "1"))
	int32 m_enemiesPerWave = 8;

	//攻撃役の数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "2", ClampMax = "4"))
	int32 m_attackersPerWave = 3;

	//包囲役のポジションを決めるための半径
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "100.0"))
	float m_encircleRadius = 300.f;

	//役割の更新間隔
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "1.0"))
	float m_roleUpdateInterval = 5.f;

	//ラストボスをスポーンさせるためのポイント
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave | Boss")
	AActor* m_midBossSpawnPoint;

	//雑魚敵を倒したら中間ボスを出る確率
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave | Boss", meta = (ClampMin = "1", ClampMax = "99"))
	int32 m_midBossSpawnPercent = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave | Boss")
	TSubclassOf<AEnemyChara> m_midBossClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave | Boss")
	TSubclassOf<AEnemyChara> m_finalBossClass;

	//ラストボスをスポーンさせるためのアクター
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave | Boss")
	AActor* m_finalBossSpawnPoint;

private:
	//アタッカー・エンサークラーを再割り当てする関数
	void UpdateAttackerRoles();

	//包囲役の位置を割り当てる関数
	void AssignEncirclePositions(AEnemyChara* _enemy, int32 _index, int32 _total);

	//包囲役の位置を計算する関数
	FVector CalculateEncirclePosition(FVector _playerPos, int32 _index, int32 _total);

	//プレイヤーを探して返す関数
	APawn* FindPlayer() const;

	//敵に攻撃役か包囲役の役割を割り当てる関数
	void SetAttackerRole(AEnemyChara* _enemy, bool _isAttacker);

	//敵に包囲役の役割を割り当てる関数
	void SetEncircleRole(AEnemyChara* _enemy, const FVector& _encirclePosition);

private:
	//現在生存している敵のリスト
	UPROPERTY()
	TArray<AEnemyChara*> m_aliveEnemies;

	//現在攻撃役の敵のリスト
	UPROPERTY()
	TArray<AEnemyChara*> m_activeAttackers;

	//中間ボスの参照
	UPROPERTY()
	AEnemyChara* m_midBoss = nullptr;

	//ラストボスの参照
	UPROPERTY()
	AEnemyChara* m_finalBoss = nullptr;

	//ウェーブが開始されたかどうか
	bool m_bWaveStarted = false;

	//中間ボスがスポーンされたかどうか
	bool m_bMidBossSpawned = false;

	//ラストボスがスポーンされたかどうか
	bool m_bFinalBossSpawned = false;

	//最初の敵の数
	int32 m_initialEnemyCount = 0;

	//倒された敵の数
	int32 m_defeatedEnemyCount = 0;

	//役割の更新タイマー
	float m_roleUpdateTimer = 0.f;
};
