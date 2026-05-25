//ActionCharacter.h
//アクションキャラクターヘッダー
// 
//プレイヤーのコアクラス
//入力とカメラ制御、各コンポーネント(戦闘・移動・残像)への処理の委譲

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Player/LockOnComponent.h"
#include "Ghost/GhostCharacter.h"
#include "Ghost/GhostRecorderComponent.h"
#include "ActionCharacter.generated.h"

class UCombatComponent;
class USpringArmComponent;
class UCameraComponent;

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

	//入力をバインドする関数（ACharacterの標準機能をオーバーライド）
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//ジャンプが成功したときに呼ばれる関数
	virtual void OnJumped_Implementation() override;

	//地面などに着地した時に呼ばれる関数
	virtual void Landed(const FHitResult& Hit) override;

	// --- インプット ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;

	//攻撃アクション
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* AttackAction;

	//ロックオン入力
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* LockOnAction;

	//移動アクション
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;

	//ジャンプアクション
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* JumpAction;

	//視点移動アクション
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* LookAction;

	//回避アクション
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* DodgeAction;

	//召喚アクション
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SummonAction;

	// --- コンポーネント ---

	//戦闘統括コンポーネント
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCombatComponent* CombatComponent;

	//ロックオンコンポーネント
	UPROPERTY(VisibleAnywhere)
	ULockOnComponent* LockOnComponent;

	//カメラ追従用のスプリングアーム
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	//メインカメラ
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	//プレイヤーの行動を記録するコンポーネント
	UPROPERTY(VisibleAnywhere)
	UGhostRecorderComponent* GhostRecorder;

	// --- 入力コールバック関数 ---

	//攻撃入力時に呼ばれる関数
	void Attack();

	//ロックオン入力時に呼ばれる関数
	void LockOn();

	//移動入力がトリガーされた時に呼ばれる関数
	void Move(const FInputActionValue& Value);

	//視点移動がトリガーされた時に呼ばれる関数
	void Look(const FInputActionValue& Value);

	//回避入力時に呼ばれる関数
	void Dodge();


	//ロックオン状態を切り替える関数
	void OnLockOnChanged(AActor* NewTarget);

	// --- 移動・ダッシュ関連のパラメータ ---

	//オートダッシュへの移行時間
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float TimeToSprint;

	//走り続けている時間を計測する変数
	float CurrentRunTime;

	//走り開始と終了
	void StartSprint();
	void StopSprint();

	// --- 分身召喚システム ---

	//敵に攻撃がヒットした際に通知を受け取り、エネルギーをチャージする
	void OnHitEnemy(float EnergyGain);

	//召喚コスト
	UPROPERTY(VisibleAnywhere, Category = "Ghost")
	float GhostSummonCost;

	//一回の攻撃で得られるエネルギー
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float EnergyGainPerHit;

	//スポーンするGhostCharacterのクラス指定
	UPROPERTY(EditAnywhere, Category = "Ghost")
	TSubclassOf<AGhostCharacter> GhostCharacterClass;

public:
	//現在のエネルギー
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
	float CurrentEnergy;

	//マックスエネルギー
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
	float MaxEnergy;

	//現在召喚中の分身数
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
	int32 ActiveGhostCount;

	//分身の最大数
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost")
	int32 MaxGhostCount;

	//分身の召喚
	UFUNCTION(BlueprintCallable)
	void SummonGhost();

private:
	// --- ジャンプ挙動のカスタマイズ（長押し制御など） ---

	//ジャンプボタンが押された時間
	float JumpPressedTime = 0.f;

	//長ジャンプと判定するためのボタン長押し時間
	UPROPERTY(EditAnywhere, Category = "Jump")
	float JumpHoldThreshold = 0.2f;

	//短ジャンプの上方向最大速度
	UPROPERTY(EditAnywhere, Category = "Jump")
	float JumpZVelocityShort = 500.f;

	//長押しの上方向最大速度
	UPROPERTY(EditAnywhere, Category = "Jump")
	float JumpZVelocityLong = 800.f;

	void OnJumpPressed();	//ボタンを入力時
	void OnJumpReleased();	//ボタンを離したとき

public:
	//自身のMovementComponentをキャストして取得しやすくする
	UFUNCTION(BlueprintCallable, Category = "Movement")
	class UActionMovementComponent* GetActionMovementComponent() const;
};
