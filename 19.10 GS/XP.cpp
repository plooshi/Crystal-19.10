#include "pch.h"
#include "XP.h"

#include "API.h"
#include "Options.h"
#include "Results.h"

void XP::GiveAccolade(AFortPlayerControllerAthena* Controller, UFortAccoladeItemDefinition* Accolade, int Amount)
{
	return;
	if (!Controller || !Controller->XPComponent || !Accolade) 
		return;

	FXPEventInfo Info{};

	Info.Accolade = UKismetSystemLibrary::GetPrimaryAssetIdFromObject(Accolade);
	float XpValue = Utils::EvaluateScalableFloat(Accolade->XpRewardAmount);

	if (XpValue == 0)
		UDataTableFunctionLibrary::EvaluateCurveTableRow(Accolade->XpRewardAmount.Curve.CurveTable, FName(L"Default_Medal"), Accolade->XpRewardAmount.Value, nullptr, &XpValue, FString());

	Info.EventXpValue = (int32)XpValue;
	Info.RestedValuePortion = Info.EventXpValue;
	Info.RestedXPRemaining = Info.EventXpValue;
	Info.TotalXpEarnedInMatch = Info.EventXpValue + Controller->XPComponent->TotalXpEarned;
	Info.Priority = Accolade->Priority;
	Info.SimulatedText = Accolade->GetShortDescription();
	Info.SimulatedName = Accolade->GetDisplayName(true);
	Info.EventName = Accolade->Name;
	Info.SeasonBoostValuePortion = 0;
	Info.AccoladeLargePreviewImageOverride = Accolade->GetLargePreviewImage();
	Info.PlayerController = Controller;

	Controller->XPComponent->MatchXp += Info.EventXpValue;
	Controller->XPComponent->TotalXpEarned += Info.EventXpValue;

	FAthenaAccolades AthenaAccolade{};
	AthenaAccolade.AccoladeDef = Accolade;
	AthenaAccolade.Count = 1;
	auto str = UEAllocatedWString(L"AthenaAccolade:") + Accolade->Name.ToWString();
	FString TemplateId;
	TemplateId.Reserve((int) str.size() + 1);
	for (auto& c : str)
	{
		TemplateId.Add(c);
	}
	AthenaAccolade.TemplateId = TemplateId;

	for (auto& AccoladeToReplace : Accolade->AccoladeToReplace)
	{
		UFortAccoladeItemDefinition* AccoladeToRemove = AccoladeToReplace;
		auto AthenaAccoladeIndex = Controller->XPComponent->PlayerAccolades.SearchIndex([AccoladeToRemove](FAthenaAccolades& item)
			{ return item.AccoladeDef == AccoladeToRemove; });

		auto MedalIndex = Controller->XPComponent->MedalsEarned.SearchIndex([AccoladeToRemove](UFortAccoladeItemDefinition* item)
			{ return item == AccoladeToRemove; });

		if (AthenaAccoladeIndex != -1)
			Controller->XPComponent->PlayerAccolades.Remove(AthenaAccoladeIndex);

		if (MedalIndex != -1)
			Controller->XPComponent->MedalsEarned.Remove(MedalIndex);
	}

	Controller->XPComponent->PlayerAccolades.Add(AthenaAccolade);

	if (Accolade->AccoladeType == EFortAccoladeType::Medal)
	{
		Controller->XPComponent->MedalsEarned.Add(Accolade);
		Controller->XPComponent->ClientMedalsRecived(Controller->XPComponent->PlayerAccolades);
	}

	Controller->XPComponent->OnXpEvent(Info);
}
static inline std::map<UFortAccoladeItemDefinition*, bool> OnceOnlyMap;


struct FParseConditionResult
{
	bool bMatch;
	size_t NextStart;
};

