// Copyright JETBLU, Inc. All Rights Reserved.

#include "ExecCalc_Damage.h"

#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/Manager/LetheGameplayTags.h"

/*

추후 ExecCalc 사용에 Attribute가 필요한 경우 주석처럼 사용하면 됩니다.

struct FLetheDamageStatics
{
	// 해당 클래스의 로직 내에서 사용할 Attribute를 여기에서 선언합니다.
	FGameplayEffectAttributeCaptureDefinition SourceAttack;
	FGameplayEffectAttributeCaptureDefinition TargetArmor;

	FLetheDamageStatics()
	// 아래는 Attribute를 가져오는 규칙에 대한 명시입니다.
	SourceAttack(ULetheAttributeSet::GetAttackAttribute(), EGameplayEffectAttributeCaptureSource::Source, false),
	TargetArmor(ULetheAttributeSet::GetArmorAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
	{
	}
};

static const FLetheDamageStatics& DamageStatics()
{
	static FLetheDamageStatics DStatics;
	return DStatics;
}

*/

UExecCalc_Damage::UExecCalc_Damage()
{
	// 여기서 '이 Attribute가 계산에 관련이 있다'고 GAS에게 알려줍니다.
	//RelevantAttributesToCapture.Add(DamageStatics().SourceAttack);
	//RelevantAttributesToCapture.Add(DamageStatics().TargetArmor);
}

void UExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// 지금 적용 중인 GameplayEffectSpec을 가져옵니다.
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// 시전자와 피격자의 태그 정보를 가져와서 평가 파라미터에 세팅합니다.
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// 구조체와 생성자에서 명시한 규칙대로 Attribute 값을 가져옵니다.
	//float SourceAttack = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().SourceAttack, EvaluationParameters, SourceAttack);

	//float TargetArmor = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().TargetArmor, EvaluationParameters, TargetArmor);

	float Damage = 0;
	//Damage = FMath::Clamp(Damage, 0.f, Damage);

	// 현재 데미지 타입과 일치하는 데미지를 탐색합니다.
	for (const FGameplayTag& DamageTypeTag : FLetheGameplayTags::Get().DamageTypeTags)
	{
		const float DamageTypeValue = Spec.GetSetByCallerMagnitude(DamageTypeTag, false);

		if (DamageTypeValue > 0.f)
		{
			// 데미지 계산 결과 반영 후 반복문을 빠져나갑니다.
			Damage += DamageTypeValue;
			break;
		}
	}

	// IncomingDamage Attribute에 Damage만큼 Additive 연산을 적용하라는 Modifier 데이터를 생성합니다.
	const FGameplayModifierEvaluatedData EvaluatedData(ULetheAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, Damage);
	// 이번 ExecCalc의 결과로 Modifier를 Output에 추가합니다.
	OutExecutionOutput.AddOutputModifier(EvaluatedData);
}
