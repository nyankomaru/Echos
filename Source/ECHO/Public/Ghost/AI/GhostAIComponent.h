//GhostAIComponent.h
//ゴーストエーアイコンポーネントヘッダー
//

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Ghost/GhostType.h"
#include "BehaviorTree/BehaviorTree.h"
#include "GhostAIComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ECHO_API UGhostAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGhostAIComponent();

	//GhostCharacterのInitGhostから呼ぶ
	void SetupByRole(EGhostRole);

protected:
	virtual void BeginPlay() override;

private:
	//ロール設定
	//EGhostRole AnalyzeRole(const TArray<FGhostActionData>& Actions);

	//ロール別ビヘイビアツリー
	UPROPERTY(EditAnywhere, Category = "GhostAI")
	UBehaviorTree* BT_Attacker;

	UPROPERTY(EditAnywhere, Category = "GhostAI")
	UBehaviorTree* BT_Dodger;

	UPROPERTY(EditAnywhere, Category = "GhostAI")
	UBehaviorTree* BT_Balanced;

public:	
	//GhostChharacterから呼ぶ
	EGhostRole AnalyzeRole(const TArray<FGhostActionData>& Actions) const;
};
