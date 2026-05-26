//ActionCharacter.cpp
//アクションキャラクターソース

#include "Player/ActionCharacter.h"
#include "Player/ActionMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Player//CombatComponent.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

#include "DrawDebugHelpers.h"//画面デバッグ用
//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("A"));

//FObjectInitializerを使用して、標準のMovementComponentを自作のものに差し替える
AActionCharacter::AActionCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UActionMovementComponent>(ACharacter::CharacterMovementComponentName)),
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

	//二段ジャンプの設定
	JumpMaxCount = 2;

	//コントローラーの回転でキャラ自体が回らないようにする
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

	//戦闘コンポーネントの生成
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	//ロックオンコンポーネントの生成
	LockOnComponent = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComponent"));

	// ----------------------------------------------------
	//カメラの設定（簡易的）
	// ----------------------------------------------------
	//スプリングアームの作成と取り付け
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	//視点操作の動きに合わせてアームを回転させるか
	CameraBoom->bUsePawnControlRotation = true;

	//基本の長さ
	CameraBoom->TargetArmLength = 400.0f;
	// 壁の衝突テスト
	CameraBoom->bDoCollisionTest = true;
	//カメラが反応するチャンネルををWorldStaticのみにする
	CameraBoom->ProbeChannel = ECC_GameTraceChannel1;
	//判定用の球の大きさを少し小さくしてがたつきを抑える
	CameraBoom->ProbeSize = 10.f;
	// 右肩越しオフセット
	CameraBoom->SocketOffset = FVector(0.0f, 60.0f, 50.0f);
	// カメラスムージング
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 15.0f;


	//カメラ本体の作成と取り付け
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	// スプリングアームの先端に取り付ける
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// カメラ自体はアームの動きに追従するだけで、自分では回転しない
	FollowCamera->bUsePawnControlRotation = false;


	//分身コンポーネントの生成
	GhostRecorder = CreateDefaultSubobject<UGhostRecorderComponent>(TEXT("GhostRecorder"));
}

UActionMovementComponent* AActionCharacter::GetActionMovementComponent() const
{
	return Cast<UActionMovementComponent>(GetCharacterMovement());
}

void AActionCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentEnergy = 0.f;

	//CombatComponentのヒットデリゲートを取得
	if (CombatComponent)
	{
		CombatComponent->OnHitEnemy.AddUObject(this, &AActionCharacter::OnHitEnemy);
	}

	DefaultSocketOffset = CameraBoom->SocketOffset;

	if (LockOnComponent)
	{
		LockOnComponent->OnLockOnChanged.AddUObject(this, &AActionCharacter::OnLockOnChanged);
	}

	//プレイヤーコントローラーを取得しIMCを登録する
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AActionCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//ロックオン中のカメラ制御
	if (LockOnComponent && LockOnComponent->IsLockedOn())
	{
		UpdateLockOnCamera(DeltaTime);

		//AActor* Target = LockOnComponent->GetTarget();
		//if (Target && Controller)
		//{
		//	//プレイヤーとターゲットの中間点を計算
		//	FVector PlayerLocation = GetActorLocation();
		//	FVector TargetLocation = Target->GetActorLocation();

		//	//カメラからターゲットへの方向を計算
		//	FVector CameraLocation = FollowCamera->GetComponentLocation();
		//	FVector ToTarget = (TargetLocation - CameraLocation).GetSafeNormal();

		//	FRotator TargetRotation = ToTarget.Rotation();

		//	//滑らかにカメラを向ける
		//	FRotator CurrentRotation = Controller->GetControlRotation();
		//	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 5.f);

		//	Controller->SetControlRotation(NewRotation);

		//	//プレイヤーも敵の方向を向く
		//	FVector ToTargetFlat = (TargetLocation - PlayerLocation);
		//	ToTargetFlat.Z = 0.f;
		//	if (!ToTargetFlat.IsNearlyZero())
		//	{
		//		FRotator PlayerRotation = ToTargetFlat.GetSafeNormal().Rotation();
		//		SetActorRotation(FMath::RInterpTo(GetActorRotation(), PlayerRotation, DeltaTime, 10.f));
		//	}
		//}
	}

	//キャラクターの現在の平行移動速度を取得
	float CurrentSpeed = GetVelocity().Size2D();

	//速度が一定以上かチェック
	if (CurrentSpeed > 10.f)
	{
		//走っている時間を加算
		CurrentRunTime += DeltaTime;

		//一定時間走り続けたら、自動的にダッシュモードへ移行
		if (CurrentRunTime >= TimeToSprint)
		{
			StartSprint();
		}
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
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//攻撃のバインド
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AActionCharacter::Attack);

		//ロックオン
		EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Started, this, &AActionCharacter::LockOn);

		//移動
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AActionCharacter::Move);

		//視点移動
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AActionCharacter::Look);

		//ジャンプ
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AActionCharacter::OnJumpPressed);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AActionCharacter::OnJumpReleased);

		//回避のバインド
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &AActionCharacter::Dodge);

		//召喚バインド
		EnhancedInputComponent->BindAction(SummonAction, ETriggerEvent::Started, this, &AActionCharacter::SummonGhost);

	}
}

void AActionCharacter::Attack()
{
	if (CombatComponent)
	{
		CombatComponent->ExecuteAttack();
	}
}

