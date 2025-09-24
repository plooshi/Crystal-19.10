#include "pch.h"
#include "GameMode.h"
#include "Misc.h"
#include "Abilities.h"
#include "API.h"
#include "Creative.h"
#include "Player.h"
#include "Inventory.h"
#include "Looting.h"
#include "Options.h"

UFortPlaylistAthena* GetPlaylist()
{
	if (bDuos)
	{
		return UObject::FindObject<UFortPlaylistAthena>("FortPlaylistAthena Playlist_DefaultDuo.Playlist_DefaultDuo");
	}
	else if (bCreative)
	{
		return Utils::FindObject<UFortPlaylistAthena>(L"/Game/Athena/Playlists/Creative/Playlist_PlaygroundV2.Playlist_PlaygroundV2");
	}
	else
	{
		return Utils::FindObject<UFortPlaylistAthena>(L"/Game/Athena/Playlists/Showdown/Playlist_ShowdownAlt_Solo.Playlist_ShowdownAlt_Solo");
	}
}


void SetPlaylist(AFortGameModeAthena* GameMode) {
	UFortPlaylistAthena* Playlist = GetPlaylist();	
	auto GameState = (AFortGameStateAthena*)GameMode->GameState;

	GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
	GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
	GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
	GameState->CurrentPlaylistInfo.MarkArrayDirty();

	GameState->CurrentPlaylistId = GameMode->CurrentPlaylistId = Playlist->PlaylistId;
	GameMode->CurrentPlaylistName = Playlist->PlaylistName;
	GameState->OnRep_CurrentPlaylistInfo();
	GameState->OnRep_CurrentPlaylistId();

	GameMode->GameSession->MaxPlayers = Playlist->MaxPlayers;

	GameState->AirCraftBehavior = Playlist->AirCraftBehavior;
	GameState->WorldLevel = Playlist->LootLevel;
	GameState->CachedSafeZoneStartUp = Playlist->SafeZoneStartUp;
	
	if (bDuos)
	{
		GameMode->bDBNOEnabled = true;
		//GameMode->bAlwaysDBNO = true;
		GameState->bDBNODeathEnabled = true;
		GameState->SetIsDBNODeathEnabled(true);
	}
	
	GameMode->AISettings = Playlist->AISettings;
	if (GameMode->AISettings)
		GameMode->AISettings->AIServices[1] = UAthenaAIServicePlayerBots::StaticClass();
}

