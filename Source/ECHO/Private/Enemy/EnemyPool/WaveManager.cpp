#include "Enemy/EnemyPool/WaveManager.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/EnemyPool/EnemyPool.h"
#include "Kismet/GameplayStatics.h"
#include "Enemy/EnemyChara.h"
#include "NavigationSystem.h"
#include "AIController.h"

// Sets default values
AWaveManager::AWaveManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AWaveManager::BeginPlay()
{
	//基底クラスのBeginPlayを呼び出す
	Super::BeginPlay();
}

void AWaveManager::Tick(float _deltaTime)
{
	//基底クラスのTickを呼び出す
	Super::Tick(_deltaTime);
	
	//ウェーブが開始されていない場合は、役割の更新を行わない
	if (m_bWaveStarted) { return; }

	//役割の更新間隔を加算
	m_roleUpdateInterval += _deltaTime;
	
	if(m_roleUpdateInterval >= m_roleUpdateInterval)
	{
		//役割の更新
		UpdateAttackerRoles();
		
		//役割の更新間隔をリセット
		m_roleUpdateInterval = 0.f;
	}
}

//Waveを開始する関数
void AWaveManager::StartWave()
{
	if(!m_pEnemyPool) 	
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyPool を割り当てていません。"));
		return;
	}

	m_aliveEnemies.Empty();			//生存している敵のリストをクリア
	m_activeAttackers.Empty();		//攻撃役のリストをクリア
	m_defeatedEnemyCount = 0;		//倒された敵の数をリセット
	m_bMidBossSpawned = false;		//中間ボススポーンフラグをリセット
	m_bFinalBossSpawned = false;	//ラストボススポーンフラグをリセット

	//エネミープールから敵をスポーン
	for (int32 i = 0; i < m_enemiesPerWave; ++i)
	{
		AEnemyChara* SpawnedEnemy = m_pEnemyPool->SpawnEnemyAtRandomLocation();
		if (SpawnedEnemy)
		{
			m_aliveEnemies.Add(SpawnedEnemy);
		}
	}

	m_initialEnemyCount = m_aliveEnemies.Num();	//ウェーブ開始時の敵の数を保存
	m_bWaveStarted = true;						//ウェーブ開始フラグを立てる

	//役割の更新
	UpdateAttackerRoles();

	UE_LOG(LogTemp, Log, TEXT("AWaveManager: Wave 開始。敵 %d 体をスポーンしました。"), m_initialEnemyCount);
}

//敵撃破通知
void AWaveManager::NotifyEnemyDefeated(AEnemyChara* _defeatedEnemy)
{
	//ウェーブが開始されていない場合は、処理を行わない
	if (!m_bWaveStarted) { return; }	

	//生存している敵のリストから倒された敵を削除
	m_aliveEnemies.Remove(_defeatedEnemy);	

	//攻撃役のリストから倒された敵を削除
	m_activeAttackers.Remove(_defeatedEnemy);	

	//倒された敵の数をカウント
	m_defeatedEnemyCount++;					
	UE_LOG(LogTemp, Log, TEXT("AWaveManager: 敵が倒されました。残りの敵の数: %d"), m_aliveEnemies.Num());
	
	//アタッカーの再割り当て
	UpdateAttackerRoles();

	//中間ボススポーンの判定
	if (!m_bMidBossSpawned && m_initialEnemyCount > 0)
	{
		//倒された敵の割合を計算
		float defeatedPercent = (float)m_defeatedEnemyCount / (float)m_initialEnemyCount * 100.f;

		//倒された敵の割合が中間ボスポーンの割合を超えている場合、中間ボスをスポーン
		if (defeatedPercent >= m_midBossSpawnPercent) { SpawnMidBoss(); }
	}

	//ラストボススポーンの判定
	if (m_aliveEnemies.Num() == 0 && !m_bFinalBossSpawned)
	{
		//中間ボスも倒されてるかを確認
		bool midBossAlive = (m_midBoss && m_midBoss->GetCurrentState() != EEnemyState::Dead);

		if (!midBossAlive) { SpawnFinalBoss(); }
	}
}

