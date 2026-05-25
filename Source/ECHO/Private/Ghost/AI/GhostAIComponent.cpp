//GhostAIComponent.cpp
//ゴーストエーアイコンポーネントソース

#include "Ghost/AI/GhostAIComponent.h"
#include "Ghost/GhostCharacter.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "Ghost/AI/GhostAIController.h"

#include "DrawDebugHelpers.h"//画面デバッグ用
//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("A"));

UGhostAIComponent::UGhostAIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGhostAIComponent::BeginPlay()
{
	Super::BeginPlay();
}

EGhostRole UGhostAIComponent::AnalyzeRole(const TArray<FGhostActionData>& Actions) const
{
	if (Actions.IsEmpty()) return EGhostRole::Balanced;

	int32 AttackCount = 0;
	int32 DodgeCount = 0;

	for (const FGhostActionData& A : Actions)
	{
		if (A.Type == EGhostActionType::Attack) AttackCount++;
		if (A.Type == EGhostActionType::Dodge) DodgeCount++;
	}

	float AttackRatio = (float)AttackCount / Actions.Num();

	if (AttackRatio >= 0.7f) return EGhostRole::Attacker;
	if (AttackRatio >= 0.7f) return EGhostRole::Dodger;
	return EGhostRole::Balanced;
}

void UGhostAIComponent::SetupByRole(EGhostRole Role)
{
	AGhostAIController* AIC = Cast<AGhostAIController>(
		Cast<APawn>(GetOwner())->GetController());

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
		AIC ? TEXT("SetupByRole: AIC取得成功") : TEXT("SetupByRole: AIC取得失敗"));

	if (!AIC)
	{
		UE_LOG(LogTemp, Warning, TEXT("GhostAIComponent: AIControllerが見つかりません"));
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Con NO"));
		return;
	}

	UBehaviorTree* SelectedBT = nullptr;
	switch(Role)
	{
	case EGhostRole::Attacker: SelectedBT = BT_Attacker; break;
	case EGhostRole::Dodger: SelectedBT = BT_Dodger; break;
	case EGhostRole::Balanced: SelectedBT = BT_Balanced; break;
	}

	if (!SelectedBT)
	{
		UE_LOG(LogTemp, Warning, TEXT("GhostAIComponent: BTが設定されていません"));
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("BT NO"));
		return;
	}

	AIC->RunBehaviorTree(SelectedBT);
}