bool bReady = false;
void GameMode::ReadyToStartMatch(UObject* Context, FFrame& Stack, bool* Ret) {
	auto Playlist = GetPlaylist();
	Stack.IncrementCode();
	auto GameMode = Context->Cast<AFortGameModeAthena>();
	if (!GameMode) {
		*Ret = callOGWithRet(((AGameMode*)Context), Stack.CurrentNativeFunction, ReadyToStartMatch);
		return;
	}
	auto GameState = ((AFortGameStateAthena*)GameMode->GameState);
	if (GameMode->CurrentPlaylistId == -1) {
		GameMode->WarmupRequiredPlayerCount = 1;

		SetPlaylist(GameMode);
		if (!bCreative)
		{
			for (auto& Level : Playlist->AdditionalLevels)
			{
				bool Success = false;
				std::cout << "Level: " << Level.Get()->Name.ToString() << std::endl;
				ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), Level, FVector(), FRotator(), &Success, FString(), nullptr);
				FAdditionalLevelStreamed level{};
				level.bIsServerOnly = false;
				level.LevelName = Level.ObjectID.AssetPathName;
				if (Success) GameState->AdditionalPlaylistLevelsStreamed.Add(level);
			}
			for (auto& Level : Playlist->AdditionalLevelsServerOnly)
			{
				bool Success = false;
				std::cout << "Level: " << Level.Get()->Name.ToString() << std::endl;
				ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), Level, FVector(), FRotator(), &Success, FString(), nullptr);
				FAdditionalLevelStreamed level{};
				level.bIsServerOnly = true;
				level.LevelName = Level.ObjectID.AssetPathName;
				if (Success) GameState->AdditionalPlaylistLevelsStreamed.Add(level);
			}
			GameState->OnRep_AdditionalPlaylistLevelsStreamed();
		}

		//GameMode->WarmupCountdownDuration = 10000;

		*Ret = false;
		return;
	}

	if (!GameMode->bWorldIsReady) {
		auto Starts = bCreative ? (TArray<AActor*>) Utils::GetAll<AFortPlayerStartCreative>() : (TArray<AActor*>) Utils::GetAll<AFortPlayerStartWarmup>();
		auto StartsNum = Starts.Num();

		Starts.Free();
		if (StartsNum == 0) {
			*Ret = false;
			return;
		}

		if (!bCreative) {
			if (!GameState->MapInfo) {
				*Ret = false;
				return;
			}
		}
		
		GameState->DefaultParachuteDeployTraceForGroundDistance = 10000;
		AbilitySets.Add(Utils::FindObject<UFortAbilitySet>(L"/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer"));

		
		auto AddToTierData = [&](UDataTable* Table, UEAllocatedVector<FFortLootTierData*>& TempArr) {
			if (!Table)
				return;

			Table->AddToRoot();
			if (auto CompositeTable = Table->Cast<UCompositeDataTable>())
				for (auto& ParentTable : CompositeTable->ParentTables)
					for (auto& [Key, Val] : (TMap<FName, FFortLootTierData*>) ParentTable->RowMap) {
						TempArr.push_back(Val);
					}

			for (auto& [Key, Val] : (TMap<FName, FFortLootTierData*>) Table->RowMap) {
				TempArr.push_back(Val);
			}
			};

		auto AddToPackages = [&](UDataTable* Table, UEAllocatedVector<FFortLootPackageData*>& TempArr) {
			if (!Table)
				return;

			Table->AddToRoot();
			if (auto CompositeTable = Table->Cast<UCompositeDataTable>())
				for (auto& ParentTable : CompositeTable->ParentTables)
					for (auto& [Key, Val] : (TMap<FName, FFortLootPackageData*>) ParentTable->RowMap) {
						TempArr.push_back(Val);
					}

			for (auto& [Key, Val] : (TMap<FName, FFortLootPackageData*>) Table->RowMap) {
				TempArr.push_back(Val);
			}
			};


		UEAllocatedVector<FFortLootTierData*> LootTierDataTempArr;
		auto LootTierData = Playlist->LootTierData.Get();
		if (!LootTierData)
			LootTierData = Utils::FindObject<UDataTable>(L"/Game/Items/Datatables/AthenaLootTierData_Client.AthenaLootTierData_Client");
		if (LootTierData)
			AddToTierData(LootTierData, LootTierDataTempArr);
		for (auto& Val : LootTierDataTempArr)
		{
			Looting::TierDataAllGroups.Add(Val);
		}

		UEAllocatedVector<FFortLootPackageData*> LootPackageTempArr;
		auto LootPackages = Playlist->LootPackages.Get();
		if (!LootPackages) LootPackages = Utils::FindObject<UDataTable>(L"/Game/Items/Datatables/AthenaLootPackages_Client.AthenaLootPackages_Client");
		if (LootPackages)
			AddToPackages(LootPackages, LootPackageTempArr);
		for (auto& Val : LootPackageTempArr)
		{
			Looting::LPGroupsAll.Add(Val);
		}

		for (int i = 0; i < UObject::GObjects->Num(); i++)
		{
			auto Object = UObject::GObjects->GetByIndex(i);

			if (!Object || !Object->Class || Object->IsDefaultObject())
				continue;

			if (auto GameFeatureData = Object->Cast<UFortGameFeatureData>())
			{
				auto LootTableData = GameFeatureData->DefaultLootTableData;
				auto LTDFeatureData = Utils::FindObject<UDataTable>(LootTableData.LootTierData.ObjectID.AssetPathName.GetRawWString().c_str());
				auto AbilitySet = Utils::FindObject<UFortAbilitySet>(GameFeatureData->PlayerAbilitySet.ObjectID.AssetPathName.GetRawWString().c_str());
				auto LootPackageData = Utils::FindObject<UDataTable>(LootTableData.LootPackageData.ObjectID.AssetPathName.GetRawWString().c_str());
				if (AbilitySet) {
					AbilitySet->AddToRoot();
					AbilitySets.Add(AbilitySet);
				}
				if (LTDFeatureData) {
					UEAllocatedVector<FFortLootTierData*> LTDTempData;

					AddToTierData(LTDFeatureData, LTDTempData);

					for (auto& Tag : Playlist->GameplayTagContainer.GameplayTags)
						for (auto& Override : GameFeatureData->PlaylistOverrideLootTableData)
							if (Tag.TagName == Override.First.TagName)
								AddToTierData(Override.Second.LootTierData.Get(), LTDTempData);

					for (auto& Val : LTDTempData)
					{
						Looting::TierDataAllGroups.Add(Val);
					}
				}
				if (LootPackageData) {
					UEAllocatedVector<FFortLootPackageData*> LPTempData;


					AddToPackages(LootPackageData, LPTempData);

					for (auto& Tag : Playlist->GameplayTagContainer.GameplayTags)
						for (auto& Override : GameFeatureData->PlaylistOverrideLootTableData)
							if (Tag.TagName == Override.First.TagName)
								AddToPackages(Override.Second.LootPackageData.Get(), LPTempData);

					for (auto& Val : LPTempData)
					{
						Looting::LPGroupsAll.Add(Val);
					}
				}
			}
		}

		Looting::SpawnFloorLootForContainer(Utils::FindObject<UBlueprintGeneratedClass>(L"/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_Warmup.Tiered_Athena_FloorLoot_Warmup_C"));
		Looting::SpawnFloorLootForContainer(Utils::FindObject<UBlueprintGeneratedClass>(L"/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_01.Tiered_Athena_FloorLoot_01_C"));
		
		UCurveTable* AthenaGameDataTable = GameState->AthenaGameDataTable; // this is js playlist gamedata or default gamedata if playlist doesn't have one

		if (AthenaGameDataTable)
		{
			static FName DefaultSafeZoneDamageName = FName(L"Default.SafeZone.Damage");

			for (const auto& [RowName, RowPtr] : ((UDataTable*)AthenaGameDataTable)->RowMap) // same offset
			{
				if (RowName != DefaultSafeZoneDamageName)
					continue;

				FSimpleCurve* Row = (FSimpleCurve*)RowPtr;

				if (!Row)
					continue;

				for (auto& Key : Row->Keys)
				{
					FSimpleCurveKey* KeyPtr = &Key;

					if (KeyPtr->Time == 0.f)
					{
						KeyPtr->Value = 0.f;
					}
				}

				Row->Keys.Add(FSimpleCurveKey(1.f, 0.01f), 1);
			}
		}

		Utils::ExecHook(L"/Game/Athena/Items/Consumables/Parents/GA_Athena_MedConsumable_Parent.GA_Athena_MedConsumable_Parent_C.Triggered_4C02BFB04B18D9E79F84848FFE6D2C32", Misc::Athena_MedConsumable_Triggered, Misc::Athena_MedConsumable_TriggeredOG);
		SetConsoleTitleA("19.10: Ready");
		GameMode->bWorldIsReady = true;

		if (!bDev)
		{
			API::GameServer("http://185.206.149.110:45011/solstice/api/v1/matchmaking/start/by-address", IP, 7777);	
		}
	}

	*Ret = GameMode->AlivePlayers.Num() >= GameMode->WarmupRequiredPlayerCount;
}

