#include "Enemy/MidBoss/MidBoss.h"
#include "Enemy/EnemyPool/WaveManager.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"

//コンストラクタ
AMidBoss::AMidBoss()
{
	//Tick関数を有効にする
	PrimaryActorTick.bCanEverTick = true;

	//中間ボスのステータスを設定
	m_maxHP = 300.f;
	m_damage = 30.f;
	m_attackRange = 200.f;
	m_cooldownTimer = 2.f;
	m_moveSpeed = 350.f;

	//AIコントローラーを設定
	AIControllerClass = AAIController::StaticClass();	//中間ボスのAIコントローラーをまだないので今後作る予定
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;		//中間ボスがスポーンしたときに自動でAIコントローラーを持つようにする
}

//ゲーム開始時に呼ばれる関数
void AMidBoss::BeginPlay()
{
	//基底クラスのBeginPlay関数を呼び出す
	Super::BeginPlay();

	//初期フェーズの設定
	m_currentPhase = 1;	
}

//毎フレーム呼ばれる関数
void AMidBoss::Tick(float _deltaTime)
{
	//基底クラスのTick関数を呼び出す
	Super::Tick(_deltaTime);

	//フェーズの更新
	UpdatePhase();
}

//攻撃関数
void AMidBoss::Attack()
{
	//ターゲットがいない場合は攻撃しない
	if (!GetCurrentTarget()) { return; }

	//死亡している場合は攻撃しない
	if (GetCurrentState() == EEnemyState::Dead) { return; }

	//攻撃アニメーションを再生
	if (m_attackMontage)
	{
		PlayAnimMontage(m_attackMontage, 1.0f);
	}
	else
	{
		PerformSlashAttack();	//攻撃アニメーションがない場合は直接攻撃関数を呼び出す

		//フェーズ2以降で突進攻撃のクールダウンがない場合は突進攻撃を行う
		if (m_currentPhase >= 2 && !m_bChargeCooldown)
		{
			PerformCharageAttack();
		}
	}

	//40%の確率でワイヤートラップを設置する
	if (FMath::RandRange(0, 100) < 40)	
	{
		PlaceWireTrap();
	}
}

//死亡関数
void AMidBoss::Die()
{
	//突進攻撃のクールダウンタイマーをクリアする)
	GetWorld()->GetTimerManager().ClearTimer(m_chargeCooldownTimer);	

	//WaveManagerに中間ボスが倒されたことを通知する
	if (AWaveManager* waveManager = Cast<AWaveManager>(GetOwner()))
	{
		waveManager->NotifyEnemyDefeated(this);	
	}

	//基底クラスのDie関数を呼び出す
	Super::Die();
}

//通常の近接攻撃
void AMidBoss::PerformSlashAttack()
{
	//ターゲットがいない場合は攻撃しない
	if (!GetCurrentTarget()) { return; }

	//現在の位置と攻撃の終点を計算
	FVector start = GetActorLocation();
	FVector end = start + GetActorForwardVector() * m_slashLength;

	//攻撃範囲内の敵を検出
	TArray<FHitResult> hitResults;
	FCollisionShape capsule = FCollisionShape::MakeCapsule(m_slashRadius, m_slashLength / 2.f);
	
	//クエリパラメータを設定
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(this);	//自身を無視する

	GetWorld()->SweepMultiByChannel(hitResults, start, end, FQuat::Identity, ECC_Pawn, capsule, queryParams);

#if WITH_EDITOR
	//攻撃範囲をデバッグ表示
	DrawDebugCapsule(GetWorld(), (start + end) / 2.f, m_slashLength / 2.f, m_slashRadius, FQuat::Identity, FColor::Orange, false, 1.f);
#endif

	//ヒットした敵にダメージを与える
	for (const auto& hit : hitResults)
	{
		AActor* hitActor = hit.GetActor();
		if (hitActor && hitActor != this)
		{
			UGameplayStatics::ApplyDamage(hitActor, m_damage, GetController(), this, nullptr);
		}
	}
}

