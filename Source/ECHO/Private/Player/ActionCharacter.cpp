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

    //二段ジャンプ設定
    JumpMaxCount = 2;

    //コントローラーの回転でキャラ自体が回らないように
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
    //移動方向にキャラクターが自動でスッと向くようにする
    GetActionMovementComponent()->bOrientRotationToMovement = true;

    //キャラクターの移動入力がある方向へ、自動的に滑らかに向きを変える設定
    if (UActionMovementComponent* MoveComp = GetActionMovementComponent())
    {
        MoveComp->bOrientRotationToMovement = true;
        MoveComp->RotationRate = FRotator(0.f, 1000.f, 0.f);  //旋回速度
    }

    CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

    LockOnComponent = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComponent"));

    //カメラの設定
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->bUsePawnControlRotation = true;

    CameraBoom->TargetArmLength = 400.f;
    //壁の衝突テスト
    CameraBoom->bDoCollisionTest = true;
    //カメラが反応するチャンネルををWorldStaticのみにする（変えたい）
    CameraBoom->ProbeChannel = ECC_GameTraceChannel1;
    //判定用の球の大きさを少し小さくしてがたつきを抑える
    CameraBoom->ProbeSize = 10.f;
    //右肩越しオフセット
    CameraBoom->SocketOffset = FVector(0.f, 60.f, 50.f);
    //カメラスムージング
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

    DefaultSocketOffset = CameraBoom->SocketOffset;

    if (LockOnComponent)
    {
        LockOnComponent->OnLockOnChanged.AddUObject(this, &AActionCharacter::OnLockOnChanged);
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

    //キャラクターの現在の平行移動速度を取得
    const float Speed = GetVelocity().Size2D();

    //速度が一定以上かチェック
    if (Speed > 10.f)
    {
        //走っている時間を加算
        CurrentRunTime += DeltaTime;

        //一定時間走り続けたら、自動的にダッシュモードへ移行
        if (CurrentRunTime >= TimeToSprint) StartSprint();
    }
    else
    {
        //立ち止まったら計測リセットとダッシュ解除
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
        //召喚バインド
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
    //ヒット時のエネルギー加算
    CurrentEnergy = FMath::Clamp(CurrentEnergy + EnergyGain, 0.f, MaxEnergy);

    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, FString::Printf(TEXT("Energy: %.1f / %.1f"), CurrentEnergy, MaxEnergy));
}

//ロックオンシステム改良中（6.2）
void AActionCharacter::OnLockOnChanged(AActor* NewTarget)
{
    if (NewTarget)
    {
        //ロックON：プレイヤーをコントローラーのYawに従わせる
        bUseControllerRotationYaw = true;
        GetActionMovementComponent()->bOrientRotationToMovement = false;

        //カメラを横にずらしてプレイヤーとターゲット両方映す
        //CameraBoom->SocketOffset = FVector(
        //	DefaultSocketOffset.X,
        //	DefaultSocketOffset.Y + LockOnCameraOffsetY,
        //	DefaultSocketOffset.Z
        //);

        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("ロックオン： %s"), *NewTarget->GetName()));
    }
    else
    {
        //ロックオンOFF：通常の移動方向向きに戻す
        //bUseControllerRotationYaw = false;
        //GetActionMovementComponent()->bOrientRotationToMovement = true;

        //ソケットオフセットを元に戻す
        CameraBoom->SocketOffset = DefaultSocketOffset;

        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Purple, TEXT("FinithLockOn"));

    }
}

void AActionCharacter::UpdateLockOnCamera(float DeltaTime)
{
    AActor* Target = LockOnComponent->GetTarget();
    if (!Target || !Controller) return;

    FVector PlayerLocation = GetActorLocation();
    FVector TargetLocation = Target->GetActorLocation();

    //ターゲットがプレイヤーの左右どちらにいるか判定
    //カメラの右ベクトルとの内積で判定する
    FVector CameraRight = FollowCamera->GetRightVector();
    FVector ToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();
    float RightDot = FVector::DotProduct(CameraRight, ToTarget);

    //ターゲットが左にいるときはカメラをプレイヤーの左側にオフセット
    float DesiredOffsetY = (RightDot >= 0.f) ? DefaultSocketOffset.Y + m_LockOnCameraOffsetY
        : -(DefaultSocketOffset.Y + m_LockOnCameraOffsetY);

    //急にオフセットが変わるとき滑らかに移動
    FVector CurrentOffset = CameraBoom->SocketOffset;
    float NewOffsetY = FMath::FInterpTo(CurrentOffset.Y, DesiredOffsetY, DeltaTime, 15.f);
    CameraBoom->SocketOffset = FVector(CurrentOffset.X, NewOffsetY, CurrentOffset.Z);

    ////プレイヤーと敵の中間点をカメラの注視点にする
    FVector MidPoint = (PlayerLocation + TargetLocation) * 0.5f;

    ////カメラ位置から中間点への方向でRotatorを作る
    FVector CameraLocation = FollowCamera->GetComponentLocation();
    FVector ToMid = (MidPoint - CameraLocation).GetSafeNormal();
    FRotator TargetRot = ToMid.Rotation();

    //距離に応じて補完速度を変える
    float Distance = FVector::Dist(PlayerLocation, TargetLocation);
    float DynamicInterpSpeed = FMath::GetMappedRangeValueClamped(
        FVector2D(200.f, 1500.f),  //距離レンジ
        FVector2D(2.f, m_LockOnCameraInterpSpeed),  //対応する補完速度レンジ
        Distance
    );

    FRotator CurrentRot = Controller->GetControlRotation();
    FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, DynamicInterpSpeed);

    // 極端な見上げ・見下ろしを防ぐ
    NewRot.Pitch = FMath::Clamp(NewRot.Pitch, m_LockOnPitchMin, m_LockOnPitchMax);
    NewRot.Roll = 0.f;

    Controller->SetControlRotation(NewRot);

    // プレイヤーも敵の方向を向く
    FVector ToTargetFlat = (Target->GetActorLocation() - PlayerLocation);
    ToTargetFlat.Z = 0.f;
    if (!ToTargetFlat.IsNearlyZero())
    {
        FRotator PlayerRot = ToTargetFlat.GetSafeNormal().Rotation();
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), PlayerRot, DeltaTime, 10.f));
    }
}

