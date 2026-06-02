//ActionMovementComponent.cpp
//アクションムーブメントコンポーネントソース

#include "Player/ActionMovementComponent.h"

UActionMovementComponent::UActionMovementComponent() :
	MaxFallSpeed(4000.f),	//落下速度
	BaseWalkSpeed(600.f),	//通常スピード
	SprintSpeed(1200.f)		//ダッシュ速度
{
	MaxAcceleration = 2048.f;				// 加速度
	GroundFriction = 8.f;					// 摩擦
	BrakingDecelerationWalking = 2048.f;	// ブレーキ力
	AirControl = 0.35f;						// 空中制御
	GravityScale = 3.f;						// 重力

	BrakingDecelerationFalling = 2048.f;	//空中ブレーキ力

	//初期の移動速度をBaseWalkSpeedに設定
	MaxWalkSpeed = BaseWalkSpeed;
}

void UActionMovementComponent::SetMovementWeight(float NewAcceleration, float NewFriction)
{
	MaxAcceleration = NewAcceleration;
	GroundFriction = NewFriction;
}

void UActionMovementComponent::SetSprinting(bool bIsSprinting)
{
	//引数がtrueならダッシュ速度、falseなら通常速度をMaxWalkSpeedに代入
	MaxWalkSpeed = bIsSprinting ? SprintSpeed : BaseWalkSpeed;
}
