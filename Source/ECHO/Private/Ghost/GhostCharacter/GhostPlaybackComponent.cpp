// GhostPlaybackComponent.cpp

#include "Ghost/GhostCharacter/GhostPlaybackComponent.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Player/GhostRecorderComponent.h"
#include "Ghost/AbilitySystem/GhostAbilitySystemComponent.h"
#include "Ghost/Data/GhostGameplayTags.h"
#include "Player/CombatComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

//コンストラ
UGhostPlaybackComponent::UGhostPlaybackComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

// 初期化/モード切替
void UGhostPlaybackComponent::Initialize( const TArray<FGhostActionData>& Snapshot,UGhostRecorderComponent* Recorder)
{
	//スナップショットをキューにセットして再生開始
    //PlaybackQueue = Snapshot;
   // PlaybackIndex = 0;
    RecorderRef = Recorder;

	//World 時刻を基準にする
    UWorld* World = GetWorld();

	//タイムス取得
    if (Recorder)
    {
        TArray<FGhostActionData> AllActions = Recorder->GetSnapshot(9999.f);
        if (AllActions.Num() > 0)
        {
            //最新のタイムスタンプ = これより前のレコードは再生しない
            LastImmediateFetchTime = AllActions.Last().Timestamp;
        }
        else
        {
            //レコードがない場合は World 時刻 or 0
            LastImmediateFetchTime = World ? World->GetTimeSeconds() : 0.f;
        }
    }
    else
    {
        LastImmediateFetchTime = World ? World->GetTimeSeconds() : 0.f;
    }


	//即時ミラー用の初期化
	CurrentMoveDirection = FVector::ZeroVector;
	LastMoveReceivedTime = -999.f;
	bAnimAttacking = false;
	bAnimDodging = false;
	bPrevAttackingCombat = false;

	//囲みオフセット角度を召喚時にランダムに決める（複数体同時に召喚されたときの重なり防止）
    SurroundAngleOffset = FMath::RandRange(0.f, 360.f);

	CurrentMode = EGhostPlaybackMode::ImmediateMirror;

    //-------------------------------
    // 遅延ミラー用
    //-------------------------------
    //CurrentMode = EGhostPlaybackMode::RecordedPlayback;
    //PlaybackStartTime = GetWorld()->GetTimeSeconds();

	//スナップショットの先頭タイムスタンプを基準にすることで、Snapshot 内のアクションはすべて相対時間で再生される
    //SnapshotStartTime = (PlaybackQueue.Num() > 0)? PlaybackQueue[0].Timestamp: PlaybackStartTime;

	//遅延ミラー用の初期化
    //LastMirrorFetchTime = PlaybackStartTime - MirrorDelay;
    //CurrentMoveDirection = FVector::ZeroVector;
    //LastMoveReceivedTime = -999.f;
    //bAnimAttacking = false;
    //bAnimDodging = false;
    //bPrevAttackingCombat = false;
    //--------------------------------------------------
	
    // Tickを有効化して再生開始
    SetComponentTickEnabled(true);

    //-------------------------------
    // 遅延ミラー用
    //-------------------------------

    //初期位置をスナップショット先頭にテレポート
    //if (PlaybackQueue.Num() > 0)
    //{
    //    if (AActor* Owner = GetOwner())
    //    {
    //        Owner->SetActorLocation(PlaybackQueue[0].Location,
    //            false, nullptr, ETeleportType::TeleportPhysics);
    //        Owner->SetActorRotation(PlaybackQueue[0].Rotation);
    //    }
    //}
}

//敵モードへ切り替える関数
void UGhostPlaybackComponent::SetEnemyMode()
{
	//敵モードへ切り替え
    CurrentMode = EGhostPlaybackMode::EnemyMode;

	//再生停止
    CurrentMoveDirection = FVector::ZeroVector;

	//AnimBP用の状態リセット
    AnimSpeed = 0.f;

	//攻撃と回避のフラグは念のためリセットしておく
    bAnimAttacking = false;

	//回避は敵モードでは使わない想定だが、一応リセットしておく
    bAnimDodging = false;

   // SetComponentTickEnabled(false);
}

