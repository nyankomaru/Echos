//WeaponBase.h
//ウエポンベースヘッダー
//
//武器メッシュの保持、当たり判定切り替え
//アニメーションモンタージュのリスト


#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

UCLASS()
class ECHO_API AWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeaponBase();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
