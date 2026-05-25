//GhostRecorderComponent.cpp
//ゴーストレコーダーコンポーネントソース

#include "Ghost/GhostRecorderComponent.h"
#include "Player/CombatComponent.h"

UGhostRecorderComponent::UGhostRecorderComponent() :
	MaxRecordCount(5.f)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGhostRecorderComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	UCombatComponent* Combat = Owner->FindComponentByClass<UCombatComponent>();
	if (!Combat) return;

	Combat->OnGhostActionRecorded.AddUObject(
		this,
		&UGhostRecorderComponent::RecordAction
	);
}

void UGhostRecorderComponent::RecordAction(const FGhostActionData& ActionData)
{
	ActionBuffer.Add(ActionData);
	//PurgeOldActions();

	//GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
	//	FString::Printf(TEXT("RecordAction this: %p, バッファサイズ: %d"), this, ActionBuffer.Num()));

	//件数が上限を超えたら先頭を削除
	if (ActionBuffer.Num() > MaxRecordCount)
	{
		ActionBuffer.RemoveAt(0, ActionBuffer.Num() - MaxRecordCount);
	}
}

TArray<FGhostActionData> UGhostRecorderComponent::GetRecordedActions() const
{
	//GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan,
	//	FString::Printf(TEXT("GetRecordedActionsのインスタンスアドレス: %p, サイズ: %d"), this, ActionBuffer.Num()));

	return ActionBuffer;
}

//void UGhostRecorderComponent::PurgeOldActions()
//{
//	if (ActionBuffer.IsEmpty()) return;
//
//	const float CurrentTime = GetWorld()->GetTimeSeconds();
//	const float Threshold = CurrentTime - MaxRecordDuration;
//
//	//削除対象のインデックスを探す
//	int32 RemoveCount = 0;
//	for (const FGhostActionData& Data : ActionBuffer)
//	{
//		if (Data.Timestamp < Threshold)
//			RemoveCount++;
//		else
//			break;
//	}
//
//	if (RemoveCount > 0)
//	{
//		//RemoveAtの代わりにRemoveAtSwapは使えないので
//		//配列を後ろから作り直す方式にする
//		ActionBuffer = TArray<FGhostActionData>(
//			ActionBuffer.GetData() + RemoveCount,
//			ActionBuffer.Num() - RemoveCount
//		);
//	}
//}