static FParseConditionResult ParseCondition(UEAllocatedString Condition, const FGameplayTagContainer& TargetTags, const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& ContextTags)
{
	size_t CondTypeStart = -1, CondTypeEnd = -1, NextCond = -1;

	for (auto& c : Condition)
	{
		if (c == '>' || c == '<' || c == '=' || c == '&')
		{
			CondTypeStart = __int64(&c - Condition.data());
		}
		else if (CondTypeStart != -1 && (c != '>' && c != '<' && c != '=' && c != '&'))
		{
			CondTypeEnd = __int64(&c - Condition.data());
		}
		else if (CondTypeEnd != -1 && (c == '=' || c == '&'))
		{
			NextCond = __int64(&c - Condition.data());
		}

		if (CondTypeStart != -1 && CondTypeEnd != -1 && NextCond != -1)
			break;
	}

	if (CondTypeStart == Condition.npos)
	{
		CondTypeStart = Condition.find(" ");

		if (CondTypeStart == Condition.npos)
			return { false, NextCond };

		CondTypeStart++;

		if (CondTypeEnd == Condition.npos)
			CondTypeEnd = Condition.find(" ", CondTypeStart);

		NextCond = Condition.find("&&", CondTypeEnd + 1);
		if (NextCond == Condition.npos)
			NextCond = Condition.find("||", CondTypeEnd + 1);
	}
	else if (CondTypeEnd == Condition.npos)
	{
		CondTypeEnd = CondTypeStart + 1;
	}

	auto Left = Condition.substr(0, CondTypeStart - 1);
	auto CondType = Condition.substr(CondTypeStart, CondTypeEnd - CondTypeStart);
	auto Right = Condition.substr(CondTypeEnd + 1, NextCond == Condition.npos ? NextCond : (Condition.substr(NextCond - 1, 1) == " " ? NextCond - 1 : NextCond) - CondTypeEnd - 1);

	if (CondType == "HasTag" || CondType == "MissingTag")
	{
		FGameplayTagContainer Container;

		if (Left == "Target.Tags")
		{
			Container = TargetTags;
		}
		else if (Left == "Source.Tags")
		{
			Container = SourceTags;
		}
		else if (Left == "Context.Tags")
		{
			Container = ContextTags;
		}
		else
		{
			return { false, NextCond };
		}

		FGameplayTag Tag(FName(UEAllocatedWString(Right.begin(), Right.end()).c_str()));

		if (CondType == "HasTag")
		{
			return { Container.HasTag(Tag), NextCond };
		}
		else
		{
			return { !Container.HasTag(Tag), NextCond };
		}
	}
	else
	{
		Log(L"Hit unimplemented condition: ", Left, " ", CondType, " ", Right);
	}
	return { false, NextCond };
}

static bool IsConditionMet(const FString& InCondition, const FGameplayTagContainer& TargetTags, const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& ContextTags)
{
	auto Condition = InCondition.ToString();

	if (Condition.empty())
		return true;

	FParseConditionResult Result = ParseCondition(Condition, TargetTags, SourceTags, ContextTags);

	if (Result.NextStart != Condition.npos)
	{
	loop:
		if (Result.NextStart == Condition.npos)
			return Result.bMatch;
		auto Start = Condition.substr(Result.NextStart, 2);

		if (Start == "&&")
		{
			Condition = Condition.substr(Result.NextStart + 2);

			if (Condition.substr(0, 1) == " ")
				Condition = Condition.substr(1);

			auto LastResult = Result;

			Result = ParseCondition(Condition, TargetTags, SourceTags, ContextTags);

			if (!LastResult.bMatch || !Result.bMatch)
				Result.bMatch = false;

			goto loop;
		}
		else if (Start == "||")
		{
			Condition = Condition.substr(Result.NextStart + 2);

			if (Condition.substr(0, 1) == " ")
				Condition = Condition.substr(1);

			auto LastResult = Result;

			Result = ParseCondition(Condition, TargetTags, SourceTags, ContextTags);

			if (LastResult.bMatch || Result.bMatch)
				Result.bMatch = true;

			goto loop;
		}
		else
		{
			return Result.bMatch;
		}
	}

	return Result.bMatch;
}


