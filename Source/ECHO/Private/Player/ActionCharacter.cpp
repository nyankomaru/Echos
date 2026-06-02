// ActionCharacter.cpp

#include "Player/ActionCharacter.h"
#include "Player/ActionMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Player/CombatComponent.h"
#include "Player/GhostRecorderComponent.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostCharacter/GhostManagerComponent.h"
#include "Ghost/Data/GhostTypes.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"

AActionCharacter::AActionCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UActionMovementComponent>(
        ACharacter::CharacterMovementComponentName)),
    TimeToSprint(1.5f),
    CurrentRunTime(0.f),
    GhostSummonCost(30.f),
    MaxEnergy(100.f),
    EnergyGainPerHit(15.f),
    CurrentEnergy(0.f),
    ActiveGhostCount(0),
    MaxGhostCount(5)
{
    PrimaryActorTick.bCanEverTick = true;

    JumpMaxCount = 2;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetActionMovementComponent()->bOrientRotationToMovement = true;
    GetActionMovementComponent()->RotationRate = FRotator(0.f, 1000.f, 0.f);

    CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

    // カメラ
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->TargetArmLength = 400.f;
    CameraBoom->bDoCollisionTest = true;
    CameraBoom->ProbeChannel = ECC_GameTraceChannel1;
    CameraBoom->ProbeSize = 10.f;
    CameraBoom->SocketOffset = FVector(0.f, 60.f, 50.f);
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 15.f;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // Ghost システム
    GhostRecorder = CreateDefaultSubobject<UGhostRecorderComponent>(TEXT("GhostRecorder"));
    GhostManager = CreateDefaultSubobject<UGhostManagerComponent>(TEXT("GhostManager"));
}

UActionMovementComponent* AActionCharacter::GetActionMovementComponent() const
{
    return Cast<UActionMovementComponent>(GetCharacterMovement());
}

void AActionCharacter::BeginPlay()
{
    Super::BeginPlay();

    CurrentEnergy = 0.f;

    if (CombatComponent)
    {
        CombatComponent->OnHitEnemy.AddUObject(this, &AActionCharacter::OnHitEnemy);
    }

    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Sub =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            Sub->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

void AActionCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    const float Speed = GetVelocity().Size2D();

    if (Speed > 10.f)
    {
        CurrentRunTime += DeltaTime;
        if (CurrentRunTime >= TimeToSprint) StartSprint();
    }
    else
    {
        CurrentRunTime = 0.f;
        StopSprint();
    }
}

void AActionCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &AActionCharacter::Attack);
        EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AActionCharacter::Move);
        EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AActionCharacter::Look);
        EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        EIC->BindAction(DodgeAction, ETriggerEvent::Started, this, &AActionCharacter::Dodge);
        EIC->BindAction(SummonAction, ETriggerEvent::Started, this, &AActionCharacter::SummonGhost);
    }
}

void AActionCharacter::Attack()
{
    if (CombatComponent) CombatComponent->ExecuteAttack();
}

void AActionCharacter::Dodge()
{
    if (CombatComponent) CombatComponent->ExecuteDodge();
}

void AActionCharacter::OnHitEnemy(float EnergyGain)
{
    CurrentEnergy = FMath::Clamp(CurrentEnergy + EnergyGain, 0.f, MaxEnergy);

    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
        FString::Printf(TEXT("Energy: %.1f / %.1f"), CurrentEnergy, MaxEnergy));
}

void AActionCharacter::Move(const FInputActionValue& Value)
{
    const FVector2D MV = Value.Get<FVector2D>();
    if (!Controller) return;

    const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), MV.Y);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), MV.X);
}

void AActionCharacter::StartSprint()
{
    if (UActionMovementComponent* MC = GetActionMovementComponent())
        MC->SetSprinting(true);
}

void AActionCharacter::StopSprint()
{
    if (UActionMovementComponent* MC = GetActionMovementComponent())
        MC->SetSprinting(false);
}

void AActionCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D LV = Value.Get<FVector2D>();
    if (!Controller) return;
    AddControllerYawInput(LV.X);
    AddControllerPitchInput(LV.Y);
}

void AActionCharacter::OnJumped_Implementation()
{
    Super::OnJumped_Implementation();
    // 1段目・2段目ジャンプのエフェクト・SE はここに追加
}

void AActionCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    if (CombatComponent) CombatComponent->ResetAirDodge();
}

// -----------------------------------------------------------------------
// SummonGhost
// エネルギーを確認し、GhostRecorder のスナップショットを渡して
// GhostCharacter をスポーンする
// -----------------------------------------------------------------------
void AActionCharacter::SummonGhost()
{
    // エネルギー不足チェック
    if (CurrentEnergy < GhostSummonCost)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
            FString::Printf(TEXT("エネルギー不足: %.1f / %.1f"), CurrentEnergy, GhostSummonCost));
        return;
    }

    // 最大数チェック
    if (ActiveGhostCount >= MaxGhostCount)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
            TEXT("分身の上限に達しています"));
        return;
    }

    if (!GhostRecorder || !GhostCharacterClass) return;

    // 直近 20秒のスナップショットを取得
    const TArray<FGhostActionData> Snapshot = GhostRecorder->GetSnapshot(20.f);
    if (Snapshot.IsEmpty())
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
            TEXT("記録データなし。まず動いてください"));
        return;
    }

    // スポーン位置（プレイヤーの右横）
    const FVector  SpawnLoc = GetActorLocation() + GetActorRightVector() * 100.f;
    const FRotator SpawnRot = GetActorRotation();

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AGhostCharacter* Ghost = GetWorld()->SpawnActor<AGhostCharacter>(
        GhostCharacterClass, SpawnLoc, SpawnRot, Params);

    if (!Ghost) return;

    // スナップショットと Recorder を渡して再生を開始
    Ghost->InitializeGhost(Snapshot, GhostRecorder);

    CurrentEnergy -= GhostSummonCost;
    ActiveGhostCount++;

    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
        FString::Printf(TEXT("Ghost 召喚！ アクティブ数: %d"), ActiveGhostCount));
}