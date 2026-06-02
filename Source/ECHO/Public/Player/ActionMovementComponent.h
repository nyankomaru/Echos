//ActionMovementComponent.h
//アクションムーブメントコンポーネントヘッダー
//
//移動・物理の拡張コンポーネント

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ActionMovementComponent.generated.h"

UCLASS()
class ECHO_API UActionMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UActionMovementComponent();

	//移動パラメータを動的に変更すための関数例
	void SetMovementWeight(float NewAcceleration, float NewFriction);

	//速度を切り替える
	void SetSprinting(bool bIsSprinting);

protected:
	//必要に応じて毎フレーム移動計算をオーバーライド可能
	//virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunnction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionMovement")
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionMovement")
	float SprintSpeed;

private:
	float MaxFallSpeed;		//落下速度
};
