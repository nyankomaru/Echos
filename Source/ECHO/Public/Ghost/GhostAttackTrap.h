#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostAttackTrap.generated.h"

class USphereComponent;
class USkeletalMeshComponent;
class UAnimMontage;

/**
 * 残像罠が再生する攻撃情報
 *
 * プレイヤーが行った攻撃内容を保存し、
 * 罠が起動した際に同じ攻撃として再現するためのデータ。
 */
USTRUCT(BlueprintType)
struct FGhostTrapAttackData
{
	GENERATED_BODY()

	// 残像が再生する攻撃モンタージュ
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAnimMontage> Montage = nullptr;

	// 攻撃判定の半径
	UPROPERTY(BlueprintReadWrite)
	float HitRadius = 60.f;

	// 残像の前方にどれだけ攻撃判定を出すか
	UPROPERTY(BlueprintReadWrite)
	float HitRange = 100.f;

	// 攻撃が命中した際に与えるダメージ量
	UPROPERTY(BlueprintReadWrite)
	float Damage = 20.f;

	// 命中時に敵を吹き飛ばす力
	UPROPERTY(BlueprintReadWrite)
	float LaunchForce = 0.f;

	// どのコンボ攻撃かを識別するための番号
	UPROPERTY(BlueprintReadWrite)
	int32 ComboIndex = 0;
};

/**
 * プレイヤーの攻撃を残像罠として配置するActor
 *
 * 敵がTriggerSphereに触れると罠が起動し、
 * 保存していた攻撃データをもとに残像が攻撃を行う。
 */
UCLASS()
class ECHO_API AGhostAttackTrap : public AActor
{
	GENERATED_BODY()

public:
	AGhostAttackTrap();

	/**
	 * 残像罠の初期化
	 *
	 * @param SpawnTransform  罠を配置する位置・回転・スケール
	 * @param InSourceOwner   この罠を設置した元のActor
	 * @param InAttackData    再現する攻撃情報
	 */
	void InitializeTrap(
		const FTransform& SpawnTransform,
		AActor* InSourceOwner,
		const FGhostTrapAttackData& InAttackData);

	/**
	 * 攻撃判定を実行する
	 *
	 * モンタージュ再生後、HitDelay経過後などに呼び出し、
	 * 残像の前方にいる敵へダメージを与える。
	 */
	UFUNCTION(BlueprintCallable, Category = "GhostTrap")
	void CheckTrapHit();

protected:
	virtual void BeginPlay() override;

private:
	// Actorのルートコンポーネント
	UPROPERTY(VisibleAnywhere, Category = "GhostTrap|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// 残像として表示する見た目用のSkeletalMesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GhostTrap|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> GhostMesh;

	// 敵が範囲内に入ったかを検知するトリガー
	UPROPERTY(VisibleAnywhere, Category = "GhostTrap|Components")
	TObjectPtr<USphereComponent> TriggerSphere;

	// 罠が反応する範囲
	UPROPERTY(EditAnywhere, Category = "GhostTrap|Trigger")
	float TriggerRadius = 140.f;

	// 罠が自動で消えるまでの時間
	UPROPERTY(EditAnywhere, Category = "GhostTrap|Life")
	float LifeTime = 8.f;

	// 罠が起動してから攻撃判定を出すまでの遅延時間
	UPROPERTY(EditAnywhere, Category = "GhostTrap|Attack")
	float HitDelay = 0.25f;

	// 攻撃後、罠を破棄するまでの待機時間
	UPROPERTY(EditAnywhere, Category = "GhostTrap|Attack")
	float DestroyDelayAfterAttack = 0.8f;

	// 起動時に対象の敵の方向へ残像を向けるか
	UPROPERTY(EditAnywhere, Category = "GhostTrap|Attack")
	bool bRotateToTargetOnActivate = true;

	// デバッグ用の攻撃判定・範囲表示を行うか
	UPROPERTY(EditAnywhere, Category = "GhostTrap|Debug")
	bool bDrawDebug = true;

	// この罠を設置したActor
	// 自分自身や味方への誤爆判定を避けるためにも使用する
	UPROPERTY()
	TObjectPtr<AActor> SourceOwner;

	// 1回の攻撃中にすでに命中したActor一覧
	// 同じ敵に複数回ダメージが入ることを防ぐ
	UPROPERTY()
	TArray<TObjectPtr<AActor>> HitActorsThisAttack;

	// 罠が再現する攻撃データ
	FGhostTrapAttackData AttackData;

	// 罠がすでに起動済みかどうか
	// 二重起動を防ぐために使用する
	bool bActivated = false;

	// 攻撃判定を遅延実行するためのタイマー
	FTimerHandle HitTimerHandle;

	// 攻撃後に罠を破棄するためのタイマー
	FTimerHandle DestroyTimerHandle;

	/**
	 * TriggerSphereにActorが入ったときに呼ばれる処理
	 *
	 * 入ってきたActorが有効な敵であれば罠を起動する。
	 */
	UFUNCTION()
	void OnTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	/**
	 * 罠を起動する
	 *
	 * 攻撃モンタージュの再生、攻撃判定タイマーの開始、
	 * 必要であれば敵方向への回転を行う。
	 */
	void ActivateTrap(AActor* TargetActor);

	/**
	 * 生成直後、すでにトリガー範囲内に敵がいるか確認する
	 *
	 * 生成時点で敵と重なっている場合でも、
	 * OnBeginOverlapが呼ばれない可能性があるため補助的に使用する。
	 */
	void CheckInitialOverlaps();

	/**
	 * 指定されたActorが攻撃対象として有効な敵か判定する
	 *
	 * SourceOwner自身や無効なActor、
	 * 敵ではないActorを除外するために使用する。
	 */
	bool IsValidEnemy(AActor* Actor) const;

	/**
	 * 罠を設置したActorのControllerを取得する
	 *
	 * ダメージ処理でInstigatorControllerとして渡すために使用する。
	 */
	AController* GetSourceController() const;
};