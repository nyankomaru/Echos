//LockOnComponent.cpp
//ロックオンコンポーネントソース

#include "Player/LockOnComponent.h"
#include "Enemy/EnemyChara.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//切り替えクールダウンを更新
	if (SwitchCooldownTimer > 0.f)
	{
		SwitchCooldownTimer = FMath::Max(0.f, SwitchCooldownTimer - DeltaTime);
	}

	if (!CurrentTarget.IsValid()) return;

	//ターゲットが無効になったら自動解除
	if (!IsTargetValid(CurrentTarget.Get()))
	{
		ClearLockOn();
	}
}

void ULockOnComponent::ToggleLockOn()
{
	if (IsLockedOn())
	{
		ClearLockOn();
	}
	else
	{
		AActor* Best = FindBestTarget();
		if (Best)
		{
			CurrentTarget = Best;
			OnLockOnChanged.Broadcast(Best);
		}
	}
}

void ULockOnComponent::TrySwitchTarget(float StickX)
{
	//ロックオン中であればなにもしない
	if (!IsLockedOn()) return;

	//クールダウン中は無視
	if (SwitchCooldownTimer > 0.f) return;

	//基準値を超えたら切り返し
	if (FMath::Abs(StickX) >= SwitchThreshold && !bSwitchInputActive)
	{
		bSwitchInputActive = true;

		AActor* Next = FindNextTarget(StickX);
		if (Next && Next != CurrentTarget.Get())
		{
			CurrentTarget = Next;
			OnLockOnChanged.Broadcast(Next);
			SwitchCooldownTimer = SwitchCooldown;
		}
	}

	//スティックを戻したらフラグをリセット
	if (FMath::Abs(StickX) < SwitchThreshold * 0.5f)
	{
		bSwitchInputActive;
	}
}

void ULockOnComponent::ClearLockOn()
{
	CurrentTarget = nullptr;
	OnLockOnChanged.Broadcast(nullptr);
}

bool ULockOnComponent::IsTargetValid(AActor* Target) const
{
	if (!Target) return false;
	
	//距離チェック
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return false;

	float Distance = FVector::Dist(
		OwnerCharacter->GetActorLocation(),
		Target->GetActorLocation()
	);

	if (Distance > LockOnBreakRange) return false;

	//死亡チェック（Poolに戻ってもDestroyされてないので必須）
	AEnemyChara* Enemy = Cast<AEnemyChara>(Target);
	if (Enemy && Enemy->GetCurrentState() == EEnemyState::Dead) return false;

	return true;
}

AActor* ULockOnComponent::FindBestTarget() const
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return nullptr;

	//カメラを取得
	UCameraComponent* Camera = OwnerCharacter->FindComponentByClass<UCameraComponent>();
	if (!Camera) return nullptr;

	FVector CameraForward = Camera->GetForwardVector();
	FVector CameraLocation = Camera->GetComponentLocation();

	//全エネミーを取得
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyChara::StaticClass(), Enemies);

	AActor* BestTarget = nullptr;
	float BestDot = -1.f;

	for (AActor* Enemy : Enemies)
	{
		if (!IsTargetValid(Enemy)) continue;

		//距離チェック
		float Distance = FVector::Dist(
			OwnerCharacter->GetActorLocation(),
			Enemy->GetActorLocation()
		);
		if (Distance > LockOnRange) continue;

		//カメラ前方との内積で画面中央に近いものを選ぶ
		FVector ToEnemy = (Enemy->GetActorLocation() - CameraLocation).GetSafeNormal();
		float Dot = FVector::DotProduct(CameraForward, ToEnemy);

		if (Dot > BestDot)
		{
			BestDot = Dot;
			BestTarget = Enemy;
		}
	}

	return BestTarget;
}

AActor* ULockOnComponent::FindNextTarget(float Direction) const
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return nullptr;

	UCameraComponent* Camera = OwnerCharacter->FindComponentByClass<UCameraComponent>();
	if (!Camera) return nullptr;

	//カメラの右ベクトルを使って敵の左右位置を判定
	FVector CameraRight = Camera->GetRightVector();
	FVector CurrentTargetLoc = CurrentTarget->GetActorLocation();

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyChara::StaticClass(), Enemies);

	AActor* BestCandidate = nullptr;

	//現在のターゲットから角度差が最も小さい敵を選ぶ
	float BestScore = FLT_MAX;

	for (AActor* Enemy : Enemies)
	{
		if (!IsTargetValid(Enemy)) continue;
		if (Enemy == CurrentTarget.Get()) continue;

		float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), Enemy->GetActorLocation());
		if (Distance > LockOnRange) continue;

		//現在ターゲットから候補敵への方向を求め、カメラ右ベクトルとの内積で左右を判定
		FVector ToCandidate = (Enemy->GetActorLocation() - CurrentTargetLoc).GetSafeNormal();
		float RightDot = FVector::DotProduct(CameraRight, ToCandidate);

		//入力方向と同じ側にいる敵だけを対象にする
		bool bIsOnCorrectSide = (Direction > 0.f) ? (RightDot > 0.f) : (RightDot < 0.f);
		if (!bIsOnCorrectSide) continue;

		//距離が近いものを優先
		if (Distance < BestScore)
		{
			BestScore = Distance;
			BestCandidate = Enemy;
		}
	}

	return BestCandidate;
}
