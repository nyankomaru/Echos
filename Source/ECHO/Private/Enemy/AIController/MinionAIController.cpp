#include "Enemy/AIController/MinionAIController.h"
#include "Enemy/Minion/MinionChara.h"

#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AIPerceptionComponent.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

#include "Navigation/PathFollowingComponent.h"

//ブラックボードのキーの名前を定数として定義
namespace MinionBBKey
{
	static const FName TargetActor(TEXT("TargetActor"));
	static const FName PatrolLocation(TEXT("PatrolLocation"));
	static const FName bIsInAttackRange(TEXT("bIsInAttackRange"));
	static const FName bIsDead(TEXT("bIsDead"));
}

//コンストラクタ
AMinionAIController::AMinionAIController() : m_sightRadius(2000.f), m_loseSightRadius(2500.f), m_peripheralVisionAngle(80.f)
, m_sightAge(5.f), m_hearingRange(1500.f), m_attackRangeCheckInterval(0.1f), m_attackRangeTimer(0.f)
{
	PrimaryActorTick.bCanEverTick = true;
}

//ゲーム開発開始時の初期化関数
void AMinionAIController::BeginPlay()
{
	//親クラスのBeginPlay関数を呼び出す
	Super::BeginPlay();
	
	//知覚コンポーネントの取得
	if (m_pAIPerceptionComp)
	{
		//知覚の更新イベントに関数をバインド
		m_pAIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AMinionAIController::OnTargetPerceptionUpdated);
	}
	
	//視野の設定
	SetupminionSight();
}

//Pawnをコントロールする際の初期化関数
void AMinionAIController::OnPossess(APawn* _inPawn)
{
	//親クラスのOnPossess関数を呼び出す
	Super::OnPossess(_inPawn);

	//ポーズしているキャラクタへの参照を保存
	m_minionChara = Cast<AMinionChara>(_inPawn);

	//キャラクタがMinionCharaでない場合は警告を出して終了
	if (!m_minionChara.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("AMinionAIController：AMinionChara以外のPawnがポーズされました"));
		return;
	}

	//ブラックボードの初期化
	if (UBlackboardComponent* blackboardComp = GetBlackboardComponent())
	{
		//ターゲットアクターをnullに設定
		blackboardComp->SetValueAsObject(MinionBBKey::TargetActor, nullptr);

		//パトロール地点をゼロベクトルに設定
		blackboardComp->SetValueAsVector(MinionBBKey::PatrolLocation, FVector::ZeroVector);

		//攻撃範囲内フラグをfalseに設定
		blackboardComp->SetValueAsBool(MinionBBKey::bIsInAttackRange, false);
		
		//死亡フラグをfalseに設定
		blackboardComp->SetValueAsBool(MinionBBKey::bIsDead, false);
	}
}

//ターゲットの知覚が更新されたときに呼び出される関数
void AMinionAIController::OnUnPossess()
{
	//親クラスのOnUnPossess関数を呼び出す
	Super::OnUnPossess();

	//ポーズしているキャラクタへの参照をクリア
	m_minionChara.Reset();
}

//毎フレーム呼び出される関数
void AMinionAIController::Tick(float _deltaTime)
{
	//親クラスのTick関数を呼び出す
	Super::Tick(_deltaTime);
	
	//攻撃範囲チェックのタイマーを更新
	m_attackRangeTimer += _deltaTime;
	
	if (m_attackRangeTimer >= m_attackRangeCheckInterval)
	{
		//攻撃範囲内にいるか判定してブラックボードを更新
		UpdateAttackRangeBlackboard();
		m_attackRangeTimer = 0.f;
	}
}

//ターゲットの知覚が更新されたときに呼び出される関数
void AMinionAIController::OnMinionPerceptionUpdated(AActor* _actor, FAIStimulus _stimulus)
{
	//ターゲットがnullの場合は何もしない
	if (!_actor) { return; }

	//ターゲットがプレイヤーでない場合は何もしない
	if (!_actor->ActorHasTag(TEXT("Player"))) { return; }

	//ターゲットが知覚された場合はターゲットをブラックボードに設定
	if (_stimulus.WasSuccessfullySensed())
	{
		SetTargetActor(_actor);
	}
	else
	{
		//ターゲットが知覚されなくなった場合はターゲットをクリア
		ClearTarget();
	}
}

//ターゲットを設定する関数
void AMinionAIController::SetTargetActor(AActor* _newTarget)
{
	Super::SetTargetActor(_newTarget);

	if (m_minionChara.IsValid())
	{
		m_minionChara->SetTargetActor(_newTarget);
	}
}

