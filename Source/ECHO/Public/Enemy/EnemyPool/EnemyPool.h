// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyPool.generated.h"

//前方宣言
class AEnemyChara;

UCLASS()
class ECHO_API AEnemyPool : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnemyPool();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	//プールから敵を取り出す関数
	UFUNCTION(BlueprintCallable, Category = "EnemyPool")
	AEnemyChara* SpawnEnemyAtRandomLocation();

	//プールから敵を取り出す関数
	UFUNCTION(BlueprintCallable, Category = "EnemyPool")
	AEnemyChara* SpawnEnemy(FVector _location, FRotator _rotation);

	//使い終わったら敵をプールに返す関数
	UFUNCTION(BlueprintCallable, Category = "EnemyPool")
	void ReturnEnemy(AEnemyChara* _enemy);

	//現在アクティブな敵の数を取得する関数
	UFUNCTION(BlueprintCallable, Category = "EnemyPool")
	int32 GetActiveEnemyCount() const { return m_activePool.Num(); }

	//現在待機中の敵の数を取得する関数
	UFUNCTION(BlueprintCallable, Category = "EnemyPool")
	int32 GetInactiveEnemyCount() const { return m_inactivePool.Num(); }

public:
	//敵のクラスを指定するためのプロパティ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyPool")
	TSubclassOf<AEnemyChara> m_pEnemyClass;

	//プールの初期サイズ
	UPROPERTY(EditAnywhere, Category = "EnemyPool", meta = (ClampMin = "1", ClampMax = "50"))
	int32 m_initialPoolSize = 10; 

	//敵をスポーンする半径
	UPROPERTY(EditAnywhere, Category = "EnemyPool", meta = (ClampMin = "100.0"))
	float m_spawnRadius = 500.f; 

	//スポーンの最大試行回数
	UPROPERTY(EditAnywhere, Category = "EnemyPool", meta = (ClampMin = "1", ClampMax = "20"))
	int32 m_spawnMaxAttemps = 5;
private:
	//プールの初期化
	void InitPool();

private:
	//待機中の敵の配列
	UPROPERTY()
	TArray<AEnemyChara*> m_inactivePool; 

	//現在アクティブな敵の配列
	UPROPERTY()
	TArray<AEnemyChara*> m_activePool; 

};