//アタッカー役割更新
void AWaveManager::UpdateAttackerRoles()
{
	//生存している敵がいない場合は、役割の更新を行わない
	if (m_aliveEnemies.Num() == 0) { return; }	
	
	//プレイヤーを探す
	APawn* player = FindPlayer();	
	if(!player) { return; }

	//死んでる敵を除外
	m_aliveEnemies.RemoveAll([](AEnemyChara* enemy) 
		{ 
			return !enemy || enemy->GetCurrentState() == EEnemyState::Dead; 
		});
	m_activeAttackers.RemoveAll([](AEnemyChara* enemy) 
		{ 
			return !enemy || enemy->GetCurrentState() == EEnemyState::Dead; 
		});

	//アタッカーの数を調整
	for(AEnemyChara* enemy : m_aliveEnemies)
	{
		//現在のアタッカーの数が、1ウェーブあたりのアタッカーの数より少ない場合、攻撃役に割り当てる
		if (m_activeAttackers.Num() < m_attackersPerWave && !m_activeAttackers.Contains(enemy))
		{
			m_activeAttackers.Add(enemy);
			SetAttackerRole(enemy, true);
		}
		else if (m_activeAttackers.Num() > m_attackersPerWave && m_activeAttackers.Contains(enemy))
		{
			m_activeAttackers.Remove(enemy);
			SetAttackerRole(enemy, false);
		}
	}

	//残りの敵を包囲役に割り当てる
	int32 encircleIndex = 0;
	int32 totalEncirclers = m_aliveEnemies.Num() - m_activeAttackers.Num();

	//アタッカー役でない敵を包囲役に割り当てる
	for(AEnemyChara* enemy : m_aliveEnemies)
	{
		//攻撃役でない敵を包囲役に割り当てる
		if (!m_activeAttackers.Contains(enemy)){ continue; }

		//包囲役の位置を割り当てる
		AssignEncirclePositions(enemy, encircleIndex, totalEncirclers);

		//包囲役の役割を割り当てる
		SetAttackerRole(enemy, false);

		//包囲役の役割を割り当てる
		encircleIndex++;
	}
}

//包囲役の位置を割り当てる関数
void AWaveManager::AssignEncirclePositions(AEnemyChara* _enemy, int32 _index, int32 _total)
{
	//プレイヤーを探す
	APawn* player = FindPlayer();

	//プレイヤーが見つからない場合は、処理を行わない
	if (!player || !_enemy) { return; }

	//包囲役の位置を計算
	FVector targetPos = CalculateEncirclePosition(player->GetActorLocation(), _index, _total);
	
	//ナビゲーションシステムを取得
	UNavigationSystemV1* navSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!navSys) { return; }

	//計算した位置がナビゲーションメッシュ上にあるかを確認し、ない場合は最も近いナビゲーションメッシュ上の位置を取得
	FNavLocation navLoc;
	if (navSys->ProjectPointToNavigation(targetPos, navLoc, FVector(200.f, 200.f, 200.f)))
	{
		targetPos = navLoc.Location;
	}

	//敵に包囲役の役割を割り当てる
	SetEncircleRole(_enemy, targetPos);
}

//包囲役の位置を計算する関数
FVector AWaveManager::CalculateEncirclePosition(FVector _playerPos, int32 _index, int32 _total)
{
	//プレイヤーを中心に等間隔
	float angle = (_total > 0) ? (360.f / _total) * _index : 0.f;

	//角度をラジアンに変換
	float rad = FMath::DegreesToRadians(angle);

	//プレイヤーを中心に、指定した半径で円を描くように位置を返す
	return  _playerPos + FVector(FMath::Cos(rad) * m_encircleRadius, FMath::Sin(rad) * m_encircleRadius, 0.f);
}

