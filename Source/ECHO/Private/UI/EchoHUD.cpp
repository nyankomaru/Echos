//EchoHUD.cpp

#include "UI/EchoHUD.h"
#include "Player/ActionCharacter.h"
#include "Player/LockOnComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Canvas.h"

void AEchoHUD::DrawHUD()
{
	Super::DrawHUD();

	//プレイヤーキャラ・LockOnComponentを取得
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	AActionCharacter* Player = Cast<AActionCharacter>(PC->GetPawn());
	if (!Player) return;

	ULockOnComponent* LockOn = Player->FindComponentByClass<ULockOnComponent>();
	if (!LockOn || !LockOn->IsLockedOn()) return;

	AActor* Target = LockOn->GetTarget();
	if (!Target) return;;

	//ターゲットの3D位置をスクリーン座標に変換
	FVector TargetWorldPos = Target->GetActorLocation();

	//GetActorLocation()はキャラの足元なので少し上にオフセット
	TargetWorldPos.Z += 25.f;

	FVector2D ScreenPos;
	if (!PC->ProjectWorldLocationToScreen(TargetWorldPos, ScreenPos)) return;

	//ターゲットとプレイヤーの距離に応じてマーカーサイズを変える
	float Distance = FVector::Dist(Player->GetActorLocation(), Target->GetActorLocation());
	float Scale = MarkerReferenceDistance / FMath::Max(Distance, 1.f);
	float Radius = FMath::Clamp(MarkerBaseRadius * Scale, MarkerMinRadius, MarkerMaxRadius);

	DrawCircle(ScreenPos.X, ScreenPos.Y, Radius);
}

void AEchoHUD::DrawCircle(float CenterX, float CenterY, float Radius)
{
	//円を MarkerSegments 本のLineで近似して描画
	const float AngleStep = 2.f * PI / MarkerSegments;

	for (int32 i = 0; i < MarkerSegments; ++i)
	{
		float A0 = AngleStep * i;
		float A1 = AngleStep * (i + 1);

		float X0 = CenterX + Radius * FMath::Cos(A0);
		float Y0 = CenterY + Radius * FMath::Sin(A0);
		float X1 = CenterX + Radius * FMath::Cos(A1);
		float Y1 = CenterY + Radius * FMath::Sin(A1);

		DrawLine(X0, Y0, X1, Y1, MarkerColor, MarkerThickness);
	}
}
