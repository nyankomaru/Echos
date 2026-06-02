// GhostEnemyComponent.cpp

#include "Ghost/GhostEnemy/GhostEnemyComponent.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostCharacter/GhostPlaybackComponent.h"

UGhostEnemyComponent::UGhostEnemyComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGhostEnemyComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UGhostEnemyComponent::StartLifetimeTimer()
{
    ElapsedTime = 0.f;
    bTimerActive = true;
    SetComponentTickEnabled(true);
    SetState(EGhostLifecycleState::Friendly);
}

void UGhostEnemyComponent::TickComponent(
    float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bTimerActive) return;

    ElapsedTime += DeltaTime;

    if (CurrentState == EGhostLifecycleState::Friendly)
    {
        if (ElapsedTime >= LifetimeSeconds - WarningThreshold)
            SetState(EGhostLifecycleState::Warning);
    }
    else if (CurrentState == EGhostLifecycleState::Warning)
    {
        if (ElapsedTime >= LifetimeSeconds)
        {
            SetState(EGhostLifecycleState::Corrupted);
            Corrupt();
        }
    }
}

float UGhostEnemyComponent::GetRemainingTime() const
{
    return FMath::Max(0.f, LifetimeSeconds - ElapsedTime);
}

float UGhostEnemyComponent::GetRemainingTimeNormalized() const
{
    if (LifetimeSeconds <= 0.f) return 0.f;
    return FMath::Clamp(GetRemainingTime() / LifetimeSeconds, 0.f, 1.f);
}

void UGhostEnemyComponent::SetState(EGhostLifecycleState NewState)
{
    if (CurrentState == NewState) return;
    CurrentState = NewState;
    OnStateChanged.Broadcast(NewState);
}

void UGhostEnemyComponent::Corrupt()
{
    AGhostCharacter* Ghost = Cast<AGhostCharacter>(GetOwner());
    if (!Ghost) return;

    if (Ghost->PlaybackComponent)
        Ghost->PlaybackComponent->SetEnemyMode();

    // SpawnAIFromControllerClass ¨ ActivateEnemyAI ‚É“ˆê
    Ghost->ActivateEnemyAI();

    SetState(EGhostLifecycleState::Enemy);

    bTimerActive = false;
    SetComponentTickEnabled(false);
}