void AActionCharacter::OnHitEnemy(float EnergyGain)
{
	//ヒット時のエネルギー加算
	CurrentEnergy = FMath::Clamp(CurrentEnergy + EnergyGain, 0.f, MaxEnergy);

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, FString::Printf(TEXT("Energy: %.1f / %.1f"), CurrentEnergy, MaxEnergy));
}

void AActionCharacter::LockOn()
{
	if (LockOnComponent)
	{
		LockOnComponent->ToggleLockOn();
	}
}

void AActionCharacter::OnLockOnChanged(AActor* NewTarget)
{
	if (NewTarget)
	{
		//ロックON：プレイヤーをコントローラーのYawに従わせる
		bUseControllerRotationYaw = true;
		GetActionMovementComponent()->bOrientRotationToMovement = false;

		//カメラを横にずらしてプレイヤーとターゲット両方映す
		CameraBoom->SocketOffset = FVector(
			DefaultSocketOffset.X,
			DefaultSocketOffset.Y + LockOnCameraOffsetY,
			DefaultSocketOffset.Z
		);

		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("ロックオン： %s"), *NewTarget->GetName()));
	}
	else
	{
		//ロックオンOFF：通常の移動方向向きに戻す
		bUseControllerRotationYaw = false;
		GetActionMovementComponent()->bOrientRotationToMovement = true;

		//ソケットオフセットを元に戻す
		CameraBoom->SocketOffset = DefaultSocketOffset;

		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Purple,TEXT("FinithLockOn"));

	}
}

void AActionCharacter::UpdateLockOnCamera(float DeltaTime)
{
	AActor* Target = LockOnComponent->GetTarget();
	if (!Target || !Controller) return;

	FVector PlayerLocation = GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();

	//プレイヤーと敵の中間点をカメラの注視点にする
	FVector MidPoint = (PlayerLocation + TargetLocation) * 0.5f;

	//カメラ位置から中間点への方向でRotatorを作る
	FVector CameraLocation = FollowCamera->GetComponentLocation();
	FVector ToMid = (MidPoint - CameraLocation).GetSafeNormal();
	FRotator TargetRot = ToMid.Rotation();

	//距離に応じて補完速度を変える
	float Distance = FVector::Dist(PlayerLocation, TargetLocation);
	float DynamicInterpSpeed = FMath::GetMappedRangeValueClamped(
		FVector2D(200.f, 1500.f),  //距離レンジ
		FVector2D(2.f, LockOnCameraInterpSpeed),  //対応する補完速度レンジ
		Distance
	);

	FRotator CurrentRot = Controller->GetControlRotation();
	FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, DynamicInterpSpeed);

	// 極端な見上げ・見下ろしを防ぐ
	NewRot.Pitch = FMath::Clamp(NewRot.Pitch, LockOnPitchMin, LockOnPitchMax);
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
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		//コントローラーが向いている回転を取得
		const FRotator Rotation = Controller->GetControlRotation();
		//Z軸だけを抽出し、ピッチやロール傾きは無視する
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		//カメラが向いている方向の「前」ベクトルの取得
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		//カメラが向いている方向の「右」ベクトルの取得
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		//キャラクターに移動命令を出す
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

//走る入力が開始（Started）された時
void AActionCharacter::StartSprint()
{
	if (UActionMovementComponent* MoveComp = GetActionMovementComponent())
	{
		MoveComp->SetSprinting(true);
	}
}

//走る入力が終了（Completed）した時
void AActionCharacter::StopSprint()
{
	if (UActionMovementComponent* MoveComp = GetActionMovementComponent())
	{
		MoveComp->SetSprinting(false);
	}
}

void AActionCharacter::Look(const FInputActionValue& Value)
{
	//マウスの移動量（XとY）を取得
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		//ロックオン中は右スティックをターゲット切り替えに使う
		if (LockOnComponent && LockOnComponent->IsLockedOn())
		{
			LockOnComponent->TrySwitchTarget(LookAxisVector.X);
			return;
		}

		//左右のカメラ回転（Yaw）
		AddControllerYawInput(LookAxisVector.X);
		//上下のカメラ回転（Pitch）
		AddControllerPitchInput(LookAxisVector.Y);
	}
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

//回避
void AActionCharacter::Dodge()
{
	if (CombatComponent)
	{
		CombatComponent->ExecuteDodge();
	}
}

//残像召喚
void AActionCharacter::SummonGhost()
{
	//エネルギーチェック
	if (CurrentEnergy < GhostSummonCost) return;

	//レコーダーからデータ取得
	if (!GhostRecorder) return;
	TArray<FGhostActionData> Actions = GhostRecorder->GetRecordedActions();

	//データが空なら召喚しない
	if (Actions.IsEmpty()) return;

	//プレイヤー近く（右側）にSpawn
	FVector SpawnLocation = GetActorLocation() + GetActorRightVector() * 100.f;
	FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters Params;
	Params.Owner = this;

	//ワールドにスポーン
	AGhostCharacter* Ghost = GetWorld()->SpawnActor<AGhostCharacter>(
		GhostCharacterClass,
		SpawnLocation,
		SpawnRotation,
		Params
	);

	if (Ghost)
	{
		Ghost->InitGhost(Actions);
		//コスト支払いとカウンタの更新
		CurrentEnergy -= GhostSummonCost;
		ActiveGhostCount++;
	}
}
