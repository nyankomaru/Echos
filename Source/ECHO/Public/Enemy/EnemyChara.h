#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyChara.generated.h"

class UBehaviorTree;
class UPawnSensingComponent;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyEliminated, AEnemyChara*, EliminatedEnemy);

//敵のステート
UENUM(BlueprintType)
enum class EEnemyState : uint8 
{
	Idle UMETA(DisplayName = "Idle"),
	Patrol UMETA(DisplayName = "Patrol"),
	Chase UMETA(DisplayName = "Chase"),
	Attack UMETA(DisplayName = "Attack"),
	Stunned UMETA(DisplayName = "Stunned"),
	Dead UMETA(DisplayName = "Dead")
};

UCLASS()
class ECHO_API AEnemyChara : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyChara();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//ダメージ用
	virtual float TakeDamage(float _damageAmount, FDamageEvent const& _damageEvent, AController* _eventInstigator, AActor* _damageCauser) override;

	//現在の体力を取得
	float GetCurrentHP() { return m_currentHP; }

	//体力をセットする
	void SetHP(float _hp) { m_currentHP = FMath::Clamp(_hp, 0.f, m_maxHP); }

	//体力をリセットする
	void RestetHP() { m_currentHP = m_maxHP; }

public:
	//攻撃関数
	virtual void Attack();

	//倒された時の関数
	virtual void Die();

	void ResetTimer();

	//ビヘイビアーを取得
	FORCEINLINE UBehaviorTree* GetBehaviorTree() const { return m_behaviorTree; }

	//ターゲットを取得
	FORCEINLINE AActor* GetCurrentTarget() const { return m_targetActor; }

	//ターゲットを設定
	FORCEINLINE void SetTargetActor(AActor* _actor) { m_targetActor = _actor; }

	//現在のステートを取得
	FORCEINLINE EEnemyState GetCurrentState() const { return m_currentState;  }

	//現在のステートを設定
	FORCEINLINE void SetCurrentState(EEnemyState _state) { m_currentState = _state; }

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEnemyEliminated m_onEnemyEliminated;

protected:
	//プレイヤーを見つけたときの関数
	UFUNCTION()
	void OnSeePawn(APawn* _pawn);

protected:
	//現在の体力
	float m_currentHP;

	//最大の体力
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,  Category = "Health")
	float m_maxHP;

	//攻撃範囲
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,  Category = "Attack")
	float m_attackRange;

	//ダメージ
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,  Category = "Damage")
	float m_damage;

	//攻撃クールダウン
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,  Category = "Attack")
	float m_cooldownTimer;

	//動きスピード
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Speed")
	float m_moveSpeed;

	//現在の状態
	UPROPERTY(BlueprintReadOnly,  Category = "AI")
	EEnemyState m_currentState = EEnemyState::Idle;

	//ターゲットアクター
	UPROPERTY(BlueprintReadOnly,  Category = "TargetActor")
	AActor* m_targetActor;

	//ビヘイビアツリー
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	UBehaviorTree* m_behaviorTree;

	//感知コンポーネント
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,  Category = "AI")
	UPawnSensingComponent* m_pawnSensing;

	//攻撃アニメーション
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* m_attackMontage;

	FTimerHandle m_attackTimerHandle;
};
