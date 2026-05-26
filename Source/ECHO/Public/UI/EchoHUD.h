//EchoHUD.h
//
//ロックオンターゲットのマーカーを描画するHUD

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EchoHUD.generated.h"

UCLASS()
class ECHO_API AEchoHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	//マーカーの基本半径
	UPROPERTY(EditAnywhere, Category = "LockOn Marker")
	float MarkerBaseRadius = 15.f;

	//距離のスケールの基準距離
	UPROPERTY(EditAnywhere, Category = "LockOn Marker")
	float MarkerReferenceDistance = 600.f;

	//マーカーの最少・最大半径
	UPROPERTY(EditAnywhere, Category = "LockOn Marker")
	float MarkerMinRadius = 5.f;

	UPROPERTY(EditAnywhere, Category = "LockOn Marker")
	float MarkerMaxRadius = 15.f;

	//線の太さ
	UPROPERTY(EditAnywhere, Category = "LockOn Marker")
	float MarkerThickness = 2.f;

	//マーカの色
	UPROPERTY(EditAnywhere, Category = "LockOn Marker")
	FLinearColor MarkerColor = FLinearColor::White;

	//円を表示するセグメント数
	int32 MarkerSegments = 16.f;

	//円をDrawLine()で分割描画する
	void DrawCircle(float CenterX, float CenterY, float Radius);
};