static inline std::map<AFortPlayerControllerAthena*, std::vector<FFortMcpQuestObjectiveInfo>> ObjCompArray;
static void ProgressQuest(AFortPlayerControllerAthena* PlayerController, UFortQuestManager* QuestManager, UFortQuestItem* QuestItem, UFortQuestItemDefinition* QuestDefinition, FName BackendName, int32 PCount, EFortQuestObjectiveStatEvent StatEvent)
{
	auto Count = QuestManager->GetObjectiveCompletionCount(QuestDefinition, BackendName);
	auto Obj = QuestDefinition->Objectives.Search([BackendName](FFortMcpQuestObjectiveInfo& item) { return item.BackendName == BackendName; });

	bool allObjsCompleted = false;
	if (Obj->Count == Count + 1) {
		allObjsCompleted = true;
		ObjCompArray[PlayerController].push_back(*Obj);

		auto CompletionCount = 0;
		for (auto& Obj : QuestDefinition->Objectives)
		{
			bool Found = false;
			for (auto& ObjComp : ObjCompArray[PlayerController])
			{
				if (Obj.BackendName == ObjComp.BackendName)
				{
					Found = true;
					break;
				}
			}
			if (!Found)
			{
				allObjsCompleted = false;
			}
			else CompletionCount++;
		}
		if (QuestDefinition->ObjectiveCompletionCount != 0 && QuestDefinition->ObjectiveCompletionCount == CompletionCount + 1) allObjsCompleted = true;
	}

	if (Count <= Obj->Count) {
		Log(L"wow");
		auto PlayerState = PlayerController->PlayerState->Cast<AFortPlayerStateAthena>();

		if (PlayerState)
		{
			for (const auto& TeamMember : PlayerState->PlayerTeam->TeamMembers)
			{
				auto TeamMemberPC = (AFortPlayerControllerAthena*)TeamMember;
				if (TeamMemberPC->IsA<AFortAthenaAIBotController>())
					continue;
				auto TeamMemberQuestManager = TeamMemberPC->GetQuestManager(ESubGame::Athena);
				TeamMemberQuestManager->HandleQuestObjectiveUpdated(TeamMemberPC, QuestDefinition, BackendName, Count + 1, 1,
					TeamMemberPC == PlayerController ? nullptr : PlayerState, Count == Obj->Count, allObjsCompleted);
			}
		}
	}
	Count = QuestManager->GetObjectiveCompletionCount(QuestDefinition, BackendName);
	if (allObjsCompleted) {
		int32 XPCount = 0;

		if (auto RewardsTable = QuestDefinition->RewardsTable)
		{
			static auto Name = FName(L"Default");
			auto DefaultRow = RewardsTable->RowMap.Search([](FName& RName, uint8* Row) { return RName == Name; });
			XPCount = (*(FFortQuestRewardTableRow**)DefaultRow)->Quantity;
		}

		if (XPCount) {
			/*FXPEventEntry QuestEntry;
			QuestEntry.EventXpValue = XPCount;
			QuestEntry.QuestDef = QuestDefinition;
			QuestEntry.Time = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			QuestEntry.TotalXpEarnedInMatch = PlayerController->XPComponent->TotalXpEarned + XPCount;
			QuestEntry.SimulatedXpEvent = UKismetTextLibrary::Conv_StringToText(L"Objective completed");*/

			FXPEventInfo EventInfo{};
			EventInfo.EventXpValue = XPCount;
			EventInfo.RestedValuePortion = EventInfo.EventXpValue;
			EventInfo.RestedXPRemaining = EventInfo.EventXpValue;
			EventInfo.TotalXpEarnedInMatch = EventInfo.EventXpValue + PlayerController->XPComponent->TotalXpEarned;
			EventInfo.QuestDef = QuestDefinition;
			EventInfo.TotalXpEarnedInMatch = PlayerController->XPComponent->TotalXpEarned + XPCount;
			EventInfo.SimulatedText = UKismetTextLibrary::Conv_StringToText(L"Objective completed");
			EventInfo.PlayerController = PlayerController;

			PlayerController->XPComponent->ChallengeXp += XPCount;
			PlayerController->XPComponent->TotalXpEarned += XPCount;
			PlayerController->XPComponent->OnXpUpdated(PlayerController->XPComponent->CombatXp, PlayerController->XPComponent->SurvivalXp, PlayerController->XPComponent->MedalBonusXP, PlayerController->XPComponent->ChallengeXp, PlayerController->XPComponent->MatchXp, PlayerController->XPComponent->TotalXpEarned);

			PlayerController->XPComponent->InMatchProfileVer++;
			PlayerController->XPComponent->OnInMatchProfileUpdate(PlayerController->XPComponent->InMatchProfileVer);
			PlayerController->XPComponent->OnProfileUpdated();

			PlayerController->XPComponent->OnXpEvent(EventInfo);
		}
	}

	if (allObjsCompleted)
		QuestManager->ClaimQuestReward(QuestItem);
}

