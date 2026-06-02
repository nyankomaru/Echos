#include "Enemy/EnemyPool/BossSpawnTrigger.h"
#include "Enemy/EnemyPool/WaveManager.h"
#include "Components/BoxComponent.h"

// Sets default values
ABossSpawnTrigger::ABossSpawnTrigger()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	//コンポネント
	m_triggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = m_triggerBox;

	//デフォルトサイズ
	m_triggerBox->SetBoxExtent(FVector(200.f, 200.f, 200.f));
	//コリジョン設定
	m_triggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	m_triggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	m_triggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	m_triggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

// Called when the game starts or when spawned
void ABossSpawnTrigger::BeginPlay()
{
	Super::BeginPlay();

	//オーバーラップイベントに関数をバインド
	m_triggerBox->OnComponentBeginOverlap.AddDynamic(this, &ABossSpawnTrigger::OnPlayerEntered);
}

//プレイヤーがトリガーに入ったときの処理
void ABossSpawnTrigger::OnPlayerEntered(UPrimitiveComponent* _overlappedComp, AActor* _otherActor, UPrimitiveComponent* _otherComp, int32 _otherBodyIndex, bool _bFromSweep, const FHitResult& _sweepResult)
{
	//一度だけ発動
	if (m_bHasTriggered) { return; }

	//Playerタグを持つアクターかチェック
	if (!_otherActor || !_otherActor->ActorHasTag(TEXT("Player"))) { return; }

	//発動をフラグを立てる
	m_bHasTriggered = true;

	//ウェーブマネージャーがヌールだったら処理しない
	if (!m_waveManager) { return; }

	//スポーンさせるフラグが立っていたらスポーンさせる
	if (m_bSpawnOnEnter) { m_waveManager->SpawnFinalBoss(); }
}

