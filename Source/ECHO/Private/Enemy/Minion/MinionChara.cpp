#include "Enemy/Minion/MinionChara.h"
#include "Enemy/EnemyChara.h"
#include "Enemy/EnemyPool/EnemyPool.h"
#include "Enemy/AIController/MinionAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

AMinionChara::AMinionChara() : m_normalSpeed(500.f), m_rushSpeed(900.f), m_rushTriggerDistance(600.f)
, m_attackTraceRadius(60.f), m_attackTraceLength(120.f), m_isRushing(false)
{
	PrimaryActorTick.bCanEverTick = true;

	//プロパティの初期化
	m_maxHP = 80.f;
	m_damage = 15.f;
	m_attackRange = 150.f;
	m_cooldownTimer = 1.5f;
	m_moveSpeed = m_normalSpeed;

	//AIコントローラーのクラスを設定し、スポーン時に自動でAIを制御するように設定
	AIControllerClass = AMinionAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

//ゲーム開始時またはスポーン時に呼び出される関数
void AMinionChara::BeginPlay()
{
	//基底クラスのBeginPlay関数を呼び出す
	Super::BeginPlay();

	//通常の移動速度を設定
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = m_normalSpeed;
	}
}

//毎フレーム呼び出される関数
void AMinionChara::Tick(float _deltaTime)
{
	//基底クラスのTick関数を呼び出す
	Super::Tick(_deltaTime);

	//突進のビヘイビアーを更新
	UpdateRushBehavior();
}

void AMinionChara::UpdateRushBehavior()
{
	//Dead状態のときは何もしない
	if (GetCurrentState() == EEnemyState::Dead) { return; }

	//ターゲットがいるか確認
	if (!GetCurrentTarget()) { return; }

	//ターゲットとの距離を計算
	float dist = FVector::Dist(GetActorLocation(), GetCurrentTarget()->GetActorLocation());

	//ターゲットが突進トリガー距離内にいて、まだ突進していない場合は突進を開始
	if (dist <= m_rushTriggerDistance && !m_isRushing)
	{
		if (dist > m_attackRange)
		{
			StartRush();
		}
	}
	else if (dist > m_rushTriggerDistance && m_isRushing)
	{
		StopRush();
	}
	
}

//攻撃関数
void AMinionChara::Attack()
{
	//ターゲットなしやDead状態のときは攻撃しない
	if (!GetCurrentTarget()) { return; }
	if (GetCurrentState() == EEnemyState::Dead) { return; }

	//突進は一度止める
	StopRush();

	if (m_attackMontage)
	{
		//攻撃アニメーションを再生
		PlayAnimMontage(m_attackMontage);
	}
	else
	{
		//攻撃ヒット判定
		PerformAttackTrace();
	}

	//攻撃クールダウンタイマーをセット
	Super::Attack();
}

//突進開始関数
void AMinionChara::StartRush()
{
	//突進フラグを立て、移動速度を突進速度に変更
	m_isRushing = true;
	m_moveSpeed = m_rushSpeed;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = m_moveSpeed;
	}
}

//突進終了関数
void AMinionChara::StopRush()
{
	//突進フラグを下げ、移動速度を通常速度に変更
	m_isRushing = false;
	m_moveSpeed = m_normalSpeed;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = m_moveSpeed;
	}
}

//攻撃ヒット判定関数
void AMinionChara::PerformAttackTrace()
{
	//ターゲットなしやDead状態のときは攻撃しない
	if (!GetCurrentTarget()) { return; }

	//攻撃の開始点と終了点を計算
	FVector start = GetActorLocation();
	FVector end = start + GetActorForwardVector() * m_attackTraceLength;

	//ヒット結果を格納する配列と、当たり判定の形状を設定
	TArray<FHitResult> hitResults;
	FCollisionShape collisionShape = FCollisionShape::MakeCapsule(m_attackTraceRadius, m_attackTraceLength* 0.5);

	//クエリパラメータを設定
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(this); //自分自身を無視

	//攻撃のヒット判定を実行
	bool isHit = GetWorld()->SweepMultiByChannel(hitResults, start, end, FQuat::Identity, ECC_Pawn, collisionShape, queryParams);

#if WITH_EDITOR
	//デバッグ用に攻撃の当たり判定を描画
	FColor debugColor = isHit ? FColor::Red : FColor::Green;
	DrawDebugCapsule(GetWorld(), (start + end) * 0.5f, m_attackTraceLength * 0.5f, m_attackTraceRadius, FQuat::Identity, debugColor, false, 1.f);
#endif
	
	for (FHitResult& hit : hitResults)
	{
		AActor* hitActor = hit.GetActor();
		if (!hitActor || hitActor == this) { continue; }

		if (hitActor == GetCurrentTarget())
		{
			//FDamageEvent damageEvent;

			//ターゲットにダメージを与える
			UGameplayStatics::ApplyDamage(hitActor, m_damage, GetController(), this, nullptr);
		}
	}
}


void AMinionChara::Die()
{
	//突進中なら止める
	if (m_isRushing)
	{
		StopRush();
	}

	//基底クラスのDie関数を呼び出す
	Super::Die();
}
