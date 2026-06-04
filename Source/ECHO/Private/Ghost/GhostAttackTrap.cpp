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
	TryActivateFromOverlappingEnemies();
}

void AGhostAttackTrap::OnTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!CanActivateTrap()) return;
	if (!IsValidEnemy(OtherActor)) return;

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
	if (!CanActivateTrap()) return;

	bIsAttacking = true;
	bIsOnCooldown = false;

	// 1回の攻撃ごとにヒット済みリストをリセット
	HitActorsThisAttack.Empty();

	if (bRotateToTargetOnActivate && TargetActor)
	{
		FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.f;

		if (!ToTarget.IsNearlyZero())
		{
			SetActorRotation(ToTarget.GetSafeNormal().Rotation());
		}
	}

	float MontageLength = 0.f;

	if (AttackData.Montage && GhostMesh)
	{
		if (UAnimInstance* AnimInstance = GhostMesh->GetAnimInstance())
		{
			MontageLength = AnimInstance->Montage_Play(AttackData.Montage);
		}
	}

	// 攻撃判定タイマー
	GetWorld()->GetTimerManager().SetTimer(
		HitTimerHandle,
		this,
		&AGhostAttackTrap::CheckTrapHit,
		HitDelay,
		false
	);

	// 攻撃終了タイマー
	const float ActualAttackEndTime = FMath::Max3(
		AttackRecoveryTime,
		MontageLength,
		HitDelay + 0.05f
	);

	GetWorld()->GetTimerManager().SetTimer(
		AttackEndTimerHandle,
		this,
		&AGhostAttackTrap::FinishAttack,
		ActualAttackEndTime,
		false
	);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			1.2f,
			FColor::Cyan,
			TEXT("GhostTrap Attack")
		);
	}
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

bool AGhostAttackTrap::CanActivateTrap() const
{
	return !bIsAttacking && !bIsOnCooldown;
}

void AGhostAttackTrap::TryActivateFromOverlappingEnemies()
{
	if (!CanActivateTrap()) return;
	if (!TriggerSphere) return;

	TArray<AActor*> OverlappingActors;
	TriggerSphere->GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		if (IsValidEnemy(Actor))
		{
			ActivateTrap(Actor);
			return;
		}
	}
}

void AGhostAttackTrap::FinishAttack()
{
	bIsAttacking = false;
	bIsOnCooldown = true;

	GetWorld()->GetTimerManager().SetTimer(
		CooldownTimerHandle,
		this,
		&AGhostAttackTrap::FinishCooldown,
		RecastCooldown,
		false
	);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			1.2f,
			FColor::Blue,
			TEXT("GhostTrap Cooldown")
		);
	}
}

void AGhostAttackTrap::FinishCooldown()
{
	bIsOnCooldown = false;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			1.2f,
			FColor::Green,
			TEXT("GhostTrap Ready")
		);
	}

	// クールダウン終了時に、まだ敵が範囲内にいるなら再発動
	TryActivateFromOverlappingEnemies();
}