APawn* GameMode::SpawnDefaultPawnFor(UObject* Context, FFrame& Stack, APawn** Ret) {
	AController* NewPlayer;
	AActor* StartSpot;
	Stack.StepCompiledIn(&NewPlayer);
	Stack.StepCompiledIn(&StartSpot);
	Stack.IncrementCode();
	auto GameMode = (AFortGameModeAthena*)Context;
	auto Transform = StartSpot->GetTransform();
	auto Pawn = GameMode->SpawnDefaultPawnAtTransform(NewPlayer, Transform);

	auto PlayerController = NewPlayer->Cast<AFortPlayerController>();
	if (!PlayerController) return *Ret = Pawn;

	static bool IsFirstPlayer = false;

	if (!IsFirstPlayer)
	{
		Utils::Patch<uint8_t>(ImageBase + 0xd6bae4, 0xc3);
		IsFirstPlayer = true;
	}

	auto Num = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num();
	if (Num != 0) {
		PlayerController->WorldInventory->Inventory.ReplicatedEntries.ResetNum();
		PlayerController->WorldInventory->Inventory.ItemInstances.ResetNum();
	}
 
	Inventory::GiveItem(PlayerController, PlayerController->CosmeticLoadoutPC.Pickaxe->WeaponDefinition);
	for (auto& StartingItem : ((AFortGameModeAthena*)GameMode)->StartingItems)
	{
		if (StartingItem.Count && !StartingItem.Item->IsA<UFortSmartBuildingItemDefinition>())
		{
			Inventory::GiveItem(PlayerController, StartingItem.Item, StartingItem.Count);
		}
	}
	
	if (Num == 0)
	{
		auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
		if (!PlayerState) return *Ret = Pawn;

		for (auto& AbilitySet : AbilitySets) Abilities::GiveAbilitySet(PlayerState->AbilitySystemComponent, AbilitySet);

		((void (*)(APlayerState*, APawn*)) (ImageBase + 0x67b715c))(PlayerController->PlayerState, Pawn);


		auto AthenaController = (AFortPlayerControllerAthena*)PlayerController;
		PlayerState->SeasonLevelUIDisplay = AthenaController->XPComponent->CurrentLevel;
		PlayerState->OnRep_SeasonLevelUIDisplay();
		AthenaController->XPComponent->bRegisteredWithQuestManager = true;
		AthenaController->XPComponent->OnRep_bRegisteredWithQuestManager();

		AthenaController->GetQuestManager(ESubGame::Athena)->InitializeQuestAbilities(Pawn);
	}
	
	return *Ret = Pawn;
}