//毎フレーム呼ばれる関数
void UGhostPlaybackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    //基底クラスを呼び出す
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//AnimBP 用の状態更新は TickComponent 内で行う。
	if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* CMC = Owner->GetCharacterMovement())
		{
			AnimSpeed = CMC->Velocity.Size2D();
		}
	}

	//敵モード中は再生処理を行わない（AnimBP の攻撃フラグの同期は行う）
	if (CurrentMode == EGhostPlaybackMode::EnemyMode)
	{
		// 敵モードでは移動やアクションの再生は行わず、AnimBP の攻撃フラグだけ CombatComponent と
		SyncAttackAnimFlag();
		return;
	}
    //囲み移動
    TickSurroundMovement(DeltaTime);

    // 移動の持続は全モード共通で毎 Tick 実行
   // TickMovement(DeltaTime);

    switch (CurrentMode)
    {
    case EGhostPlaybackMode::RecordedPlayback:
        //TickRecordedPlayback(DeltaTime);
        break;
    case EGhostPlaybackMode::DelayedMirror:
        //TickDelayedMirror(DeltaTime);
        break;
	case EGhostPlaybackMode::ImmediateMirror:
		TickImmediateMirror(DeltaTime);
        break;
    default:
        break;
    }


    //AnimBP用の攻撃フラグの更新
	SyncAttackAnimFlag();
}

//プレイヤーを囲むように移動する関数
void UGhostPlaybackComponent::TickSurroundMovement(float DeltaTime)
{
    ACharacter* GhostOwner = Cast<ACharacter>(GetOwner());
    if (!GhostOwner) return;

    // プレイヤーを取得
    APawn* PlayerPawn = nullptr;
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        PlayerPawn = PC->GetPawn();
    if (!PlayerPawn) return;

    const FVector MyPos = GhostOwner->GetActorLocation();
    const FVector PlayerPos = PlayerPawn->GetActorLocation();

    // プレイヤーに近すぎる場合は離れる（邪魔しない）
    const float DistToPlayer = FVector::Dist2D(MyPos, PlayerPos);
    if (DistToPlayer < MinDistFromPlayer)
    {
        FVector AwayDir = (MyPos - PlayerPos).GetSafeNormal2D();
        CurrentMoveDirection = AwayDir;
        LastMoveReceivedTime = GetWorld()->GetTimeSeconds();

        GhostOwner->AddMovementInput(AwayDir, 1.f);
        return;
    }

    // 近くの敵を探す（プレイヤーの周囲 EnemySearchRadius 内）
    AActor* NearestEnemy = nullptr;
    float   NearestDist = EnemySearchRadius;

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), AllActors);

    for (AActor* Actor : AllActors)
    {
        if (Actor == GhostOwner) continue;
        if (Actor == PlayerPawn) continue;
        // Ghost 自身（AGhostCharacter）は除外
        if (Actor->IsA(GetOwner()->GetClass())) continue;

        const float Dist = FVector::Dist2D(Actor->GetActorLocation(), PlayerPos);
        if (Dist < NearestDist)
        {
            NearestDist = Dist;
            NearestEnemy = Actor;
        }
    }

    if (!NearestEnemy)
    {
        // 敵がいない → プレイヤーの後方に待機
        const FVector BehindPlayer = PlayerPos - PlayerPawn->GetActorForwardVector() * 150.f;
        const float DistToBehind = FVector::Dist2D(MyPos, BehindPlayer);
        if (DistToBehind > 80.f)
        {
            FVector Dir = (BehindPlayer - MyPos).GetSafeNormal2D();
            CurrentMoveDirection = Dir;
            LastMoveReceivedTime = GetWorld()->GetTimeSeconds();
            GhostOwner->AddMovementInput(Dir, 1.f);

            const FRotator NewRot = FMath::RInterpTo(GhostOwner->GetActorRotation(), Dir.Rotation(), DeltaTime, RotationInterpSpeed);
            GhostOwner->SetActorRotation(NewRot);
        }
        else
        {
            CurrentMoveDirection = FVector::ZeroVector;
        }
        return;
    }

    //敵を囲む目標位置を計算
    //SurroundAngleOffset で各 Ghost が別々の角度から囲む
    const FVector EnemyPos = NearestEnemy->GetActorLocation();
    const FRotator AngleRot(0.f, SurroundAngleOffset, 0.f);
    const FVector  SurroundDir = AngleRot.Vector();
    const FVector  TargetPos = EnemyPos + SurroundDir * SurroundRadius;
    const float    DistToTarget = FVector::Dist2D(MyPos, TargetPos);
    const float    DistToEnemy = FVector::Dist2D(MyPos, EnemyPos);

    if (DistToEnemy <= AttackStopRange)
    {
        //攻撃射程内 → 停止して敵の方を向く
        CurrentMoveDirection = FVector::ZeroVector;
        FVector FaceDir = (EnemyPos - MyPos).GetSafeNormal2D();
        if (!FaceDir.IsNearlyZero())
        {
            const FRotator NewRot = FMath::RInterpTo( GhostOwner->GetActorRotation(), FaceDir.Rotation(), DeltaTime, RotationInterpSpeed);
            GhostOwner->SetActorRotation(NewRot);
        }
    }
    else if (DistToTarget > 50.f)
    {
        //囲み位置へ移動
        FVector Dir = (TargetPos - MyPos).GetSafeNormal2D();
        CurrentMoveDirection = Dir;
        LastMoveReceivedTime = GetWorld()->GetTimeSeconds();
        GhostOwner->AddMovementInput(Dir, 1.f);

        const FRotator NewRot = FMath::RInterpTo(
            GhostOwner->GetActorRotation(), Dir.Rotation(), DeltaTime, RotationInterpSpeed);
        GhostOwner->SetActorRotation(NewRot);
    }
}