//ワイヤートラップを設置する関数
void AMidBoss::PlaceWireTrap()
{
	//足元の位置にワイヤートラップを設置する
	FVector tarpPosition = GetActorLocation();

	//ワイヤートラップの範囲内の敵を検出
	TArray<FOverlapResult> overlapResults;
	FCollisionShape sphere = FCollisionShape::MakeSphere(m_wireRadius);

	//クエリパラメータを設定
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(this);	//自身を無視する
	GetWorld()->OverlapMultiByChannel(overlapResults, tarpPosition, FQuat::Identity, ECC_Pawn, sphere, queryParams);

#if WITH_EDITOR
	//ワイヤートラップの範囲をデバッグ表示
	DrawDebugSphere(GetWorld(), tarpPosition, m_wireRadius, 12, FColor::Yellow, false, 1.f);
#endif
	
	//範囲内の敵にダメージを与える
	for (const auto& overlap : overlapResults)
	{
		AActor* overlapActor = overlap.GetActor();
		if (overlapActor && overlapActor != this)
		{
			UGameplayStatics::ApplyDamage(overlapActor, m_wireDamage, GetController(), this, nullptr);
		}
	}
}

//突進攻撃の関数
void AMidBoss::PerformCharageAttack()
{
	if (!GetCurrentTarget() || m_bChargeCooldown) { return; }

	//ターゲットの方向を計算
	FVector toTarget = (GetCurrentTarget()->GetActorLocation() - GetActorLocation()).GetSafeNormal();

	//突進ベクトルを設定
	float chargeSpeed = m_chargeDistance / .3f;	

	//キャラクターを突進させる
	LaunchCharacter(toTarget * chargeSpeed, true, false);

	//突進後の判定
	FTimerHandle hitHandle;
	GetWorld()->GetTimerManager().SetTimer(hitHandle, [this]()
		{
			//突進攻撃の範囲内の敵を検出
			TArray<FHitResult> hitResults;
			FCollisionShape capsule = FCollisionShape::MakeCapsule(m_slashRadius, m_chargeDistance / 2.f);

			//クエリパラメータを設定
			FCollisionQueryParams queryParams;
			queryParams.AddIgnoredActor(this);	//自身を無視する
			
			//突進攻撃の範囲を計算
			FVector start = GetActorLocation();
			FVector end = start + GetActorForwardVector() * m_chargeDistance;
			GetWorld()->SweepMultiByChannel(hitResults, start, end, FQuat::Identity, ECC_Pawn, capsule, queryParams);

			for (auto& hit : hitResults)
			{
				AActor* hitActor = hit.GetActor();
				if (hitActor && hitActor->ActorHasTag("Player"))
				{
					//突進攻撃のダメージを計算
					float chargeDamage = m_damage * m_chargeDamageMultiplier;
					UGameplayStatics::ApplyDamage(hitActor, chargeDamage, GetController(), this, nullptr);
				}
			}

		}, 0.3f, false);

	m_bChargeCooldown = true;	//突進攻撃のクールダウンを開始

	//5秒後にクールダウンを終了
	GetWorld()->GetTimerManager().SetTimer(m_chargeCooldownTimer, [this]()
		{
			m_bChargeCooldown = false;	//突進攻撃のクールダウンを終了
		}, 5.f, false);	
}

//Phaseの更新関数
void AMidBoss::UpdatePhase()
{
	//死亡している場合はフェーズの更新を行わない
	if (GetCurrentState() == EEnemyState::Dead) { return; }

	//体力の割合を計算
	float hpPercentage = (m_maxHP > 0) ? (GetCurrentHP() / m_maxHP * 100.f) : 0.f;

	//新しいフェーズを決定
	int32 newPhase = 1;
	if (hpPercentage <= m_phaseThreeThreshold)
	{
		newPhase = 3;
	}
	else if (hpPercentage <= m_phaseTwoThreshold)
	{
		newPhase = 2;
	}

	//フェーズが変わった場合は更新してイベントを呼び出す
	if (newPhase != m_currentPhase)
	{
		OnPhaseChanged(m_currentPhase);
	}
}

//フェーズが変わったときの処理
void AMidBoss::OnPhaseChanged(int32 _newPhase)
{
	//フェーズが変わったときの処理をここに追加
	m_currentPhase = _newPhase;

	//フェーズ3に移行したときの処理
	if (m_currentPhase == 3)
	{
		m_moveSpeed = m_phaseThreeMoveSpeed;	//フェーズ3では移動速度を上げる
		m_cooldownTimer = m_phaseThreeCooldown;	//フェーズ3では攻撃のクールダウンを短くする

		//キャラクターの移動速度を更新
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->MaxWalkSpeed = m_moveSpeed;	
		}
	}
}
