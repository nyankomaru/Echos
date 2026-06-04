#include "Ghost/GhostCharacter/GhostManagerComponent.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Player/GhostRecorderComponent.h"
#include "Ghost/GhostEnemy/GhostEnemyComponent.h"
#include "GameFramework/Character.h"

//コンストラクタ
UGhostManagerComponent::UGhostManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

//開始時に呼ばれる関数
void UGhostManagerComponent::BeginPlay()
{
	//親クラスの BeginPlay を呼び出す
    Super::BeginPlay();

    // オーナーから GhostRecorderComponent を取得
    RecorderRef = GetOwner()->FindComponentByClass<UGhostRecorderComponent>();

	// GhostRecorderComponent が見つからない場合の処理
    if (!RecorderRef)
    {
        return;
    }
}

//生成を試みる
bool UGhostManagerComponent::TrySummon(float Cost)
{
    CleanupDeadGhosts();

    // 最大数チェック
    if (ActiveGhosts.Num() >= MaxGhostCount)
    {
        return false;
    }

	// コストチェック（実装は呼び出し元に任せる）
    if (!GhostCharacterClass || !RecorderRef)
    {
        return false;
    }

    // スナップショット取得
    TArray<FGhostActionData> Snapshot = RecorderRef->GetSnapshot(SnapshotDuration);
    if (Snapshot.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[GhostManager] 記録データが空です。まず動いてください。"));
        return false;
    }

    // スポーン位置：プレイヤー周囲に分散して配置
    ACharacter* Owner = Cast<ACharacter>(GetOwner());
    if (!Owner) return false;

    const FRotator Yaw = FRotator(0.f, Owner->GetActorRotation().Yaw, 0.f);
    // 既存の Ghost 数に応じて角度をずらして配置（重なり防止）
    const float   Angle = 60.f * ActiveGhosts.Num();
    const FVector Offset = FRotator(0.f, Angle, 0.f).RotateVector(SpawnOffset);
    const FVector SpawnLoc = Owner->GetActorLocation() + Yaw.RotateVector(Offset);

	//ActorSpawnParameters 設定
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	//分身をスポーン
    AGhostCharacter* Ghost = GetWorld()->SpawnActor<AGhostCharacter>(GhostCharacterClass, SpawnLoc, Owner->GetActorRotation(), Params);

	//スポーン失敗の可能性を考慮
    if (!Ghost)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GhostManager] Ghost のスポーンに失敗しました。"));
        return false;
    }

    // 初期化（再生開始・敵化タイマー開始）
    Ghost->InitializeGhost(Snapshot, RecorderRef);

	//アクティブリストに追加
    ActiveGhosts.Add(Ghost);

    UE_LOG(LogTemp, Log, TEXT("[GhostManager] Ghost 召喚成功。アクティブ数: %d"),ActiveGhosts.Num());

    return true;
}

//既に死亡・無効になった Ghost をリストから除去
void UGhostManagerComponent::CleanupDeadGhosts()
{
    ActiveGhosts.RemoveAll([](const TObjectPtr<AGhostCharacter>& G)
        {
            if (!G.Get()) return true;

            // Dead 状態の Ghost を除去
            if (UGhostEnemyComponent* EC = G->EnemyComponent)
            {
                return EC->GetLifecycleState() == EGhostLifecycleState::Dead;
            }
            return false;
        });
}
