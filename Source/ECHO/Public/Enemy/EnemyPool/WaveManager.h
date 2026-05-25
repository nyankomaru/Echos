#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveManager.generated.h"

class AEnemyPool;
class AEnemyChara;

UCLASS()
class ECHO_API AWaveManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWaveManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnEnemyEliminatedCallback(AEnemyChara* _eliminatedEnemy);

protected:
	UPROPERTY(EditAnywhere, Category = "Wave")
	AEnemyPool* m_pEnemyPool;

	UPROPERTY(EditAnywhere, Category = "Wave")
	int32 m_enemiesPerWave;

	int32 m_remainingEnemiesWave;

private:
	void StartWave();
};