//アニメーションを同期する関数
void UGhostPlaybackComponent::SyncAttackAnimFlag()
{
	//AnimBP の攻撃フラグは CombatComponent の状態に完全に同期させる。
	if (AGhostCharacter* Ghost = Cast<AGhostCharacter>(GetOwner()))
	{
		if (Ghost->CombatComponent)
		{
			bAnimAttacking = Ghost->CombatComponent->IsAttacking();
		}
	}
}

// 現在の移動情報を毎フレーム AddMovementInput に渡し続ける。
void UGhostPlaybackComponent::TickMovement(float DeltaTime)
{
	//現在時刻を取得
    const float Now = GetWorld()->GetTimeSeconds();

    // Move レコードが一定時間来なければ方向をクリアして停止
    if (!CurrentMoveDirection.IsNearlyZero())
    {
        if (Now - LastMoveReceivedTime > MoveExpireTime)
        {
            CurrentMoveDirection = FVector::ZeroVector;
        }
    }

	//キャラクタークラスのオーナーを取得
    ACharacter* Owner = Cast<ACharacter>(GetOwner());
    if (!Owner) return;

	//移動方向があれば AddMovementInput を呼び出す
    if (!CurrentMoveDirection.IsNearlyZero())
    {
        Owner->AddMovementInput(CurrentMoveDirection, 1.f);

        // 回転を移動方向に補間
        const FRotator TargetRot = CurrentMoveDirection.Rotation();
        const FRotator NewRot = FMath::RInterpTo(Owner->GetActorRotation(), TargetRot, DeltaTime, RotationInterpSpeed);
        Owner->SetActorRotation(NewRot);
    }
}

//-----------------------------------------------------------------
// 遅延ミラー用なので一旦コメントアウト
//-----------------------------------------------------------------

