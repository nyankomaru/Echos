//GhostCharacter.cpp
//ゴーストキャラクターソース

#include "Ghost/GhostCharacter.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AGhostCharacter::AGhostCharacter() :
	CorruptionDuration(30.f)
{
	PrimaryActorTick.bCanEverTick = false;

	PlaybackComponent = CreateDefaultSubobject<UGhostPlaybackComponent>(TEXT("PlaybackComponent"));

	GhostAIComponent = CreateDefaultSubobject<UGhostAIComponent>(TEXT("GhostAIComponent"));
}

void AGhostCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	//マテリアルをセット
	if (GhostMaterial)
	{
		GetMesh()->SetMaterial(0, GhostMaterial);
	}
}

void AGhostCharacter::InitGhost(const TArray<FGhostActionData>& InActions)
{
	RecordedActions = InActions;

	//浸食タイマー開始
	GetWorldTimerManager().SetTimer(
		CorruptionTimerHandle,
		this,
		&AGhostCharacter::OnCorruptionTimeout,
		CorruptionDuration,
		false
	);

	if (GhostAIComponent)
	{
		EGhostRole GhostRole = GhostAIComponent->AnalyzeRole(InActions);
		GhostAIComponent->SetupByRole(GhostRole);
	}

	//再生開始
	if (PlaybackComponent)
	{
		PlaybackComponent->StartPlayback(InActions);
	}
}

void AGhostCharacter::OnCorruptionTimeout()
{
	//敵化処理を実装
	//とりあえず色を変えて視覚的に確認できるようにしておく
	UE_LOG(LogTemp, Warning, TEXT("GhostCharacter: Corruption triggered!"));

	if (GetMesh())
	{
		// 動的マテリアルを作って赤くする
		UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(0), this);
		if (DynMat)
		{
			DynMat->SetVectorParameterValue(FName("BaseColor"), FLinearColor::Red);
			GetMesh()->SetMaterial(0, DynMat);
		}
	}
}

