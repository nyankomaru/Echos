#include "Enemy/EnemyChara.h"
#include "Enemy/AIController/EnemyAIController.h"
#include "Enemy/EnemyPool/EnemyPool.h"
#include "Perception/PawnSensingComponent.h"
#include "Enemy/EnemyPool/WaveManager.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AEnemyChara::AEnemyChara() : m_maxHP(100.f), m_currentHP(0.0f), m_attackRange(200.f), m_damage(20.f), m_cooldownTimer(2.f), m_moveSpeed(300.f)
, m_targetActor(nullptr), m_behaviorTree(nullptr), m_pawnSensing(nullptr), m_currentState(EEnemyState::Idle)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//初期ステートはIdle
	m_currentState = EEnemyState::Idle;

	//PawnSensingComponentの作成
	m_pawnSensing = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensing"));

	//視野角と半径
	m_pawnSensing->SightRadius = 2000.f;
	m_pawnSensing->SetPeripheralVisionAngle(80.f);

	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

// Called when the game starts or when spawned
void AEnemyChara::BeginPlay()
{
	Super::BeginPlay();
	
	//OnSeePawnイベントに関数をバインド
	if (m_pawnSensing)
	{
		m_pawnSensing->OnSeePawn.AddDynamic(this, &AEnemyChara::OnSeePawn);
	}

	//体力を最大値に設定
	m_currentHP = m_maxHP;

	//移動速度を設定
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = m_moveSpeed;
	}
}

// Called every frame
void AEnemyChara::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

//ダメージを受ける関数
float AEnemyChara::TakeDamage(float _damageAmount, FDamageEvent const& _damageEvent, AController* _eventInstigator, AActor* _damageCauser)
{
	//ダメージ処理を行う前に、親クラスのTakeDamage関数を呼び出す
	Super::TakeDamage(_damageAmount, _damageEvent, _eventInstigator, _damageCauser);

	//体力を減らす
	m_currentHP = FMath::Clamp(m_currentHP - _damageAmount, 0, m_maxHP);

	//体力が0以下になったら死亡処理を行う
	if (m_currentHP <= 0.f)
	{
		Die();
	}

	//ダメージ量を返す
	return _damageAmount;
}

//攻撃関数
void AEnemyChara::Attack()
{
	//ターゲットがいない場合は攻撃しない
	if (!m_targetActor) { return; }

	//攻撃処理を行う（ここではダメージを与えるだけの簡単な処理）
	GetWorld()->GetTimerManager().SetTimer(m_attackTimerHandle, this, &AEnemyChara::ResetTimer, m_cooldownTimer, false);
}

//タイマーをリセットする関数
void AEnemyChara::ResetTimer()
{
	//攻撃後のクールダウンが終わったら、ステートをIdleに戻す
	m_currentState = EEnemyState::Idle;
	
	//攻撃タイマーをクリア
	GetWorld()->GetTimerManager().ClearTimer(m_attackTimerHandle);
}

//死亡処理関数
void AEnemyChara::Die()
{
	//現在のステートを死亡にする
	m_currentState = EEnemyState::Dead;

	//攻撃タイマーをクリア
	GetWorld()->GetTimerManager().ClearTimer(m_attackTimerHandle);
	
	//敵が倒されたことを通知
	if(AWaveManager* waveManager = Cast<AWaveManager>(GetOwner()))
	{
		waveManager->NotifyEnemyDefeated(this);
	}

	//プールへ戻す
	AEnemyPool* enemyPool = Cast<AEnemyPool>(GetOwner());
	if (enemyPool)
	{
		//プールに戻す
		enemyPool->ReturnEnemy(this);
	}
	else
	{
		Destroy();
	}
}

void AEnemyChara::OnSeePawn(APawn* _pawn)
{
	//ヌルだったら処理しない
	if (!_pawn) { return; }

	m_targetActor = _pawn;

	//プレイヤーを追尾する
	m_currentState = EEnemyState::Chase;
	
	//コントローラーを取得
	AEnemyAIController* aiCtrl = Cast<AEnemyAIController>(GetController());

	//コントローラーがNULLだったら処理しない
	if (!aiCtrl) { return; }

	aiCtrl->SetTargetActor(_pawn);
}

