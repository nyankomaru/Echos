//ActionMovementComponent.cpp
//アクションムーブメントコンポーネントソース

#include "Player/ActionMovementComponent.h"

UActionMovementComponent::UActionMovementComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    //アクションゲームらしいきびきびとした挙動にするための初期設定
    MaxAcceleration = 2048.f;             //加速度を高くし、レスポンスを向上
    GroundFriction = 8.f;                 //地面摩擦を高め、滑りを抑える
    BrakingDecelerationWalking = 2048.f;  //歩行時のブレーキ力を強める
    GravityScale = 6.f;                   //重力を強くし、もっさりした滞空時間を減らす
    BrakingDecelerationFalling = 2048.f;  //空中でのブレーキ力

    MaxWalkSpeed = BaseWalkSpeed;         
}

void UActionMovementComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    //空中時の移動制御
    if (IsFalling())
    {
        //空中では進行方向へ強制的にキャラクターを回転させない
        bOrientRotationToMovement = false;

        //回避中でなければ、空中でも入力方向に対して力を加える
        if (!bIsDodging)
        {
            FVector InputVector = GetLastInputVector();
            if (!InputVector.IsNearlyZero())
            {
                //入力方向に力を正規化（斜め速度アップを防ぐ）
                FVector InputDir = InputVector.GetSafeNormal2D();

                //入力方向に力を加算
                Velocity += InputDir * AirPushForce * DeltaTime;

                //水平方向の速度を抽出し、最大速度制限をかける
                FVector HorizontalVelocity = FVector(Velocity.X, Velocity.Y, 0.f);
                if (HorizontalVelocity.Size() > AirMaxHorizontalSpeed)
                {
                    //制限を超えている場合はクランプする
                    HorizontalVelocity = HorizontalVelocity.GetSafeNormal() * AirMaxHorizontalSpeed;
                    Velocity.X = HorizontalVelocity.X;
                    Velocity.Y = HorizontalVelocity.Y;
                }
            }
        }
    }
    else
    {
        //地面にいる時は進行方向へキャラクターを向かわせる
        bOrientRotationToMovement = true;
    }

    //着地硬直の回復処理
    if (bIsLandingRecovery)
    {
        LandingRecoveryElapsed += DeltaTime;

        //回復の進行度を0.0～1.0の範囲で算出
        float Alpha = FMath::Clamp(LandingRecoveryElapsed / LandingRecoveryTime, 0.f, 1.f);

        //最初は重く、後から急速に元の速度に戻るような手触りにする
        float EasedAlpha = FMath::InterpEaseIn(0.f, 1.f, Alpha, 2.f);

        MaxWalkSpeed = FMath::Lerp(
            CachedMaxWalkSpeed * LandingSpeedMultiplier,
            CachedMaxWalkSpeed,
            EasedAlpha);

        //回復時間が終了したら、速度を完全に戻す
        if (Alpha >= 1.f)
        {
            bIsLandingRecovery = false;
            MaxWalkSpeed = CachedMaxWalkSpeed;
        }
    }
}

void UActionMovementComponent::SetMovementWeight(
    float NewAcceleration, float NewFriction)
{
    MaxAcceleration = NewAcceleration;
    GroundFriction = NewFriction;
}

void UActionMovementComponent::SetSprinting(bool _bIsSprinting)
{
    //ダッシュフラグに応じて最大速度を切り替え
    MaxWalkSpeed = _bIsSprinting ? SprintSpeed : BaseWalkSpeed;
    bIsSprinting = _bIsSprinting;
}

void UActionMovementComponent::StartLandingRecovery(bool bWasSprinting)
{
    //着地の状態を元に回復するべき目標速度を保存
    CachedMaxWalkSpeed = bWasSprinting ? SprintSpeed : BaseWalkSpeed;
    LandingRecoveryElapsed = 0.f;
    bIsLandingRecovery = true;
    //着地瞬間の速度を制限
    MaxWalkSpeed = CachedMaxWalkSpeed * LandingSpeedMultiplier;
}