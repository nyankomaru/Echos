//GhostType.h
//ゴーストタイプヘッダー
//
//残像の状態の構造体を持つクラス

#pragma once

#include "CoreMinimal.h"
//#include "UObject/Class.h"
#include "GhostType.generated.h"

UENUM(BlueprintType)
enum class EGhostActionType : uint8
{
	Move	UMETA(DisplayName = "Move"),
	Attack	UMETA(DisplayName = "Attack"),
	Dodge	UMETA(DisplayName = "Dodge"),
	Jump	UMETA(DisplayName = "Jump"),
	Land	UMETA(DisplayName = "Land"),
};

UENUM(BlueprintType)
enum class EGhostRole : uint8
{
	Attacker UMETA(DisplayName = "Attacker"),
	Dodger   UMETA(DisplayName = "Dodger"),
	Balanced UMETA(DisplayName = "Balanced"),
};

//アクションスナップショット
USTRUCT(BlueprintType)
struct FGhostActionData
{
	GENERATED_BODY()

	//アクションの種類
	UPROPERTY()
	EGhostActionType Type = EGhostActionType::Move;

	//記録時のワールド時刻
	UPROPERTY()
	float Timestamp = 0.f;

	//記録時のワールド座標
	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	//記録の向き
	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	//移動・回避の方向ベクトル
	UPROPERTY()
	FVector Direction = FVector::ZeroVector;

	//再生時に使うアニメーション識別タグ
	UPROPERTY()
	FName AnimationTag = NAME_None;
};