//中間ボスのスポーン
void AWaveManager::SpawnMidBoss()
{
	//すでに中間ボスがスポーンされている場合は、処理を行わない
	if (m_bMidBossSpawned) { return; }

	//スポーンポイントと中間ボスクラスが設定されていない場合は、処理を行わない
	if (!m_midBossClass) { return; }

	//スポーン位置と回転を設定
	FVector spawnLoc = m_midBossSpawnPoint ? m_midBossSpawnPoint->GetActorLocation() : GetActorLocation() + FVector(500.f, 0.f, 0.f);
	FRotator spawnRot = m_midBossSpawnPoint ? m_midBossSpawnPoint->GetActorRotation() : FRotator::ZeroRotator;

	//スポーンパラメータを設定
	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	//中間ボスをスポーン
	m_midBoss = GetWorld()->SpawnActor<AEnemyChara>(m_midBossClass, spawnLoc, spawnRot, spawnParams);
	if (m_midBoss)
	{
		m_midBoss->SetOwner(this);
		m_aliveEnemies.Add(m_midBoss);
		m_bMidBossSpawned = true;
	}
}

//ラストボスのスポーン
void AWaveManager::SpawnFinalBoss()
{
	//すでにラストボスがスポーンされている場合は、処理を行わない
	if (m_bFinalBossSpawned) { return; }

	//スポーンポイントとラストボスクラスが設定されていない場合は、処理を行わない
	if (!m_finalBossClass) { return; }

	//スポーン位置と回転を設定
	FVector spawnLoc = m_finalBossSpawnPoint ? m_finalBossSpawnPoint->GetActorLocation() : GetActorLocation() + FVector(1000.f, 0.f, 0.f);
	FRotator spawnRot = m_finalBossSpawnPoint ? m_finalBossSpawnPoint->GetActorRotation() : FRotator::ZeroRotator;
	
	//スポーンパラメータを設定
	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	//ラストボスをスポーン
	AEnemyChara* finalBoss = GetWorld()->SpawnActor<AEnemyChara>(m_finalBossClass, spawnLoc, spawnRot, spawnParams);
	if (finalBoss)
	{
		m_bFinalBossSpawned = true;
	}
}

//現在生存している敵の数を返す関数
int32 AWaveManager::GetAliveEnemyCount() const
{
	return m_aliveEnemies.Num();
}

//プレイヤーを探して返す関数
APawn* AWaveManager::FindPlayer() const
{
	//プレイヤーを取得
	APawn* playerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	return playerPawn;
}

//敵に攻撃役か包囲役かの役割を設定する関数
void AWaveManager::SetAttackerRole(AEnemyChara* _enemy, bool _isAttacker)
{
	if (!_enemy) { return; }

	//AIコントローラーを取得
	AAIController* aiController = Cast<AAIController>(_enemy->GetController());
	if (!aiController) { return; }

	//ブラックボードコンポーネントを取得
	UBlackboardComponent* blackboardComp = aiController->GetBlackboardComponent();
	if (!blackboardComp) { return; }

	//ブラックボードのキーに基づいて、攻撃役か包囲役かを設定
	blackboardComp->SetValueAsBool(WaveBBKey::bIsActiveAttacker, _isAttacker);
}

//敵に包囲役の位置を設定する関数
void AWaveManager::SetEncircleRole(AEnemyChara* _enemy, const FVector& _encirclePosition)
{
	//敵が存在しない場合は、処理を行わない
	if (!_enemy) { return; }

	//AIコントローラーを取得
	AAIController* aiController = Cast<AAIController>(_enemy->GetController());
	if (!aiController) { return; }

	//ブラックボードコンポーネントを取得
	UBlackboardComponent* blackboardComp = aiController->GetBlackboardComponent();
	if (!blackboardComp) { return; }

	//ブラックボードのキーに基づいて、包囲役の位置を設定
	blackboardComp->SetValueAsVector(WaveBBKey::EncirclePosition, _encirclePosition);
}