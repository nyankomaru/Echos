//GhostPlaybackComponent.cpp
//ゴーストプレイバックコンポーネントソース

#include "Ghost/GhostPlaybackComponent.h"
#include "Ghost/GhostCharacter.h"
#include "Ghost/AI/GhostActionHandlerBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UGhostPlaybackComponent::UGhostPlaybackComponent() :
	CurrentIndex(0),
	PlaybackStartTime(0.f),
	RecordStartTime(0.f)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGhostPlaybackComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGhostPlaybackComponent::StartPlayback(const TArray<FGhostActionData>& InActions)
{
	if (InActions.IsEmpty()) return;

	Actions = InActions;
	CurrentIndex = 0;
	PlaybackStartTime = GetWorld()->GetTimeSeconds();

	//記録の先頭Timestampを基準にする
	RecordStartTime = Actions[0].Timestamp;

	ScheduleNextAction();
}

void UGhostPlaybackComponent::ScheduleNextAction()
{
	if (CurrentIndex >= Actions.Num())
	{
		//全アクション再生完了、先頭からループ
		CurrentIndex = 0;
		PlaybackStartTime = GetWorld()->GetTimeSeconds();
		ScheduleNextAction();

		//ループせず待機（今は止まるだけ）
		//GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::White, TEXT("Ghost: 再生完了"));
		return;
	}

	const FGhostActionData& Next = Actions[CurrentIndex];

	//記録時の相対時間を計算
	float RecordRelativeTime = Next.Timestamp - RecordStartTime;

	//再生時間からの経過時間と比較して待機時間を決める
	float Now = GetWorld()->GetTimeSeconds();
	float PlaybackElapsed = Now - PlaybackStartTime;
	float Delay = FMath::Max(0.f, RecordRelativeTime - PlaybackElapsed);

	GetWorld()->GetTimerManager().SetTimer(
		PlaybackTimerHandle,
		this,
		&UGhostPlaybackComponent::ExecuteCurrentAction,
		FMath::Max(Delay, 0.01f),
		false
	);
}

void UGhostPlaybackComponent::ExecuteCurrentAction()
{
	if (CurrentIndex >= Actions.Num()) return;

	const FGhostActionData& Data = Actions[CurrentIndex];

	//ハンドラを検索して実行
	TArray<UGhostActionHandlerBase*> Handlers;
	GetOwner()->GetComponents<UGhostActionHandlerBase>(Handlers);

	bool bHandled = false;
	for (UGhostActionHandlerBase* Handler : Handlers)
	{
		if (Handler && Handler->GetHandledType() == Data.Type)
		{
			Handler->Execute(Data, Cast<ACharacter>(GetOwner()));
			bHandled = true;
			break;
		}
	}

	if (!bHandled)
	{
		UE_LOG(LogTemp, Warning, TEXT("GhostPlayback: ハンドラが見つかりません Type=%d"),
			(int32)Data.Type);
	}

	CurrentIndex++;
	ScheduleNextAction();
}

//void UGhostPlaybackComponent::PlayMove(const FGhostActionData& Data)
//{
//	ACharacter* Owner = Cast<ACharacter>(GetOwner());
//	if (!Owner) return;
//
//	//記録した位置に向かって移動入力を与える
//	FVector CurrentLocation = Owner->GetActorLocation();
//	FVector TargetLocation = Data.Location;
//	FVector Direction = (TargetLocation - CurrentLocation);
//
//	//向きを記録時の回転に合わせる
//	Owner->SetActorRotation(Data.Rotation);
//
//	if (!Direction.IsNearlyZero())
//	{
//		Owner->AddMovementInput(Direction, 1.f);
//	}
//}
//
//void UGhostPlaybackComponent::PlayAttack(const FGhostActionData& Data)
//{
//	ACharacter* Owner = Cast<ACharacter>(GetOwner());
//	if (!Owner) return;
//
//	//向きだけ合わせる
//	Owner->SetActorRotation(Data.Rotation);
//
//	//AnimationTagを使ってモンタージュを再生する
//	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("Ghost: Attack再生"));
//}
//
//void UGhostPlaybackComponent::PlayDodge(const FGhostActionData& Data)
//{
//	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue,
//		FString::Printf(TEXT("Dodge Direction: X=%.2f Y=%.2f Z=%.2f"),
//			Data.Direction.X, Data.Direction.Y, Data.Direction.Z));
//
//	ACharacter* Owner = Cast<ACharacter>(GetOwner());
//	if (!Owner) return;
//
//	Owner->SetActorRotation(Data.Rotation);
//
//	UCharacterMovementComponent* MoveComp = Owner->GetCharacterMovement();
//	if (!MoveComp) return;
//
//	//重力と摩擦を一時的に0にする
//	MoveComp->GravityScale = 0.f;
//	MoveComp->GroundFriction = 0.f;
//
//	//記録した方向に回避の力を与える
//	FVector LaunchVelocity = Data.Direction * 3000.f;
//	Owner->LaunchCharacter(LaunchVelocity, true, true);
//
//	FTimerHandle DodgeEndTimer;
//	GetWorld()->GetTimerManager().SetTimer(
//		DodgeEndTimer,
//		[Owner, MoveComp]()
//		{
//			if (Owner && MoveComp)
//			{
//				MoveComp->GravityScale = 1.f;
//				MoveComp->GroundFriction = 8.f;
//
//				FVector Vel = MoveComp->Velocity;
//				Vel.X = 0.f;
//				Vel.Y = 0.f;
//				MoveComp->Velocity = Vel;
//			}
//		},
//		0.1f,
//		false
//	);
//
//	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Blue, TEXT("Ghost: Dodge再生"));
//}