// ActionCharacter.h
// プレイヤーのコアクラス
// 入力・カメラ制御・各コンポーネントへの委譲

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Player/GhostRecorderComponent.h"
#include "ActionCharacter.generated.h"

class UCombatComponent;
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
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void OnJumped_Implementation() override;
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
    class UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* DodgeAction;
    void Dodge();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* SummonAction;

    // -----------------------------------------------------------------------
    // 移動
    // -----------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float TimeToSprint;

    float CurrentRunTime;
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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost")
    int32 MaxGhostCount;

private:
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
};