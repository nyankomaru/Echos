//KariEnemyChar.cpp

#include "Enemy/KariEnemy/KariEnemyChar.h"

AEnemyChar::AEnemyChar()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AEnemyChar::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;
}

float AEnemyChar::TakeDamage(
    float DamageAmount,
    const FDamageEvent& DamageEvent,
    AController* EventInstigator,
    AActor* DamageCauser)
{
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, MaxHealth);

    //ヒットを通知
    OnEnemyHit.Broadcast(DamageAmount);

    UE_LOG(LogTemp, Log, TEXT("EnemyHP: %.1f / %.1f"), CurrentHealth, MaxHealth);

    return ActualDamage;
}
