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

	UPROPERTY(EditAnywhere, Category = "GhostTrap|Trigger")
	float TriggerRadius = 140.f;

	UPROPERTY(EditAnywhere, Category = "GhostTrap|Life")
	float LifeTime = 8.f;

	UPROPERTY(EditAnywhere, Category = "GhostTrap|Attack")
	float HitDelay = 0.25f;

	// 変更：攻撃後に消える時間ではなく、攻撃状態の終了時間
	UPROPERTY(EditAnywhere, Category = "GhostTrap|Attack")
	float AttackRecoveryTime = 0.8f;

	// 追加：再発動までのリキャスト時間
	UPROPERTY(EditAnywhere, Category = "GhostTrap|Cooldown")
	float RecastCooldown = 1.5f;

	UPROPERTY(EditAnywhere, Category = "GhostTrap|Attack")
	bool bRotateToTargetOnActivate = true;

	UPROPERTY(EditAnywhere, Category = "GhostTrap|Debug")
	bool bDrawDebug = true;

	UPROPERTY()
	TObjectPtr<AActor> SourceOwner;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> HitActorsThisAttack;

	FGhostTrapAttackData AttackData;

	// 変更：bActivatedは使わない
	bool bIsAttacking = false;
	bool bIsOnCooldown = false;

	FTimerHandle HitTimerHandle;
	FTimerHandle AttackEndTimerHandle;
	FTimerHandle CooldownTimerHandle;

	UFUNCTION()
	void OnTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void ActivateTrap(AActor* TargetActor);

	void FinishAttack();
	void FinishCooldown();

	void CheckInitialOverlaps();

	// 追加：クールダウン終了時などに範囲内の敵を確認する
	void TryActivateFromOverlappingEnemies();

	bool CanActivateTrap() const;
	bool IsValidEnemy(AActor* Actor) const;
	AController* GetSourceController() const;
};