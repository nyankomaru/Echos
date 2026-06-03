#include "Ghost/GhostAttackTrap.h"

#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Enemy/EnemyChara.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"

AGhostAttackTrap::AGhostAttackTrap()
{
	// Tick処理は使用しない
	// 罠はオーバーラップとタイマーで動作するため、毎フレーム更新は不要
	PrimaryActorTick.bCanEverTick = false;

	// ルートコンポーネントを作成
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// 残像の見た目として使用するSkeletalMesh
	GhostMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GhostMesh"));
	GhostMesh->SetupAttachment(SceneRoot);

	// 見た目用なので、GhostMesh自体には当たり判定を持たせない
	GhostMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 敵が罠の範囲に入ったことを検知するトリガー
	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->SetupAttachment(SceneRoot);
	TriggerSphere->SetSphereRadius(TriggerRadius);

	// TriggerSphereは物理衝突ではなく、オーバーラップ検知のみ行う
	TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// 罠は動的なオブジェクトとして扱う
	TriggerSphere->SetCollisionObjectType(ECC_WorldDynamic);

	// 基本的にはすべてのチャンネルを無視する
	TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);

	// Pawnのみオーバーラップ対象にする
	// 敵キャラクターがPawnである想定
	TriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// オーバーラップイベントを有効化
	TriggerSphere->SetGenerateOverlapEvents(true);
}

void AGhostAttackTrap::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerSphere)
	{
		// Blueprint側でTriggerRadiusを変更した場合でも、
		// BeginPlay時に正しい半径が反映されるようにする
		TriggerSphere->SetSphereRadius(TriggerRadius);

		// 敵がTriggerSphereに入ったときに呼ばれる関数を登録
		TriggerSphere->OnComponentBeginOverlap.AddDynamic(
			this,
			&AGhostAttackTrap::OnTriggerBeginOverlap
		);
	}

	// 一定時間経過後、罠を自動で削除する
	// 起動しなかった罠が残り続けることを防ぐ
	SetLifeSpan(LifeTime);
}

void AGhostAttackTrap::InitializeTrap(
	const FTransform& SpawnTransform,
	AActor* InSourceOwner,
	const FGhostTrapAttackData& InAttackData)
{
	// この罠を設置したActorを保存
	// 自分自身への誤爆防止や、ダメージ発生元の取得に使用する
	SourceOwner = InSourceOwner;

	// 罠が再現する攻撃データを保存
	AttackData = InAttackData;

	// 指定された位置・回転・スケールに罠を配置
	SetActorTransform(SpawnTransform);

	// 生成時点ですでに敵が範囲内にいる場合の取りこぼし対策
	// Spawn直後はBeginOverlapが発生しないケースがあるため、次のTickで範囲確認を行う
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			this,
			&AGhostAttackTrap::CheckInitialOverlaps
		);
	}
}

void AGhostAttackTrap::CheckInitialOverlaps()
{
	// すでに起動済み、またはTriggerSphereが無効なら処理しない
	if (bActivated || !TriggerSphere) return;

	// 現在TriggerSphereと重なっているActorを取得
	TArray<AActor*> OverlappingActors;
	TriggerSphere->GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		// 有効な敵がいれば即座に罠を起動
		if (IsValidEnemy(Actor))
		{
			ActivateTrap(Actor);
			return;
		}
	}
}

void AGhostAttackTrap::OnTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	// 二重起動を防ぐ
	if (bActivated) return;

	// 触れたActorが攻撃対象として有効な敵でなければ無視
	if (!IsValidEnemy(OtherActor)) return;

	// 有効な敵が範囲に入ったため罠を起動
	ActivateTrap(OtherActor);
}

bool AGhostAttackTrap::IsValidEnemy(AActor* Actor) const
{
	// 無効なActorは対象外
	if (!Actor) return false;

	// 自分自身は対象外
	if (Actor == this) return false;

	// 罠を設置した本人は対象外
	if (Actor == SourceOwner) return false;

	// AEnemyCharaであれば攻撃対象として扱う
	return Cast<AEnemyChara>(Actor) != nullptr;
}

