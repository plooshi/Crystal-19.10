#pragma once
#include "pch.h"
#include "Utils.h"

class XP
{
public:
	static void GiveAccolade(AFortPlayerControllerAthena* PC, UFortAccoladeItemDefinition* AccoladeDef, int Amount = 67);
	DefHookOg(void, SendComplexCustomStatEvent, UFortQuestManager* QuestManager, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, int32 Count);

	InitHooks;
};