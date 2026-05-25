#include "Enemy/Notify/AnimNotify_EnemyAttack.h"
#include "Components/SkeletalMeshComponent.h"
#include "Enemy/Minion/MinionChara.h"
#include "GameFramework/Actor.h"

void UAnimNotify_EnemyAttack::Notify(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation)
{
	//基底クラスを呼び出す
	Super::Notify(_meshComp, _animation);
	if (!_meshComp) { return; }

	AMinionChara* owner = Cast<AMinionChara>(_meshComp);
	if (!owner) { return; }

	owner->Attack();
}

FString UAnimNotify_EnemyAttack::GetNotifyName_Implementation() const
{
	return FString("EnemyAttack");
}