void AActionCharacter::Move(const FInputActionValue& Value)
{
    //入力値を2Dベクトルとして取得
    const FVector2D MovementVector = Value.Get<FVector2D>();

    if (!Controller) return;

    const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), MovementVector.Y);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), MovementVector.X);
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
    const FVector2D LookAxisVector = Value.Get<FVector2D>();
    if (!Controller) return;

    //ロックオン中は右スティックをターゲット切り替えに使う
    if (LockOnComponent && LockOnComponent->IsLockedOn())
    {
        LockOnComponent->TrySwitchTarget(LookAxisVector.X);
        return;
    }

    //左右のカメラ回転
    AddControllerYawInput(LookAxisVector.X);
    //上下のカメラ回転
    AddControllerPitchInput(LookAxisVector.Y);
}

void AActionCharacter::OnJumped_Implementation()
{
    Super::OnJumped_Implementation();

    UActionMovementComponent* MoveComp = GetActionMovementComponent();
    if (!MoveComp) return;

    //現在のジャンプ回数を確認
    //JumpCurrentCount はACharacterに標準で用意されている「現在何回目のジャンプか」を持つ変数
    if (JumpCurrentCount == 1)
    {
        //【1段目のジャンプ時の処理】
        MoveComp->AirControl = MoveComp->AirControlFirstJump;


        //地面を蹴る土煙のエフェクト(Niagara)を足元に出す
        //「ハッ！」という通常ジャンプのボイスやSEを再生する
    }
    else if (JumpCurrentCount == 2)
    {
        //【2段目のジャンプ（エアハイク）時の処理】
        MoveComp->AirControl = MoveComp->AirControlSecondJump;

        FVector Vel = MoveComp->Velocity;
        Vel.Z = MoveComp->SecondJumpZVelocity;
        MoveComp->Velocity = Vel;

        //入力方向に一度だけ方向転換
        FVector InputVector = MoveComp->GetLastInputVector();
        if (!InputVector.IsNearlyZero())
        {
            FVector InputDir = InputVector.GetSafeNormal2D();
            SetActorRotation(InputDir.Rotation());

            float HorizontalSpeed = FVector(Vel.X, Vel.Y, 0.f).Size();
            MoveComp->Velocity.X = InputDir.X * HorizontalSpeed;
            MoveComp->Velocity.Y = InputDir.Y * HorizontalSpeed;
        }

        //空中に魔法陣や衝撃波のエフェクトを出す
        //キャラクターが空中でクルッと回転するような専用のモンタージュを再生する
        //Z軸（上方向）に少し追加の初速を与えて、滞空時間を伸ばす
    }
}

void AActionCharacter::OnJumpPressed()
{
    JumpPressedTime = GetWorld()->GetTimeSeconds();
    Jump();
}

void AActionCharacter::OnJumpReleased()
{
    StopJumping();

    // 二段ジャンプ中はZ速度を触らない
    if (JumpCurrentCount >= 2) return;

    //ボタンを押していた時間を算出
    float HoldDuration = GetWorld()->GetTimeSeconds() - JumpPressedTime;

    UActionMovementComponent* MoveComp = GetActionMovementComponent();
    if (!MoveComp) return;

    //空中でボタンが即座に離された場合、上昇速度の上限を低くカットすることで小ジャンプを表現する
    if (MoveComp->IsFalling())
    {
        FVector Vel = MoveComp->Velocity;
        if (HoldDuration < JumpHoldThreshold)
        {
            Vel.Z = FMath::Min(Vel.Z, JumpZVelocityShort);
        }
        MoveComp->Velocity = Vel;
    }
}

void AActionCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);

    //着地復帰処理
    UActionMovementComponent* MoveComp = GetActionMovementComponent();
    if (MoveComp)
    {
        MoveComp->AirControl = 0.05f;
        bool bWasSprinting = MoveComp->IsSprinting();
        MoveComp->StartLandingRecovery(bWasSprinting);
    }

    if (CombatComponent)
    {
        CombatComponent->ResetAirDodge();
    }

    //【着地時の処理】
    //着地した瞬間に「ドスッ」という重いSEと、足元に砂埃エフェクトを出す
    //高い場所から落ちた場合（落下速度 Z が一定以上だった場合）、数フレームだけ移動入力を無視して「着地硬直」のアニメーションを入れる
    //空中コンボ中だった場合、コンボのステート（状態）をリセットする
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