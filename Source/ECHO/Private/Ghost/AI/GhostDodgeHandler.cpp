//GhostDodgeHandleer.cpp
//ゴーストドッジハンドラーソース

#include "Ghost/AI/GhostDodgeHandler.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UGhostDodgeHandler::Execute(const FGhostActionData& Data, ACharacter* Owner)
{
    if (!Owner) return;

    Owner->SetActorRotation(Data.Rotation);

    UCharacterMovementComponent* MoveComp = Owner->GetCharacterMovement();
    if (!MoveComp) return;

    MoveComp->GravityScale = 0.f;
    MoveComp->GroundFriction = 0.f;

    FVector LaunchVelocity = Data.Direction * 3000.f;
    Owner->LaunchCharacter(LaunchVelocity, true, true);

    GetWorld()->GetTimerManager().SetTimer(
        DodgeEndTimer,
        [Owner, MoveComp]()
        {
            if (Owner && MoveComp)
            {
                MoveComp->GravityScale = 1.f;
                MoveComp->GroundFriction = 8.f;

                FVector Vel = MoveComp->Velocity;
                Vel.X = 0.f;
                Vel.Y = 0.f;
                MoveComp->Velocity = Vel;
            }
        },
        0.1f,
        false
    );
}