// タイムスタンプに基づいてキューからアクションを順番に実行する関数
//void UGhostPlaybackComponent::TickRecordedPlayback(float DeltaTime)
//{
//	//キューの終端に達していれば再生完了
//    if (PlaybackIndex >= PlaybackQueue.Num())
//    {
//        //全アクション再生完了 → ImmediatMirror へ遷移
//        CurrentMode = EGhostPlaybackMode::DelayedMirror;
//        LastImmediateFetchTime = GetWorld()->GetTimeSeconds();
//        OnRecordedPlaybackFinished.Broadcast();
//        return;
//    }
//
//	//再生開始からの経過時間
//    const float Elapsed = GetWorld()->GetTimeSeconds() - PlaybackStartTime;
//
//    // 現在時刻以前のアクションをまとめて処理（フレーム落ち対応）
//    while (PlaybackIndex < PlaybackQueue.Num())
//    {
//        const float T = PlaybackQueue[PlaybackIndex].Timestamp - SnapshotStartTime;
//        if (Elapsed < T) break;
//        ExecuteAction(PlaybackQueue[PlaybackIndex]);
//        PlaybackIndex++;
//    }
//}

// TickDelayedMirror
//void UGhostPlaybackComponent::TickDelayedMirror(float DeltaTime)
//{
//    if (!RecorderRef) return;
//
//    const float Now = GetWorld()->GetTimeSeconds();
//    const float FetchBefore = Now - MirrorDelay;
//
//    if (FetchBefore <= LastMirrorFetchTime) return;
//
//    TArray<FGhostActionData> NewActions =
//        RecorderRef->GetActionsSince(LastMirrorFetchTime);
//    LastMirrorFetchTime = FetchBefore;
//
//    for (const FGhostActionData& Action : NewActions)
//    {
//        if (Action.Timestamp > FetchBefore) break;
//        ExecuteAction(Action);
//    }
//}

//--------------------------------------------------------------------------------

//即時ミラー（遅延なし）
void UGhostPlaybackComponent::TickImmediateMirror(float DeltaTime)
{
	//RecorderRef が nullptr なら何もしない
	if (!RecorderRef) return;

    //前回フェッチ以降に追加されたアクションをすべて取得
	TArray<FGhostActionData> NewActions =RecorderRef->GetActionsSince(LastImmediateFetchTime);

	//フェッチしたアクションの中で、MirrorDelay 秒以上前のものをすべて実行
	if (NewActions.Num() == 0) return;

	//すべてのアクションを実行した後のタイムスタンプを次回フェッチの基準にする
	LastImmediateFetchTime = NewActions.Last().Timestamp;

	//フェッチしたアクションをすべて実行
    for (const FGhostActionData& Action : NewActions)
    {
        if (Action.Type == EGhostActionType::Move) continue;
        ExecuteAction(Action);
    }
}

// ExecuteAction：Type ごとに処理を振り分ける
void UGhostPlaybackComponent::ExecuteAction(const FGhostActionData& Action)
{
    switch (Action.Type)
    {
    case EGhostActionType::Move:
        ExecuteMove(Action);
        break;
    case EGhostActionType::Attack:
        ExecuteAttack();
        break;
    case EGhostActionType::Dodge:
        ExecuteDodge(Action);
        break;
    case EGhostActionType::Jump:
        ExecuteJump();
        break;
    case EGhostActionType::Land:
        bAnimDodging = false;
        break;
    }
}

//毎フレーム移動入力を維持するための関数
void UGhostPlaybackComponent::ExecuteMove(const FGhostActionData& Action)
{
	////Action から移動方向を取得
 //   FVector Dir = Action.Direction;

 //   // Direction が空なら記録座標への方向を補完
 //   if (Dir.IsNearlyZero())
 //   {
 //       if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
 //       {
 //           Dir = (Action.Location - Owner->GetActorLocation()).GetSafeNormal2D();
 //       }
 //   }

 //   if (!Dir.IsNearlyZero())
 //   {
 //       CurrentMoveDirection = Dir;
 //       LastMoveReceivedTime = GetWorld()->GetTimeSeconds();
 //   }
}

