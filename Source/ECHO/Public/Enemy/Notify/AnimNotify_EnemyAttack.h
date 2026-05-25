// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EnemyAttack.generated.h"


UCLASS()
class ECHO_API UAnimNotify_EnemyAttack : public UAnimNotify
{
	GENERATED_BODY()

public:
	//Notify
	virtual void Notify(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation) override;

	FString GetNotifyName_Implementation() const;
};
