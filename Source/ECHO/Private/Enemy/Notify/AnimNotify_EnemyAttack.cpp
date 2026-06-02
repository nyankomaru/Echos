#include "Enemy/Notify/AnimNotify_EnemyAttack.h"
#include "Components/SkeletalMeshComponent.h"
#include "Enemy/Minion/MinionChara.h"
#include "GameFramework/Actor.h"

void UAnimNotify_EnemyAttack::Notify(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation)
{
	//基底クラスを呼び出す
	Super::Notify(_meshComp, _animation);
	if (!_meshComp) { return; }

	//オーナーを取得し、取得できなかったら処理しない
	AMinionChara* owner = Cast<AMinionChara>(_meshComp->GetOwner());
	if (!owner) { return; }

	owner->Attack();
}

//名前を返す関数
FString UAnimNotify_EnemyAttack::GetNotifyName_Implementation() const
{
	return FString("EnemyAttack");
}