void SendStatEvent(UFortQuestManager* QuestManager, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, bool* QuestActive, bool* QuestCompleted, int32 Count, EFortQuestObjectiveStatEvent StatEvent)
{
	auto PlayerController = (AFortPlayerControllerAthena*)QuestManager->GetPlayerControllerBP();

	if (!PlayerController)
		return;

	FGameplayTagContainer PlayerSourceTags;
	FGameplayTagContainer ContextTags;

	QuestManager->GetSourceAndContextTags(&PlayerSourceTags, &ContextTags);

	ContextTags.AppendTags(((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->CurrentPlaylistInfo.BasePlaylist->GameplayTagContainer);
	AdditionalSourceTags.AppendTags(PlayerSourceTags);	

	static UDataTable* AthenaObjectiveStatXPTable = Utils::FindObject<UDataTable>(L"/Game/Athena/Items/Quests/AthenaObjectiveStatXPTable.AthenaObjectiveStatXPTable");

	if (AthenaObjectiveStatXPTable)
	{
		for (const auto& [RowName, RowPtr] : AthenaObjectiveStatXPTable->RowMap)
		{
			auto Row = (FFortQuestObjectiveStatXPTableRow*)RowPtr;

			if (Row->Type != StatEvent || (Row->CountThreshhold > 0 && Row->CountThreshhold < Count) || Count > Row->MaxCount)
				continue;

			if (!TargetTags.HasAll(Row->TargetTags))
				continue;

			if (!AdditionalSourceTags.HasAll(Row->SourceTags))
				continue;

			if (!ContextTags.HasAll(Row->ContextTags))
				continue;

			if (!IsConditionMet(Row->Condition, TargetTags, AdditionalSourceTags, ContextTags))
				continue;

			static auto AccoladeName = FName(L"Accolades");

			if (Row->AccoladePrimaryAssetId.PrimaryAssetType.Name == AccoladeName)
			{
				auto AccoladeToGive = (UFortAccoladeItemDefinition*)UKismetSystemLibrary::GetObjectFromPrimaryAssetId(Row->AccoladePrimaryAssetId);

				if (!AccoladeToGive)
					continue;

				if (Row->bOnceOnly)
				{
					if (OnceOnlyMap[AccoladeToGive])
						continue;

					OnceOnlyMap[AccoladeToGive] = true;
				}

				XP::GiveAccolade(PlayerController, AccoladeToGive, 67);
			}
		}
	}

	for (auto& CurrentQuest : QuestManager->CurrentQuests)
	{
		if (CurrentQuest->HasCompletedQuest())
			continue;

		auto QuestDef = CurrentQuest->GetQuestDefinitionBP();

		if (!QuestDef)
			continue;

		if (QuestManager->HasCompletedQuest(QuestDef))
			continue;

		// Utils::Log("QuestDef: " + QuestDef->GetName());

		for (auto& Objective : QuestDef->Objectives)
		{
			if (QuestManager->HasCompletedObjectiveWithName(QuestDef, Objective.BackendName) ||
				QuestManager->HasCompletedObjective(QuestDef, Objective.ObjectiveStatHandle) ||
				CurrentQuest->HasCompletedObjectiveWithName(Objective.BackendName) ||
				CurrentQuest->HasCompletedObjective(Objective.ObjectiveStatHandle))
			{
				continue;
			}

			// Utils::Log("BackendName: " + Objective.BackendName.ToString());

			auto StatTable = Objective.ObjectiveStatHandle.DataTable;
			auto& StatRowName = Objective.ObjectiveStatHandle.RowName;

			if (!StatTable || !StatRowName.IsValid())
			{
				// Utils::Log("Using InlineObjectiveStats!");

				for (auto& ObjectiveStat : Objective.InlineObjectiveStats)
				{
					if (ObjectiveStat.Type != StatEvent)
						continue;
					bool bFoundCorrectQuest = true; // start with true and set to false if we dont contain

					for (auto& TagCondition : ObjectiveStat.TagConditions)
					{
						if (!TagCondition.Require || !bFoundCorrectQuest)
							continue;

						switch (TagCondition.Type)
						{
						case EInlineObjectiveStatTagCheckEntryType::Target:
						{
							if (!ObjectiveStat.bHasInclusiveTargetTags)
								break;

							if (!TargetTags.HasTag(TagCondition.Tag))
								bFoundCorrectQuest = false;

							break;
						}
						case EInlineObjectiveStatTagCheckEntryType::Source:
						{
							if (!ObjectiveStat.bHasInclusiveSourceTags)
								break;

							if (!AdditionalSourceTags.HasTag(TagCondition.Tag))
								bFoundCorrectQuest = false;

							break;
						}
						case EInlineObjectiveStatTagCheckEntryType::Context:
						{
							if (!ObjectiveStat.bHasInclusiveContextTags)
								break;

							if (!ContextTags.HasTag(TagCondition.Tag))
								bFoundCorrectQuest = false;

							break;
						}
						default:
							break;
						}
					}

					if (!bFoundCorrectQuest)
						continue;

					if (!IsConditionMet(ObjectiveStat.Condition, TargetTags, AdditionalSourceTags, ContextTags))
						continue;


					Log(L"aa2 %s", CurrentQuest->Name.ToWString().c_str());
					//if (bFoundCorrectQuest)
					ProgressQuest(PlayerController, QuestManager, CurrentQuest, QuestDef, Objective.BackendName, Count, StatEvent);
				}
			}
			else if (StatTable && StatRowName.ComparisonIndex)
			{
				auto& RowMap = StatTable->RowMap;

				for (const auto& [RowName, RowPtr] : RowMap)
				{
					if (RowName == StatRowName)
					{
						auto Row = (FFortQuestObjectiveStatTableRow*)RowPtr;

						if (Row->Type != StatEvent)
							continue;

						if (!TargetTags.HasAll(Row->TargetTagContainer))
							continue;

						if (!AdditionalSourceTags.HasAll(Row->SourceTagContainer))
							continue;

						if (!ContextTags.HasAll(Row->ContextTagContainer))
							continue;

						if (!IsConditionMet(Row->Condition, TargetTags, AdditionalSourceTags, ContextTags))
							continue;

						// todo implement count

						Log(L"aa %s", CurrentQuest->Name.ToWString().c_str());
						ProgressQuest(PlayerController, QuestManager, CurrentQuest, QuestDef, Objective.BackendName, Count, StatEvent);
					}
				}
			}
		}
	}
}

void XP::SendComplexCustomStatEvent(UFortQuestManager* QuestManager, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, int32 Count)
{
	std::cout << "dsaasdasdasdasdasdasdasdsasdasdasd" << std::endl;
	//std::cout << TargetObject->GetFullName() << std::endl;
	std::cout << Count << std::endl;

	SendStatEvent(QuestManager, TargetObject, AdditionalSourceTags, TargetTags, nullptr, nullptr, Count, EFortQuestObjectiveStatEvent::ComplexCustom);
}

void XP::Hook()
{
	//Utils::Hook(ImageBase + 0x67EA55C, SendComplexCustomStatEvent, SendComplexCustomStatEventOG);
}