//ターゲットをクリアする関数
void AMinionAIController::ClearTarget()
{
	//親クラスのClearTarget関数を呼び出す
	Super::ClearTarget();

	//ポーズしているキャラクタが有効な場合はターゲットをクリア
	if (m_minionChara.IsValid())
	{
		m_minionChara->SetTargetActor(nullptr);
	}

	//ブラックボードをクリア
	if (UBlackboardComponent* blackboardComp = GetBlackboardComponent())
	{
		//ブラックボードのターゲットアクターをnullに設定
		blackboardComp->SetValueAsObject(MinionBBKey::TargetActor, nullptr);

		//攻撃範囲内フラグをfalseに設定
		blackboardComp->SetValueAsBool(MinionBBKey::bIsInAttackRange, false);
	}
}

//死んだときに呼び出される関数
void AMinionAIController::NotifyDead()
{
	//ビヘイビアツリーコンポーネントを取得
	UBehaviorTreeComponent* behaviorTreeComp = FindComponentByClass<UBehaviorTreeComponent>();
	if (behaviorTreeComp)
	{
		//ビヘイビアツリーを停止
		behaviorTreeComp->StopTree(EBTStopMode::Safe);
	}

	//移動を停止する
	StopMovement();

	//ブラックボードの死亡フラグをtrueに設定
	if (UBlackboardComponent* blackboardComp = GetBlackboardComponent())
	{
		blackboardComp->SetValueAsBool(MinionBBKey::bIsDead, true);
	}

	UE_LOG(LogTemp, Log, TEXT("AMinionAIController：死亡通知を受け取りました"));
}

void AMinionAIController::UpdateAttackRangeBlackboard()
{
	//ブラックボードコンポーネントを取得
	UBlackboardComponent* blackboardComp = GetBlackboardComponent();
	if (!blackboardComp) { return; }
	
	//ターゲットアクターをブラックボードから取得
	AActor* targetActor = Cast<AActor>(blackboardComp->GetValueAsObject(MinionBBKey::TargetActor));
	
	//ターゲットアクターがnullの場合や、ポーズしているキャラクタが無効な場合は攻撃範囲内フラグをfalseに設定して終了
	if (!targetActor || !m_minionChara.IsValid()) 
	{ 
		blackboardComp->SetValueAsBool(MinionBBKey::bIsInAttackRange, false);
		return;
	}

	//ターゲットアクターとポーズしているキャラクタの距離を計算
	float dist = FVector::Dist(m_minionChara->GetActorLocation(), targetActor->GetActorLocation());
	
	//距離が攻撃範囲内か判定してブラックボードを更新
	bool bInRange = (dist <= m_minionChara->GetAttackRange());

	//攻撃範囲内フラグをブラックボードに設定
	blackboardComp->SetValueAsBool(MinionBBKey::bIsInAttackRange, bInRange);
}

//視野パラメータをスクラッパー用設定
void AMinionAIController::SetupminionSight()
{
	//視覚の設定がない場合は終了
	if (!m_pSightConfig) { return; }

	//視覚のパラメータを設定
	m_pSightConfig->SightRadius = m_sightRadius;
	m_pSightConfig->LoseSightRadius = m_loseSightRadius;
	m_pSightConfig->PeripheralVisionAngleDegrees = m_peripheralVisionAngle;
	m_pSightConfig->SetMaxAge(m_sightAge);

	//視覚の刺激の検出対象を設定
	m_pSightConfig->DetectionByAffiliation.bDetectEnemies = true;
	m_pSightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	m_pSightConfig->DetectionByAffiliation.bDetectFriendlies = true;


	if (m_pHearingConfig)
	{
		m_pHearingConfig->HearingRange = m_hearingRange;
	}

	//知覚コンポーネントがない場合は警告を出して終了
	if (!m_pAIPerceptionComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("AMinionAIController：知覚コンポーネントが見つかりませんでした"));
		return;
	}
	
	//視覚の設定を知覚コンポーネントに適用
	m_pAIPerceptionComp->ConfigureSense(*m_pSightConfig);

	//聴覚の設定を知覚コンポーネントに適用
	if (m_pHearingConfig) 
	{
		m_pAIPerceptionComp->ConfigureSense(*m_pHearingConfig);
	}
	
	//知覚コンポーネントに変更を通知して更新
	m_pAIPerceptionComp->RequestStimuliListenerUpdate();
}