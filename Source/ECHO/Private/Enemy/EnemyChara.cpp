#include "Enemy/EnemyChara.h"
#include "Enemy/AIController/EnemyAIController.h"
#include "Enemy/EnemyPool/EnemyPool.h"
#include "Perception/PawnSensingComponent.h"
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

float AEnemyChara::TakeDamage(float _damageAmount, FDamageEvent const& _damageEvent, AController* _eventInstigator, AActor* _damageCauser)
{
	Super::TakeDamage(_damageAmount, _damageEvent, _eventInstigator, _damageCauser);

	m_currentHP = FMath::Clamp(m_currentHP - _damageAmount, 0, m_maxHP);

	if (m_currentHP <= 0.f)
	{
		Die();
	}

	return _damageAmount;
}

void AEnemyChara::Attack()
{
	if (!m_targetActor) { return; }

	GetWorld()->GetTimerManager().SetTimer(m_attackTimerHandle, this, &AEnemyChara::ResetTimer, m_cooldownTimer, false);
}

void AEnemyChara::ResetTimer()
{
	m_currentState = EEnemyState::Idle;
	GetWorld()->GetTimerManager().ClearTimer(m_attackTimerHandle);
}

void AEnemyChara::Die()
{
	//現在のステートを死亡にする
	m_currentState = EEnemyState::Dead;

	//攻撃タイマーをクリア
	GetWorld()->GetTimerManager().ClearTimer(m_attackTimerHandle);
	
	//イベント通知
	m_onEnemyEliminated.Broadcast(this);

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

