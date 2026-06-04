// GhostCharacter.cpp

#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostCharacter/GhostPlaybackComponent.h"
#include "Ghost/GhostEnemy/GhostEnemyComponent.h"
#include "Player/GhostRecorderComponent.h"
#include "Ghost/AbilitySystem/GhostAbilitySystemComponent.h"
#include "Player/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"

AGhostCharacter::AGhostCharacter()
{
    // -----------------------------------------------------------------------
    // GAS
    // -----------------------------------------------------------------------
    GhostASC = CreateDefaultSubobject<UGhostAbilitySystemComponent>(TEXT("GhostASC"));
    GhostASC->SetIsReplicated(false);

    // -----------------------------------------------------------------------
    // コンポーネント
    // -----------------------------------------------------------------------
    PlaybackComponent = CreateDefaultSubobject<UGhostPlaybackComponent>(TEXT("PlaybackComponent"));
    CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
    EnemyComponent = CreateDefaultSubobject<UGhostEnemyComponent>(TEXT("EnemyComponent"));

    // -----------------------------------------------------------------------
    // CharacterMovement 設定
    // AddMovementInput が効くためにプレイヤーと同じ設定にする
    // -----------------------------------------------------------------------
    if (UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        // 移動方向に自動で向く（これがないと Ghost が正面を向いたまま横移動する）
        CMC->bOrientRotationToMovement = true;
        CMC->RotationRate = FRotator(0.f, 720.f, 0.f);

        CMC->MaxWalkSpeed = 600.f;
        CMC->MaxAcceleration = 2048.f;
        CMC->GroundFriction = 8.f;
        CMC->BrakingDecelerationWalking = 2048.f;
        CMC->GravityScale = 1.75f;
        CMC->JumpZVelocity = 600.f;
    }

    // コントローラー回転でキャラが回らないように
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    // EnemyMaterial は BP で設定するため null 初期化
    EnemyMaterial = nullptr;

    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();
}

void AGhostCharacter::BeginPlay()
{
    Super::BeginPlay();

    // GAS 初期化（二重呼び出し防止）
    if (!bGASInitialized && GhostASC)
    {
        GhostASC->InitAbilityActorInfo(this, this);
        GrantDefaultAbilities();
        bGASInitialized = true;
    }
}

// -----------------------------------------------------------------------
// IAbilitySystemInterface
// -----------------------------------------------------------------------
UAbilitySystemComponent* AGhostCharacter::GetAbilitySystemComponent() const
{
    return GhostASC;
}

// -----------------------------------------------------------------------
// GrantDefaultAbilities
// -----------------------------------------------------------------------
void AGhostCharacter::GrantDefaultAbilities()
{
    if (!GhostASC) return;
    if (AttackAbilityClass) GhostASC->GiveAbilityIfNotExists(AttackAbilityClass);
    if (DodgeAbilityClass)  GhostASC->GiveAbilityIfNotExists(DodgeAbilityClass);
}

// -----------------------------------------------------------------------
// InitializeGhost
// GhostManagerComponent or ActionCharacter から召喚後に呼ばれる
// -----------------------------------------------------------------------
void AGhostCharacter::InitializeGhost(
    const TArray<FGhostActionData>& Snapshot,
    UGhostRecorderComponent* Recorder)
{
    // SpawnActor 直後・BeginPlay より前に呼ばれる場合の安全対策
    if (!bGASInitialized && GhostASC)
    {
        GhostASC->InitAbilityActorInfo(this, this);
        GrantDefaultAbilities();
        bGASInitialized = true;
    }

    if (PlaybackComponent)
    {
        PlaybackComponent->Initialize(Snapshot, Recorder);
    }

    if (EnemyComponent)
    {
        EnemyComponent->StartLifetimeTimer();
    }
}

// -----------------------------------------------------------------------
// ActivateEnemyAI
// GhostEnemyComponent::Corrupt() から呼ばれる
// ① メッシュマテリアルを敵色に変更
// ② AIController をスポーンして Possess
// -----------------------------------------------------------------------
void AGhostCharacter::ActivateEnemyAI()
{
    // ① マテリアル変更（BP で EnemyMaterial が設定されていれば全スロットに適用）
    if (EnemyMaterial && GetMesh())
    {
        const int32 NumMaterials = GetMesh()->GetNumMaterials();
        for (int32 i = 0; i < NumMaterials; i++)
        {
            GetMesh()->SetMaterial(i, EnemyMaterial);
        }
   /*     UE_LOG(LogTemp, Log, TEXT("[GhostCharacter] マテリアルを敵色に変更しました。"));*/
    }
    else
    {
       /* UE_LOG(LogTemp, Log,
            TEXT("[GhostCharacter] EnemyMaterial 未設定。BP_GhostCharacter の Ghost|Visual で設定してください。"));*/
    }

    // ② AIController スポーン
    if (!GhostAIControllerClass)
    {
   /*     UE_LOG(LogTemp, Warning,
            TEXT("[GhostCharacter] GhostAIControllerClass 未設定。敵化後に動きません。"));*/
        return;
    }

    if (Controller)
    {
        Controller->UnPossess();
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AAIController* NewAI = GetWorld()->SpawnActor<AAIController>(
        GhostAIControllerClass, GetActorTransform(), Params);

    if (NewAI)
    {
        NewAI->Possess(this);
        UE_LOG(LogTemp, Log, TEXT("[GhostCharacter] 敵化完了 : AIController をアタッチしました。"));
    }
}