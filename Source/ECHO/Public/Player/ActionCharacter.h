// ActionCharacter.h
// プレイヤーのコアクラス
// 入力・カメラ制御・各コンポーネントへの委譲

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Player/LockOnComponent.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Player/GhostRecorderComponent.h"
#include "Player/CombatComponent.h"
#include "Ghost/GhostAttackTrap.h"
#include "ActionCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UGhostManagerComponent;

UCLASS()
class ECHO_API AActionCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AActionCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	//オートダッシュの計測に使う
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	//ジャンプが成功したときに呼ばれる関数
	virtual void OnJumped_Implementation() override;
	//地面などに着地したときに呼ばれる関数
	virtual void Landed(const FHitResult& Hit) override;

	// -----------------------------------------------------------------------
	// 入力
	// -----------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* AttackAction;
	void Attack();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;
	void Move(const FInputActionValue& Value);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* LookAction;
	void Look(const FInputActionValue& Value);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* LockOnAction;
	void LockOn();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* DodgeAction;
	void Dodge();

	//召喚アクション
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SummonAction;

	//　追加　罠召喚
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* GhostTrapAction;

	void UseGhostAttackTrap();

	// -----------------------------------------------------------------------
	// 移動
	// -----------------------------------------------------------------------

	//オートダッシュへの移行時間
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float TimeToSprint;

	//走り続けている時間を計測する変数
	float CurrentRunTime;
	//走り開始と終了
	void StartSprint();
	void StopSprint();

	// -----------------------------------------------------------------------
	// カメラ
	// -----------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	// -----------------------------------------------------------------------
	// 戦闘
	// -----------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* CombatComponent;

public:
	UFUNCTION(BlueprintCallable, Category = "Movement")
	class UActionMovementComponent* GetActionMovementComponent() const;

	// -----------------------------------------------------------------------
	// ロックオン処理
	// -----------------------------------------------------------------------

		//ロックオンコンポーネント
	UPROPERTY(VisibleAnywhere)
	ULockOnComponent* LockOnComponent;

	//ロックオン状態を切り替える関数
	void OnLockOnChanged(AActor* NewTarget);

	//ロックオン中のカメラ補間速度
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_LockOnCameraInterpSpeed;

	//ロックオン中のカメラの距離オフセット
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_LockOnCameraOffsetY;

	//ロックオン中カメラPitchの下限
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_LockOnPitchMin;

	//ロックオン中カメラPitchの上限
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_LockOnPitchMax;

	FVector DefaultSocketOffset;

	//ロックオン中のかどうかのフラグ
	bool m_bsLockedOn;

	//ロックオン中のカメラの更新
	void UpdateLockOnCamera(float DeltaTime);

	// -----------------------------------------------------------------------
	// Ghost システム
	// -----------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Ghost")
	void SummonGhost();

	// エネルギー（UI 表示用に public）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
	float CurrentEnergy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost")
	float MaxEnergy;

	// 現在召喚中の分身数（UI 表示用）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
	int32 ActiveGhostCount;

	//機能してません（6月2日）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost")
	int32 MaxGhostCount;

private:
	//敵に攻撃がヒットした際に通知を受け取り、エネルギーをチャージする
	void OnHitEnemy(float EnergyGain);

	// プレイヤー行動を記録するコンポーネント
	UPROPERTY(VisibleAnywhere, Category = "Ghost")
	UGhostRecorderComponent* GhostRecorder;

	// 分身の召喚・管理コンポーネント（TrySummon 経由で利用可能）
	UPROPERTY(VisibleAnywhere, Category = "Ghost")
	UGhostManagerComponent* GhostManager;

	// 召喚コスト
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float GhostSummonCost;

	// ヒット時のエネルギー増加量
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float EnergyGainPerHit;

	// スポーンする GhostCharacter クラス（BP で BP_GhostCharacter を設定）
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = "true"), Category = "Ghost")
	TSubclassOf<AGhostCharacter> GhostCharacterClass;

	// -----------------------------------------------------------------------
	// ジャンプ挙動のカスタマイズ（長押し制御など）
	// -----------------------------------------------------------------------

	//ジャンプボタンが押された時間
	float JumpPressedTime = 0.f;

	//長ジャンプと判定するためのボタン長押し時間
	UPROPERTY(EditAnywhere, Category = "Jump")
	float JumpHoldThreshold = 0.2f;

	//短ジャンプの上方向最大速度
	UPROPERTY(EditAnywhere, Category = "Jump")
	float JumpZVelocityShort = 1500.f;

	//長押しの上方向最大速度
	UPROPERTY(EditAnywhere, Category = "Jump")
	float JumpZVelocityLong = 800.f;

	void OnJumpPressed();	//ボタンを入力時
	void OnJumpReleased();	//ボタンを離したとき

	// -----------------------------------------------------------------------
	// 追加　Ghost Trap System
	// -----------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Trap", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AGhostAttackTrap> GhostAttackTrapClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Trap", meta = (AllowPrivateAccess = "true"))
	bool bEnableGhostAttackTrap = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Trap", meta = (AllowPrivateAccess = "true"))
	float GhostAttackTrapCost = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Trap", meta = (AllowPrivateAccess = "true"))
	int32 MaxGhostAttackTrapCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Trap", meta = (AllowPrivateAccess = "true"))
	float GhostAttackTrapBackOffset = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost|Trap", meta = (AllowPrivateAccess = "true"))
	bool bHasRecordedGhostTrapAttack = false;

	UPROPERTY()
	FGhostTrapAttackData LastGhostTrapAttackData;

	UPROPERTY()
	TArray<TObjectPtr<AGhostAttackTrap>> ActiveGhostAttackTraps;

	void RecordLastComboStep(int32 ComboIndex, const FComboStepData& ComboStep);
	bool SpawnGhostAttackTrapFromAttackData(const FGhostTrapAttackData& TrapAttackData);
	void CleanupGhostAttackTraps();

};