EFortTeam GameMode::PickTeam(AFortGameModeAthena* GameMode, uint8_t PreferredTeam, AFortPlayerControllerAthena* Controller) {
	uint8_t ret = CurrentTeam;

	if (bCreative)
	{
		uint8_t ret = CurrentTeam;

		if (++PlayersOnCurTeam >= 1) {
			CurrentTeam++;
			PlayersOnCurTeam = 0;
		}

		return EFortTeam(ret);
	} else
	{
		if (++PlayersOnCurTeam >= ((AFortGameStateAthena*)GameMode->GameState)->CurrentPlaylistInfo.BasePlaylist->MaxSquadSize) {
			CurrentTeam++;
			PlayersOnCurTeam = 0;
		}

		return EFortTeam(ret);
	}
}

UClass** GetGameSessionClass(AFortGameMode*, UClass** OutClass) {
	*OutClass = AFortGameSessionDedicatedAthena::StaticClass();
	return OutClass;
}

static bool bFirstPLayer = false;
void GameMode::HandleStartingNewPlayer(UObject* Context, FFrame& Stack) {
	AFortPlayerControllerAthena* NewPlayer;
	Stack.StepCompiledIn(&NewPlayer);
	Stack.IncrementCode();
	auto GameMode = (AFortGameModeAthena*)Context;
	auto GameState = (AFortGameStateAthena*)GameMode->GameState;
	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)NewPlayer->PlayerState;

	PlayerState->SquadId = PlayerState->TeamIndex - 3;
	PlayerState->OnRep_SquadId();

	FGameMemberInfo Member;
	Member.MostRecentArrayReplicationKey = -1;
	Member.ReplicationID = -1;
	Member.ReplicationKey = -1;
	Member.TeamIndex = PlayerState->TeamIndex;
	Member.SquadId = PlayerState->SquadId;
	Member.MemberUniqueId = PlayerState->UniqueId;

	GameState->GameMemberInfoArray.Members.Add(Member);
	GameState->GameMemberInfoArray.MarkItemDirty(Member);

	if (!NewPlayer->MatchReport)
	{
		NewPlayer->MatchReport = reinterpret_cast<UAthenaPlayerMatchReport*>(UGameplayStatics::SpawnObject(UAthenaPlayerMatchReport::StaticClass(), NewPlayer));
	}

	if (bCreative && !PlayerState->bIsSpectator)
	{
		bFirstPLayer = true;

		AFortAthenaCreativePortal* Portal = nullptr;
		for (int i = 0; i < GameState->CreativePortalManager->AllPortals.Num(); i++)
		{
			auto CurrentPortal = GameState->CreativePortalManager->AllPortals[i];
			if (!CurrentPortal->GetLinkedVolume() || CurrentPortal->GetLinkedVolume()->VolumeState == ESpatialLoadingState::Ready) continue;
			
			Portal = CurrentPortal;
			break;
		}
		
		if (Portal)
		{
			Portal->OwningPlayer = PlayerState->UniqueId;
			Portal->OnRep_OwningPlayer();

			if (!Portal->bPortalOpen) {
				Portal->bPortalOpen = true;
				Portal->OnRep_PortalOpen();
			}
			
			Portal->PlayersReady.Add(PlayerState->UniqueId);
			Portal->OnRep_PlayersReady();
			
			Portal->bIsPublishedPortal = false;
			Portal->OnRep_PublishedPortal();

			Portal->bUserInitiatedLoad = true;
			Portal->bInErrorState = false;
			
			Portal->IslandInfo.AltTitle = UKismetTextLibrary::Conv_StringToText(L""); //  UKismetTextLibrary::Conv_StringToText(PlayerState->GetPlayerName())
			Portal->IslandInfo.CreatorName = PlayerState->GetPlayerName();
			Portal->IslandInfo.Version = 1;
			Portal->IslandInfo.SupportCode = L"Crystal";
			Portal->IslandInfo.Mnemonic = L"";
			Portal->IslandInfo.ImageUrl = L"";
			Portal->IslandInfo.Privacy = EMMSPrivacy::Public; // idk what this does tbh
			
			Portal->OnRep_IslandInfo();

			Creative::LoadIslands(NewPlayer, PlayerState->GetPlayerName());

			NewPlayer->OwnedPortal = Portal;	
			
			NewPlayer->CreativePlotLinkedVolume = Portal->LinkedVolume;
			NewPlayer->OnRep_CreativePlotLinkedVolume();
 
			auto LevelStreamComponent = Portal->GetLinkedVolume()->GetComponentByClass(UPlaysetLevelStreamComponent::StaticClass())->Cast<UPlaysetLevelStreamComponent>();
			auto LevelSaveComponent = Portal->GetLinkedVolume()->GetComponentByClass(UFortLevelSaveComponent::StaticClass())->Cast<UFortLevelSaveComponent>();
			auto Playset = Utils::FindObject<UFortPlaysetItemDefinition>(L"/Game/Playsets/PID_Playset_60x60_Composed.PID_Playset_60x60_Composed");
			
			if (NewPlayer->CreativePlotLinkedVolume)
			{
				if (LevelSaveComponent)
				{
					LevelSaveComponent->AccountIdOfOwner = PlayerState->UniqueId;
					LevelSaveComponent->PlotPermissions.Permission = EFortCreativePlotPermission::Public;
					LevelSaveComponent->LoadedLinkData = Portal->IslandInfo;
					LevelSaveComponent->bIsLoaded = true;
					LevelSaveComponent->bAutoLoadFromRestrictedPlotDefinition = true;

					if (LevelSaveComponent->LoadedPlot)
					{
						LevelSaveComponent->LoadedPlot->IslandTitle = PlayerState->GetPlayerName();
					}

					LevelSaveComponent->OnRep_LoadedPlotInstanceId();
					LevelSaveComponent->OnRep_LoadedLinkData();
				}
			}
			
			Portal->LinkedVolume->CurrentPlayset = Playset;
			Portal->LinkedVolume->OnRep_CurrentPlayset();
			
			LevelStreamComponent->bAutoLoadLevel = true;
			
			Creative::ShowPlayset(Playset, Portal->GetLinkedVolume());

			Portal->LinkedVolume->VolumeState = ESpatialLoadingState::Ready;
			Portal->LinkedVolume->OnRep_VolumeState();

			NewPlayer->bBuildFree = true;
		}
	}

	return callOG(GameMode, Stack.CurrentNativeFunction, HandleStartingNewPlayer, NewPlayer);
}