//攻撃実行関数
void UGhostPlaybackComponent::ExecuteAttack()
{
	//オーナーをキャストして CombatComponent を取得
    AGhostCharacter* Ghost = Cast<AGhostCharacter>(GetOwner());
    if (!Ghost || !Ghost->CombatComponent) return;

    // すでに攻撃中なら無視
    //if (Ghost->CombatComponent->IsAttacking()) return;

    Ghost->CombatComponent->ExecuteAttack();

    //少し遅らせて攻撃モーションが出た後に判定する
    FTimerHandle HitTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(HitTimerHandle, [this, Ghost]()
        {
            if (IsValid(Ghost) && IsValid(Ghost->CombatComponent))
            {
                PerformFriendlyAttackHit(Ghost);
            }
        }, AttackHitDelay, false );
}

//味方フェーズ用：直接ヒット判定
void UGhostPlaybackComponent::PerformFriendlyAttackHit(AGhostCharacter* Ghost)
{
	//Ghost が無効になっていれば何もしない
    if (!Ghost) return;

	//攻撃範囲内のアクターを取得
	FVector Start = Ghost->GetActorLocation();
	FVector End = Start + Ghost->GetActorForwardVector() * FriendlyAttackRange;

	//当たった結果を格納する配列
	TArray<FHitResult> HitResults;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(FriendlyAttackRadius);
	bool bHit = GetWorld()->SweepMultiByChannel(HitResults, Start, End, FQuat::Identity,ECC_Pawn, CollisionShape);
	
	//デバッグ用の描画
#if WITH_EDITOR
    DrawDebugSphere(GetWorld(), Start, FriendlyAttackRadius, 12, FColor::Cyan, false, 1.f);
    DrawDebugSphere(GetWorld(), End, FriendlyAttackRadius, 12, FColor::Blue, false, 1.f);
    DrawDebugLine(GetWorld(), Start, End, FColor::Cyan, false, 1.f);
#endif
    
	//何も当たらなければ終了
    if (!bHit) return;

    // プレイヤーの参照を取得（プレイヤーには当てない）
    AActor* PlayerActor = nullptr;
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        PlayerActor = PC->GetPawn();
    }

	//当たったアクターにダメージを与える
	for (const FHitResult& Hit : HitResults)
	{
		//当たったアクターを取得
		AActor* HitActor = Hit.GetActor();
        if (!HitActor) continue;
        if (HitActor == Ghost) continue;
        if (HitActor == PlayerActor) continue; // プレイヤーはスキップ

        UGameplayStatics::ApplyDamage(HitActor, FriendlyAttackDamage, nullptr, Ghost, UDamageType::StaticClass());

        GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Cyan, FString::Printf(TEXT("[GhostFriend] %s Damage %.1f"), *HitActor->GetName(), FriendlyAttackDamage));
    }
}

// 回避を実行関数
void UGhostPlaybackComponent::ExecuteDodge(const FGhostActionData& Action)
{
    AGhostCharacter* Ghost = Cast<AGhostCharacter>(GetOwner());
    if (!Ghost || !Ghost->CombatComponent) return;

    // 回避方向を先にキャラに向かせる
    if (!Action.Direction.IsNearlyZero())
    {
        Ghost->SetActorRotation(Action.Direction.Rotation());
       
        // 回避中は移動方向もセットしておく
        CurrentMoveDirection = Action.Direction;
        LastMoveReceivedTime = GetWorld()->GetTimeSeconds();
    }

    Ghost->CombatComponent->ExecuteDodge();
    bAnimDodging = true;
}

//ジャンプ実行関数
void UGhostPlaybackComponent::ExecuteJump()
{
    if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
    {
        Owner->Jump();
    }
}