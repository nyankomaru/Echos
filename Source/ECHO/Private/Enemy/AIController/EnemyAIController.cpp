#include "Enemy/AIController/EnemyAIController.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

#include "Enemy/EnemyChara.h"

//コンストラクタ
AEnemyAIController::AEnemyAIController() : m_pAIPerceptionComp(nullptr), m_pSightConfig(nullptr), m_pHearingConfig(nullptr)
{
	m_pAIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));

	//視覚
	m_pSightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	m_pSightConfig->SightRadius = 2000.f;
	m_pSightConfig->LoseSightRadius = 2500.f;
	m_pSightConfig->PeripheralVisionAngleDegrees = 90.f;
	m_pSightConfig->SetMaxAge(5.f);
	
	//聴覚
	m_pHearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	m_pHearingConfig->HearingRange = 2000.f;
	m_pHearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	m_pHearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	m_pHearingConfig->DetectionByAffiliation.bDetectNeutrals = true;

	//知覚コンポーネントに感覚の設定を追加
	m_pAIPerceptionComp->ConfigureSense(*m_pSightConfig);
	m_pAIPerceptionComp->SetDominantSense(m_pSightConfig->GetSenseImplementation());

	//ビヘイビアツリーとブラックボードのコンポーネントを作成
	m_pBehaviorTreeComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
	m_pBlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackbooardComponent"));
}

//ゲーム開発開始時の初期化関数
void AEnemyAIController::BeginPlay()
{
	//親クラスのBeginPlay関数を呼び出す
	Super::BeginPlay();

	//知覚コンポーネントの取得とイベントのバインド
	if (m_pAIPerceptionComp)
	{
		m_pAIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnTargetPerceptionUpdated);
	}
}

//Pawnをコントロールする際の初期化関数
void AEnemyAIController::OnPossess(APawn* _inPawn)
{
	//親クラスのOnPossess関数を呼び出す
	Super::OnPossess(_inPawn);

	//ポーズしているキャラクタへの参照を保存
	AEnemyChara* enemyChara = Cast<AEnemyChara>(_inPawn);

	//キャラクタがEnemyCharaでない場合は警告を出して終了
	if (!enemyChara) { return; }

	//ビヘイビアツリーがある場合は実行
	if (enemyChara->GetBehaviorTree())
	{
		RunBehaviorTree(enemyChara->GetBehaviorTree());

		m_pBlackboardComp = GetBlackboardComponent();
	}
}


//ターゲットの知覚が更新されたときに呼び出される関数
void AEnemyAIController::OnTargetPerceptionUpdated(AActor* _actor, FAIStimulus _stimulus)
{
	//ターゲットがnullの場合は何もしない
	if (!_actor) { return; }

	//ターゲットがプレイヤーでない場合は何もしない
	if (!_actor->ActorHasTag(TEXT("Player"))) { return; }

	//ターゲットが知覚された場合はターゲットをブラックボードに設定、知覚されなくなった場合はターゲットをクリア
	if (_stimulus.WasSuccessfullySensed()) { SetTargetActor(_actor); }
	else { ClearTarget(); }

	//if (_stimulus.WasSuccessfullySensed())
	//{
	//	SetTargetActor(_actor);
	//}
	//else
	//{
	//	ClearTarget();
	//}

	//if (_actor->ActorHasTag("Player"))
	//{
	//	if (_stimulus.WasSuccessfullySensed())
	//	{
	//		GetBlackboardComponent()->SetValueAsObject(TEXT("TargetActor"), _actor);
	//	}
	//}
	//else
	//{
	//	GetBlackboardComponent()->ClearValue(TEXT("TargetActor"));
	//}
}

//ターゲットをブラックボードに設定する関数
void AEnemyAIController::SetTargetActor(AActor* _actor)
{
	if (!m_pBlackboardComp) { return; }
	GetBlackboardComponent()->SetValueAsObject(TEXT("TargetActor"), _actor);
}

//ターゲットをブラックボードからクリアする関数
void AEnemyAIController::ClearTarget()
{
	if (!m_pBlackboardComp) { return; }
	GetBlackboardComponent()->ClearValue(TEXT("TargetActor"));
	StopMovement();
}