void GameMode::OnAircraftEnteredDropZone(UObject* Context, FFrame& Stack) {
	AFortAthenaAircraft* Aircraft;
	Stack.StepCompiledIn(&Aircraft);
	Stack.IncrementCode();

	auto GameMode = (AFortGameModeAthena*)Context;

	if (!bDev)
	{
		API::GameServer("http://185.206.149.110:45011/solstice/api/v1/matchmaking/stop/by-address", IP, 7777);	
	}
	
	callOG(GameMode, Stack.CurrentNativeFunction, OnAircraftEnteredDropZone, Aircraft);
}

void GameMode::OnAircraftExitedDropZone(UObject* Context, FFrame& Stack)
{
	AFortAthenaAircraft* Aircraft;
	Stack.StepCompiledIn(&Aircraft);
	Stack.IncrementCode();

	auto GameMode = (AFortGameModeAthena*)Context;
	auto GameState = (AFortGameStateAthena*)GameMode->GameState;

	for (auto& Player : GameMode->AlivePlayers)
	{
		if (Player->IsInAircraft())
		{
			Player->GetAircraftComponent()->ServerAttemptAircraftJump({});
		}
	}
	
	callOG(GameMode, Stack.CurrentNativeFunction, OnAircraftExitedDropZone, Aircraft);
}


void GameMode::Hook()
{
	Utils::ExecHook(L"/Script/Engine.GameMode.ReadyToStartMatch", ReadyToStartMatch, ReadyToStartMatchOG);
	Utils::ExecHook(L"/Script/Engine.GameModeBase.SpawnDefaultPawnFor", SpawnDefaultPawnFor);
	Utils::Hook(ImageBase + 0x5F9B9C8, PickTeam, PickTeamOG);
	Utils::ExecHook(L"/Script/Engine.GameModeBase.HandleStartingNewPlayer", HandleStartingNewPlayer, HandleStartingNewPlayerOG);
	Utils::ExecHook(L"/Script/FortniteGame.FortGameModeAthena.OnAircraftExitedDropZone", OnAircraftExitedDropZone, OnAircraftExitedDropZoneOG);
	Utils::ExecHook(L"/Script/FortniteGame.FortGameModeAthena.OnAircraftEnteredDropZone", OnAircraftEnteredDropZone, OnAircraftEnteredDropZoneOG);
	if (bGameSessions)
	{
		Utils::Hook(ImageBase + 0x20745F4, GetGameSessionClass);
	}
}