void AGhostAttackTrap::ActivateTrap(AActor* TargetActor)
{
	// 二重起動を防ぐ
	if (bActivated) return;
	bActivated = true;

	// 今回の攻撃で命中したActorリストを初期化
	// 1回の攻撃中に同じ敵へ複数回ダメージが入らないようにする
	HitActorsThisAttack.Empty();

	if (TriggerSphere)
	{
		// 罠が起動した後は、再度オーバーラップしないように当たり判定を無効化
		TriggerSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (bRotateToTargetOnActivate && TargetActor)
	{
		// 対象の敵がいる方向を計算
		FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();

		// 上下方向の差は無視し、水平面上だけで向きを決める
		ToTarget.Z = 0.f;

		// 有効な方向ベクトルであれば、罠を敵の方向へ向ける
		if (!ToTarget.IsNearlyZero())
		{
			SetActorRotation(ToTarget.GetSafeNormal().Rotation());
		}
	}

	// 再生したモンタージュの長さ
	// 罠を消すタイミングの計算に使用する
	float MontageLength = 0.f;

	if (AttackData.Montage && GhostMesh)
	{
		// GhostMeshのAnimInstanceを取得し、攻撃モンタージュを再生する
		if (UAnimInstance* AnimInstance = GhostMesh->GetAnimInstance())
		{
			MontageLength = AnimInstance->Montage_Play(AttackData.Montage);
		}
	}

	// 罠起動後、少し遅らせて攻撃判定を実行する
	// モンタージュの攻撃タイミングに合わせるための遅延
	GetWorld()->GetTimerManager().SetTimer(
		HitTimerHandle,
		this,
		&AGhostAttackTrap::CheckTrapHit,
		HitDelay,
		false
	);

	// 攻撃後の削除待機時間とモンタージュ時間を比較し、長い方を採用
	// モンタージュ再生中に罠が消えてしまうことを防ぐ
	const float DestroyDelay = FMath::Max(DestroyDelayAfterAttack, MontageLength);

	// 罠を削除するためのタイマー処理
	// BindWeakLambdaにすることで、Actor破棄済みの場合の安全性を高める
	FTimerDelegate DestroyDelegate;
	DestroyDelegate.BindWeakLambda(this, [this]()
		{
			Destroy();
		});

	// 指定時間後に罠を削除
	GetWorld()->GetTimerManager().SetTimer(
		DestroyTimerHandle,
		DestroyDelegate,
		DestroyDelay,
		false
	);
}

void AGhostAttackTrap::CheckTrapHit()
{
	// 攻撃判定の開始位置
	const FVector Start = GetActorLocation();

	// 残像の前方方向に攻撃判定を伸ばした終点
	const FVector End = Start + GetActorForwardVector() * AttackData.HitRange;

	// Sweepでヒットした結果を格納する配列
	TArray<FHitResult> HitResults;

	// 球状の攻撃判定を作成
	FCollisionShape Sphere = FCollisionShape::MakeSphere(AttackData.HitRadius);

	// Sweep判定用のパラメータ
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GhostAttackTrapHit), false);

	// 自分自身は攻撃判定から除外
	Params.AddIgnoredActor(this);

	if (SourceOwner)
	{
		// 罠を設置した本人も攻撃判定から除外
		Params.AddIgnoredActor(SourceOwner);
	}

	if (bDrawDebug)
	{
		// 攻撃判定の開始地点を表示
		DrawDebugSphere(GetWorld(), Start, AttackData.HitRadius, 12, FColor::Cyan, false, 1.f);

		// 攻撃判定の終点を表示
		DrawDebugSphere(GetWorld(), End, AttackData.HitRadius, 12, FColor::Blue, false, 1.f);

		// 攻撃判定の移動ラインを表示
		DrawDebugLine(GetWorld(), Start, End, FColor::Cyan, false, 1.f);
	}

	// StartからEndまで球状のSweep判定を行う
	// ECC_Pawnを対象にして、敵キャラクターへの命中を調べる
	const bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		Sphere,
		Params
	);

	// 何もヒットしていなければ処理終了
	if (!bHit) return;

	for (const FHitResult& Hit : HitResults)
	{
		// ヒットしたActorを取得
		AActor* HitActor = Hit.GetActor();

		// 攻撃対象として無効なActorは無視
		if (!IsValidEnemy(HitActor)) continue;

		// 同じ攻撃中にすでに命中している敵は無視
		// 多段ヒット化してしまうことを防ぐ
		if (HitActorsThisAttack.Contains(HitActor)) continue;

		// この敵に命中済みであることを記録
		HitActorsThisAttack.Add(HitActor);

		// UE標準のダメージ処理を実行
		// GetSourceController()を渡すことで、罠を設置したプレイヤーの攻撃として扱える
		UGameplayStatics::ApplyDamage(
			HitActor,
			AttackData.Damage,
			GetSourceController(),
			this,
			UDamageType::StaticClass()
		);

		if (AttackData.LaunchForce > 0.f)
		{
			// 吹き飛ばし対象がCharacterであればLaunchCharacterを使用する
			if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
			{
				// 罠から敵へ向かう方向を吹き飛ばし方向にする
				FVector LaunchDir = (HitActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();

				// 少しだけ上方向成分を加える
				LaunchDir.Z = 0.2f;

				// 敵を吹き飛ばす
				HitCharacter->LaunchCharacter(
					LaunchDir * AttackData.LaunchForce,
					true,
					true
				);
			}
		}

		if (GEngine)
		{
			// デバッグ用に命中したことを画面表示
			GEngine->AddOnScreenDebugMessage(
				-1,
				1.5f,
				FColor::Cyan,
				FString::Printf(TEXT("GhostTrap Hit! ComboIndex: %d"), AttackData.ComboIndex)
			);
		}
	}
}

AController* AGhostAttackTrap::GetSourceController() const
{
	// 罠を設置したActorがPawnであればControllerを取得する
	const APawn* SourcePawn = Cast<APawn>(SourceOwner);

	// ダメージ処理のInstigatorControllerとして使用する
	return SourcePawn ? SourcePawn->GetController() : nullptr;
}