#include "Enemy/EnemyPool/WaveManager.h"
#include "Enemy/EnemyPool/EnemyPool.h"
#include "Enemy/EnemyChara.h"

// Sets default values
AWaveManager::AWaveManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AWaveManager::BeginPlay()
{
	Super::BeginPlay();
	
	//エネミープールがなければ処理しない
	if (!m_pEnemyPool) { return; }

	//ウェブを開始
	StartWave();
}

void AWaveManager::OnEnemyEliminatedCallback(AEnemyChara* _eliminatedEnemy)
{
	if (!_eliminatedEnemy || !m_pEnemyPool) { return; }

	_eliminatedEnemy->m_onEnemyEliminated.RemoveDynamic(this, &AWaveManager::OnEnemyEliminatedCallback);

	m_pEnemyPool->ReturnEnemy(_eliminatedEnemy);

	m_remainingEnemiesWave--;

	UE_LOG(LogTemp, Log, TEXT("Enemy Eliminated. Remaining: %d"), m_remainingEnemiesWave);

	if (m_remainingEnemiesWave <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("WaveCleared!"));
	}
}

void AWaveManager::StartWave()
{
	m_remainingEnemiesWave = m_enemiesPerWave;

	for (int32 i = 0; i < m_remainingEnemiesWave; i++)
	{
		AEnemyChara* enemy = m_pEnemyPool->SpawnEnemyAtRandomLocation();

		if (!enemy) { return; }
		enemy->m_onEnemyEliminated.RemoveDynamic(this, &AWaveManager::OnEnemyEliminatedCallback);
		enemy->m_onEnemyEliminated.AddDynamic(this, &AWaveManager::OnEnemyEliminatedCallback);
	}
}



