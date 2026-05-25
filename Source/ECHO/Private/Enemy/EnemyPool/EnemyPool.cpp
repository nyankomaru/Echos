#include "Enemy/EnemyPool/EnemyPool.h"
#include "Enemy/EnemyChara.h"
#include "NavigationSystem.h"
// Sets default values
AEnemyPool::AEnemyPool()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AEnemyPool::BeginPlay()
{
	Super::BeginPlay();

	//プールの初期化
	InitPool();

	SpawnEnemyAtRandomLocation();
}

//プールの初期化関数
void AEnemyPool::InitPool()
{
	//敵のクラスが設定されているか確認
	if (!m_pEnemyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Enemy class is not set in EnemyPool"));
		return;
	}

	//スポーンパラメータの設定
	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;	//スポーン時の衝突処理を常にスポーンするに設定

	//初期プールサイズ分の敵を生成して待機中のプールに追加
	for (int32 i = 0; i < m_initialPoolSize; ++i)
	{
		AEnemyChara* enemy = GetWorld()->SpawnActor<AEnemyChara>(m_pEnemyClass, FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, spawnParams);
		
		if (enemy)
		{
			enemy->SetActorHiddenInGame(true);
			enemy->SetActorEnableCollision(false);

			//敵の所有者をこのプールに設定
			enemy->SetOwner(this);

			//敵を待機中のプールに追加
			m_inactivePool.Add(enemy);
		}
	}
}

AEnemyChara* AEnemyPool::SpawnEnemyAtRandomLocation()
{
	//ナビゲーションシステムを取得
	UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!navSystem) { return nullptr; }
	
	//スポーン位置をランダムに生成
	FNavLocation navLocation;
	FVector center = GetActorLocation();

	//ポイントが見つかるまで繰り返す
	bool bFoundLocation = false;
	for (int32 i = 0; i < m_spawnMaxAttemps; ++i)
	{
		if (navSystem->GetRandomReachablePointInRadius(center, m_spawnRadius, navLocation))
		{
			bFoundLocation = true;
			break;
		}
	}

	//スポーン位置が見つからなかった場合は警告を出してnullを返す
	if (!bFoundLocation)
	{
		UE_LOG(LogTemp, Warning, TEXT("スポーン位置が見つかりませんでした。"));
		return nullptr;
	}

	//スポーン位置が見つかった場合は敵をスポーンする
	FRotator randomRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f);

	return SpawnEnemy(navLocation.Location, randomRotator);
}

//プールから敵を取り出す関数
AEnemyChara* AEnemyPool::SpawnEnemy(FVector _location, FRotator _rotation)
{
	AEnemyChara* enemy = nullptr;

	//待機中のプールに敵がいるか確認
	if (m_inactivePool.Num() > 0)
	{
		//待機中のプールから最後の敵を取得
		enemy = m_inactivePool.Last();

		//待機中のプールから最後の敵を削除
		m_inactivePool.RemoveAt(m_inactivePool.Num() - 1);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("エネミープールが空です。"));

		//敵のクラスが設定されているか確認
		FActorSpawnParameters spawnParams;
		spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;	//スポーン時の衝突処理を常にスポーンするに設定
		enemy = GetWorld()->SpawnActor<AEnemyChara>(m_pEnemyClass, _location, _rotation, spawnParams);

		if (enemy)
		{
			UE_LOG(LogTemp, Warning, TEXT("エネミープールから新しい敵をスポーンしました。"));
			enemy->SetOwner(this);                 
		}
	}
	if (enemy)
	{
		//敵をアクティブにする
		enemy->SetActorLocation(_location);
		enemy->SetActorRotation(_rotation);
		enemy->SetActorHiddenInGame(false);
		enemy->SetActorEnableCollision(true);

		//敵の体力をリセット
		enemy->RestetHP();

		//敵をアクティブなプールに追加
		m_activePool.Add(enemy);
	}
	
	return enemy;                                       
}

//使い終わったら敵をプールに返す関数
void AEnemyPool::ReturnEnemy(AEnemyChara* _enemy)
{
	if (!_enemy) { return; }
	
	//敵をアクティブなプールから削除
	m_activePool.Remove(_enemy);
	
	//敵を非アクティブにする
	_enemy->SetActorHiddenInGame(true);

	//敵のコリジョンを無効にする
	_enemy->SetActorEnableCollision(false);
		
	//敵を待機中のプールに追加
	m_inactivePool.Add(_enemy);

	UE_LOG(LogTemp, Verbose, TEXT("敵をプールに返しました。アクティブな敵の数: %d, 待機中の敵の数: %d"), m_activePool.Num(), m_inactivePool.Num());
}
