#include "pch.h"
#include "Building.h"
#include "Inventory.h"
#include "XP.h"

bool Building::CanBePlacedByPlayer(UClass* BuildClass) {
	return ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->AllPlayerBuildableClasses.Search([BuildClass](UClass* Class) { return Class == BuildClass; });
}

void Building::ServerCreateBuildingActor(UObject* Context, FFrame& Stack)
{
	FCreateBuildingActorData CreateBuildingData;
	Stack.StepCompiledIn(&CreateBuildingData);
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerController*)Context;
	if (!PlayerController)
		return;
		//return callOG(PlayerController, Stack.CurrentNativeFunction, ServerCreateBuildingActor, CreateBuildingData);


	auto BuildingClassPtr = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->AllPlayerBuildableClassesIndexLookup.SearchForKey([&](UClass* Class, int32 Handle) {
		return Handle == CreateBuildingData.BuildingClassHandle;
		});
	if (!BuildingClassPtr)
		return;
		//return callOG(PlayerController, Stack.CurrentNativeFunction, ServerCreateBuildingActor, CreateBuildingData);

	auto BuildingClass = *BuildingClassPtr;

	auto Resource = UFortKismetLibrary::K2_GetResourceItemDefinition(((ABuildingSMActor*)BuildingClass->DefaultObject)->ResourceType);
	
	FFortItemEntry* ItemEntry = nullptr;
	if (!PlayerController->bBuildFree)
	{
		ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
			{ return entry.ItemDefinition == Resource; });
		if (!ItemEntry || ItemEntry->Count < 10)
		{
			PlayerController->ClientSendMessage(UKismetTextLibrary::Conv_StringToText(L"Not enough resources to build! Change building material or gather more!"), nullptr);
			PlayerController->ClientTriggerUIFeedbackEvent(FName(L"BuildPreviewUnableToAfford"));
			return;
			//return callOG(PlayerController, Stack.CurrentNativeFunction, ServerCreateBuildingActor, CreateBuildingData);
		}
	}

	TArray<ABuildingSMActor*> RemoveBuildings;
	char _Unk_OutVar1;
	static auto CantBuild = (__int64 (*)(UWorld*, UObject*, FVector, FRotator, bool, TArray<ABuildingSMActor*> *, char*))(ImageBase + 0x63fcf40);
	if (CantBuild(UWorld::GetWorld(), BuildingClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, CreateBuildingData.bMirrored, &RemoveBuildings, &_Unk_OutVar1))
	{
		PlayerController->ClientSendMessage(UKismetTextLibrary::Conv_StringToText(L"Building in this location is restricted!"), nullptr);
		PlayerController->ClientTriggerUIFeedbackEvent(FName(L"BuildPreviewUnableToPlace"));
		return;
		//return callOG(PlayerController, Stack.CurrentNativeFunction, ServerCreateBuildingActor, CreateBuildingData);
	}


	for (auto& RemoveBuilding : RemoveBuildings)
		RemoveBuilding->K2_DestroyActor();
	RemoveBuildings.Free();

	ABuildingSMActor* Building = Utils::SpawnActor<ABuildingSMActor>(BuildingClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, PlayerController);
	if (!Building)
	{
		return;
		//return callOG(PlayerController, Stack.CurrentNativeFunction, ServerCreateBuildingActor, CreateBuildingData);
	}

	Building->CurrentBuildingLevel = CreateBuildingData.BuildingClassData.UpgradeLevel;
	Building->OnRep_CurrentBuildingLevel();

	Building->SetMirrored(CreateBuildingData.bMirrored);

	Building->bPlayerPlaced = true;

	Building->InitializeKismetSpawnedBuildingActor(Building, PlayerController, true, nullptr);


	if (!PlayerController->bBuildFree)
	{
		ItemEntry->Count -= 10;
		if (ItemEntry->Count <= 0)
			Inventory::Remove(PlayerController, ItemEntry->ItemGuid);
		else
			Inventory::ReplaceEntry((AFortPlayerControllerAthena*)PlayerController, *ItemEntry);
	}

	Building->TeamIndex = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
	Building->Team = EFortTeam(Building->TeamIndex);

	//return callOG(PlayerController, Stack.CurrentNativeFunction, ServerCreateBuildingActor, CreateBuildingData);
}

void SetEditingPlayer(ABuildingSMActor* _this, AFortPlayerStateZone* NewEditingPlayer)
{
	auto& _LastEditInstigatorPC = *(TWeakObjectPtr<AController>*)(__int64(_this) + offsetof(ABuildingSMActor, EditingPlayer) + sizeof(void*));
	if (_this->Role == ENetRole::ROLE_Authority && (!_this->EditingPlayer || !NewEditingPlayer))
	{
		_this->SetNetDormancy(ENetDormancy(2 - (NewEditingPlayer != 0)));
		_this->ForceNetUpdate();
		auto EditingPlayer = _this->EditingPlayer;
		if (EditingPlayer)
		{
			auto Handle = EditingPlayer->Owner;
			if (Handle)
			{
				if (auto PlayerController = Handle->Cast<AFortPlayerController>())
				{
					_LastEditInstigatorPC.ObjectIndex = PlayerController->Index;
					_LastEditInstigatorPC.ObjectSerialNumber = UObject::GObjects->GetItemByIndex(PlayerController->Index)->SerialNumber;
					_this->EditingPlayer = NewEditingPlayer;
					return;
				}
			}
		}
		else
		{
			if (!NewEditingPlayer)
			{
				_this->EditingPlayer = NewEditingPlayer;
				return;
			}

			auto Handle = NewEditingPlayer->Owner;
			if (auto PlayerController = Handle->Cast<AFortPlayerController>())
			{
				_LastEditInstigatorPC.ObjectIndex = PlayerController->Index;
				_LastEditInstigatorPC.ObjectSerialNumber = UObject::GObjects->GetItemByIndex(PlayerController->Index)->SerialNumber;
				_this->EditingPlayer = NewEditingPlayer;
			}
		}
	}
}

void Building::ServerBeginEditingBuildingActor(UObject* Context, FFrame& Stack)
{
	ABuildingSMActor* Building;
	Stack.StepCompiledIn(&Building);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerController*)Context;
	if (!PlayerController || !PlayerController->MyFortPawn || !Building || Building->TeamIndex != static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState)->TeamIndex)
		return;

	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
	if (!PlayerState)
		return;

	SetEditingPlayer(Building, PlayerState);

	//auto EditTool = PlayerController->MyFortPawn->CurrentWeapon->Cast<AFortWeap_EditingTool>();
	auto EditToolEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
		return entry.ItemDefinition->IsA<UFortEditToolItemDefinition>();
	});
	
	PlayerController->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)EditToolEntry->ItemDefinition, EditToolEntry->ItemGuid, EditToolEntry->TrackerGuid, false);
	
	if (auto EditTool = PlayerController->MyFortPawn->CurrentWeapon->Cast<AFortWeap_EditingTool>())
	{
		EditTool->EditActor = Building;
		EditTool->OnRep_EditActor();
	}

	//PlayerController->ServerExecuteInventoryItem(ItemEntry->ItemGuid);
}

void Building::ServerEditBuildingActor(UObject* Context, FFrame& Stack)
{
	ABuildingSMActor* Building;
	TSubclassOf<ABuildingSMActor> NewClass;
	uint8 RotationIterations;
	bool bMirrored;
	Stack.StepCompiledIn(&Building);
	Stack.StepCompiledIn(&NewClass);
	Stack.StepCompiledIn(&RotationIterations);
	Stack.StepCompiledIn(&bMirrored);
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerController*)Context;
	if (!PlayerController || !Building || !NewClass || !Building->IsA<ABuildingSMActor>() || !CanBePlacedByPlayer(NewClass) || Building->TeamIndex != static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState)->TeamIndex || Building->bDestroyed)
		return;

	SetEditingPlayer(Building, nullptr);

	static auto ReplaceBuildingActor = (ABuildingSMActor * (*)(ABuildingSMActor*, unsigned int, UObject*, unsigned int, int, bool, AFortPlayerController*))(ImageBase + 0x61b1ab4);

	ABuildingSMActor* NewBuild = ReplaceBuildingActor(Building, 1, NewClass, Building->CurrentBuildingLevel, RotationIterations, bMirrored, PlayerController);

	if (NewBuild)
		NewBuild->bPlayerPlaced = true;
}

void Building::ServerEndEditingBuildingActor(UObject* Context, FFrame& Stack)
{
	ABuildingSMActor* Building;
	Stack.StepCompiledIn(&Building);
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerController*)Context;
	if (!PlayerController || !PlayerController->MyFortPawn || !Building || Building->EditingPlayer != (AFortPlayerStateZone*)PlayerController->PlayerState || Building->TeamIndex != static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState)->TeamIndex || Building->bDestroyed)
		return;

	SetEditingPlayer(Building, nullptr);

	auto EditToolEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
		return entry.ItemDefinition->IsA<UFortEditToolItemDefinition>();
		});

	PlayerController->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)EditToolEntry->ItemDefinition, EditToolEntry->ItemGuid, EditToolEntry->TrackerGuid, false);

	if (auto EditTool = PlayerController->MyFortPawn->CurrentWeapon->Cast<AFortWeap_EditingTool>())
	{
		EditTool->EditActor = nullptr;
		EditTool->OnRep_EditActor();
	}
}

void Building::ServerRepairBuildingActor(UObject* Context, FFrame& Stack)
{
	ABuildingSMActor* Building;
	Stack.StepCompiledIn(&Building);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerController*)Context;
	if (!PlayerController)
		return;

	auto Price = (int32)std::floor((10.f * (1.f - Building->GetHealthPercent())) * 0.75f);
	auto res = UFortKismetLibrary::K2_GetResourceItemDefinition(Building->ResourceType);
	auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([res](FFortItemEntry& entry) {
		return entry.ItemDefinition == res;
		});

	itemEntry->Count -= Price;
	if (itemEntry->Count <= 0)
		Inventory::Remove(PlayerController, itemEntry->ItemGuid);
	else
		Inventory::ReplaceEntry(PlayerController, *itemEntry);

	Building->RepairBuilding(PlayerController, Price);

	if (auto ControllerAthena = PlayerController->Cast<AFortPlayerControllerAthena>()) ControllerAthena->BuildingsRepaired++;
}

void Building::OnDamageServer(ABuildingSMActor* Actor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, FHitResult HitInfo, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, FGameplayEffectContextHandle EffectContext) {
	auto GameState = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState);
	if (!InstigatedBy || Actor->bPlayerPlaced || Actor->GetHealth() == 1 || Actor->IsA(UObject::FindClassFast("B_Athena_VendingMachine_C")) || Actor->IsA(GameState->MapInfo->LlamaClass)) return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	if (!DamageCauser || !DamageCauser->IsA<AFortWeapon>() || !((AFortWeapon*)DamageCauser)->WeaponData->IsA<UFortWeaponMeleeItemDefinition>()) return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	static auto PickaxeTag = FName(L"Weapon.Melee.Impact.Pickaxe");
	auto entry = DamageTags.GameplayTags.Search([](FGameplayTag& entry) {
		return entry.TagName.ComparisonIndex == PickaxeTag.ComparisonIndex;
		});
	if (!entry)
		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	auto Resource = UFortKismetLibrary::K2_GetResourceItemDefinition(Actor->ResourceType);
	if (!Resource)
		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	auto MaxMat = (int32)Utils::EvaluateScalableFloat(Resource->MaxStackSize);

	FCurveTableRowHandle& BuildingResourceAmountOverride = Actor->BuildingResourceAmountOverride;
	int ResCount = 0;

	if (Actor->BuildingResourceAmountOverride.RowName.ComparisonIndex > 0)
	{
		float Out;
		UDataTableFunctionLibrary::EvaluateCurveTableRow(Actor->BuildingResourceAmountOverride.CurveTable, Actor->BuildingResourceAmountOverride.RowName, 0.f, nullptr, &Out, FString());

		float RC = Out / (Actor->GetMaxHealth() / Damage);

		ResCount = (int)round(RC);
	}

	if (ResCount > 0)
	{
		auto itemEntry = InstigatedBy->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
			return entry.ItemDefinition == Resource;
			});

		if (itemEntry)
		{
			itemEntry->Count += ResCount;
			if (itemEntry->Count > MaxMat)
			{
				Inventory::SpawnPickup(InstigatedBy->Pawn->K2_GetActorLocation(), itemEntry->ItemDefinition, itemEntry->Count - MaxMat, 0, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset, InstigatedBy->MyFortPawn);
				itemEntry->Count = MaxMat;
			}

			Inventory::ReplaceEntry(InstigatedBy, *itemEntry);
		}
		else
		{
			if (ResCount > MaxMat)
			{
				Inventory::SpawnPickup(InstigatedBy->Pawn->K2_GetActorLocation(), Resource, ResCount - MaxMat, 0, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset, InstigatedBy->MyFortPawn);
				ResCount = MaxMat;
			}

			Inventory::GiveItem(InstigatedBy, Resource, ResCount, 0, 0, false);
		}
	}

	if (GameState->GamePhase != EAthenaGamePhase::Warmup) 
	{
		if (Damage == 100.f)
		{
			XP::GiveAccolade(InstigatedBy, Utils::FindObject<UFortAccoladeItemDefinition>(L"/BattlePassPermanentQuests/Items/Accolades/AccoladeId_066_WeakSpotsInARow.AccoladeId_066_WeakSpotsInARow"));
		}
	}

	InstigatedBy->ClientReportDamagedResourceBuilding(Actor, ResCount == 0 ? EFortResourceType::None : Actor->ResourceType, ResCount, false, Damage == 100.f);
	return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
}


void Building::ServerSpawnDeco(UObject* Context, FFrame& Stack) {
	FVector Location;
	FRotator Rotation;
	ABuildingSMActor* AttachedActor;
	EBuildingAttachmentType InBuildingAttachmentType;
	Stack.StepCompiledIn(&Location);
	Stack.StepCompiledIn(&Rotation);
	Stack.StepCompiledIn(&AttachedActor);
	Stack.StepCompiledIn(&InBuildingAttachmentType);
	Stack.IncrementCode();

	auto DecoTool = (AFortDecoTool*)Context;
	auto ContextTrap = DecoTool->Cast<AFortDecoTool_ContextTrap>();
	auto ItemDefinition = (UFortDecoItemDefinition*)DecoTool->ItemDefinition;

	if (auto ContextTrapTool = DecoTool->Cast<AFortDecoTool_ContextTrap>()) {
		switch ((int)InBuildingAttachmentType) {
		case 0:
		case 6:
			ItemDefinition = ContextTrapTool->ContextTrapItemDefinition->FloorTrap;
			break;
		case 7:
		case 2:
			ItemDefinition = ContextTrapTool->ContextTrapItemDefinition->CeilingTrap;
			break;
		case 1:
			ItemDefinition = ContextTrapTool->ContextTrapItemDefinition->WallTrap;
			break;
		case 8:
			ItemDefinition = ContextTrapTool->ContextTrapItemDefinition->StairTrap;
			break;
		}
	}

	auto NewTrap = Utils::SpawnActor<ABuildingActor>(ItemDefinition->BlueprintClass.Get(), Location, Rotation, AttachedActor);
	AttachedActor->AttachBuildingActorToMe(NewTrap, true);
	AttachedActor->bHiddenDueToTrapPlacement = ItemDefinition->bReplacesBuildingWhenPlaced;
	if (ItemDefinition->bReplacesBuildingWhenPlaced) AttachedActor->bActorEnableCollision = false;
	AttachedActor->ForceNetUpdate();

	auto Pawn = (APawn*)DecoTool->Owner;
	if (!Pawn)
		return;
	auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
	if (!PlayerController)
		return;

	auto Resource = UFortKismetLibrary::GetDefaultObj()->K2_GetResourceItemDefinition(AttachedActor->ResourceType);
	auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
		return entry.ItemDefinition == DecoTool->ItemDefinition;
		});
	if (!itemEntry)
		return;

	itemEntry->Count--;
	if (itemEntry->Count <= 0)
		Inventory::Remove(PlayerController, itemEntry->ItemGuid);
	else
		Inventory::ReplaceEntry(PlayerController, *itemEntry);

	if (NewTrap->TeamIndex != ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex)
	{
		NewTrap->TeamIndex = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
		NewTrap->Team = EFortTeam(NewTrap->TeamIndex);
	}
}

EFortBuildingType GetBuildingTypeFromBuildingAttachmentType(EBuildingAttachmentType BuildingAttachmentType)
{
	if (uint8(BuildingAttachmentType) <= 7)
	{
		LONG Val = 0xC5;
		if (BitTest(&Val, uint8(BuildingAttachmentType)))
			return EFortBuildingType::Floor;
	}
	if (BuildingAttachmentType == EBuildingAttachmentType::ATTACH_Wall)
		return EFortBuildingType::Wall;
	return EFortBuildingType::None;
}

AFortPlayerController* DecoGetPlayerController(AFortDecoTool* Tool)
{
	auto Instigator = Tool->Instigator;
	if (!Instigator)
		return nullptr;

	auto Pawn = Instigator->Cast<AFortPlayerPawn>();
	if (!Pawn)
		return 0;

	if (auto PlayerController = Pawn->Controller->Cast<AFortPlayerController>())
		return PlayerController;

	auto VehicleActor = Pawn->GetVehicleActor();
	if (!VehicleActor)
		return nullptr;

	auto VehicleInterface = Utils::GetInterface<IFortVehicleInterface>(VehicleActor);

	return VehicleInterface->GetVehicleController(Pawn);
}

void Building::ServerCreateBuildingAndSpawnDeco(UObject* Context, FFrame& Stack) {
	auto Tool = (AFortDecoTool*)Context;
	auto Pawn = (APawn*)Tool->Owner;
	if (!Pawn) return;
	auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
	if (!PlayerController) return;
	FVector_NetQuantize10 BuildingLocation;
	FRotator BuildingRotation;
	FVector_NetQuantize10 Location;
	FRotator Rotation;
	EBuildingAttachmentType InBuildingAttachmentType;
	bool bSpawnDecoOnExtraPiece;
	FVector BuildingExtraPieceLocation;
	Stack.StepCompiledIn(&BuildingLocation);
	Stack.StepCompiledIn(&BuildingRotation);
	Stack.StepCompiledIn(&Location);
	Stack.StepCompiledIn(&Rotation);
	Stack.StepCompiledIn(&InBuildingAttachmentType);
	Stack.StepCompiledIn(&bSpawnDecoOnExtraPiece);
	Stack.StepCompiledIn(&BuildingExtraPieceLocation);
	Stack.IncrementCode();

	auto ItemDefinition = (UFortDecoItemDefinition*)Tool->ItemDefinition;


	if (auto ContextTrapTool = Tool->Cast<AFortDecoTool_ContextTrap>()) {
		switch ((int)InBuildingAttachmentType) {
		case 0:
		case 6:
			ItemDefinition = ContextTrapTool->ContextTrapItemDefinition->FloorTrap;
			break;
		case 7:
		case 2:
			ItemDefinition = ContextTrapTool->ContextTrapItemDefinition->CeilingTrap;
			break;
		case 1:
			ItemDefinition = ContextTrapTool->ContextTrapItemDefinition->WallTrap;
			break;
		case 8:
			ItemDefinition = ContextTrapTool->ContextTrapItemDefinition->StairTrap;
			break;
		}
	}

	TArray<UBuildingEditModeMetadata*> AutoCreateAttachmentBuildingShapes;
	for (auto& AutoCreateAttachmentBuildingShape : ItemDefinition->AutoCreateAttachmentBuildingShapes)
	{
		AutoCreateAttachmentBuildingShapes.Add(AutoCreateAttachmentBuildingShape);
	}

	auto bIgnoreCanAffordCheck = UFortKismetLibrary::DoesItemDefinitionHaveGameplayTag(ItemDefinition, FGameplayTag(FName(L"Trap.ExtraPiece.Cost.Ignore")));
	TSubclassOf<ABuildingSMActor> Ret;
	static auto FindAvailableBuildingClassForBuildingTypeAndEditModePattern = (TSubclassOf<ABuildingSMActor>*(*)(AFortPlayerController*, TSubclassOf<ABuildingSMActor>*, EFortBuildingType, TArray<UBuildingEditModeMetadata*>*, bool, EFortResourceType)) (ImageBase + 0x67611E4);
	FindAvailableBuildingClassForBuildingTypeAndEditModePattern(PlayerController, &Ret, GetBuildingTypeFromBuildingAttachmentType(InBuildingAttachmentType), &AutoCreateAttachmentBuildingShapes, bIgnoreCanAffordCheck, ItemDefinition->AutoCreateAttachmentBuildingResourceType);

	auto Build = Ret.Get();

	ABuildingSMActor* Building = nullptr;
	TArray<ABuildingSMActor*> RemoveBuildings;
	char _Unk_OutVar1;
	static auto CantBuild = (__int64 (*)(UWorld*, UObject*, FVector, FRotator, bool, TArray<ABuildingSMActor*> *, char*))(ImageBase + 0x63fcf40);
	if (CantBuild(UWorld::GetWorld(), Build, BuildingLocation, BuildingRotation, false, &RemoveBuildings, &_Unk_OutVar1)) return;

	auto Resource = UFortKismetLibrary::GetDefaultObj()->K2_GetResourceItemDefinition(((ABuildingSMActor*)Build->DefaultObject)->ResourceType);
	auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
		return entry.ItemDefinition == Resource;
		});
	if (!itemEntry) return;

	Building = Utils::SpawnActor<ABuildingSMActor>(Build, BuildingLocation, BuildingRotation, PlayerController);

	Building->bPlayerPlaced = true;

	Building->InitializeKismetSpawnedBuildingActor(Building, PlayerController, true, nullptr);


	if (!PlayerController->bBuildFree)
	{
		itemEntry->Count -= 10;
		if (itemEntry->Count <= 0)
			Inventory::Remove(PlayerController, itemEntry->ItemGuid);
		else
			Inventory::ReplaceEntry((AFortPlayerControllerAthena*)PlayerController, *itemEntry);
	}

	Building->TeamIndex = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
	Building->Team = EFortTeam(Building->TeamIndex);
	Tool->ServerSpawnDeco(Location, Rotation, Building, InBuildingAttachmentType);
}

struct FWeaponAntiCheat
{
	FGuid ItemEntryGuid;
	AFortPawn* LastActor;
	float LastFireTime;
	int32 LastAmmoCount;
	int32 LastBulletsPerCartridge;
	int32 LastCartridgePerFire;
};
TArray<FWeaponAntiCheat> WeaponAntiCheats;

void Building::PawnOnDamageServer(AFortPawn* Actor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, FHitResult HitInfo, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, FGameplayEffectContextHandle EffectContext) {
	auto GameState = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState);
	AFortWeapon* Weapon = DamageCauser->Cast<AFortWeapon>();
	UWorld* World = UWorld::GetWorld();

	if (Weapon && World)
	{
		//UAntiCheatOdyssey* AntiCheatOdyssey = FindOrCreateAntiCheatOdyssey(Cast<APlayerController>(InstigatedBy));

		//if (AntiCheatOdyssey)
		{
			const float TimeSeconds = UGameplayStatics::GetTimeSeconds(World);

			//FAntiCheatWeapon* AntiCheatWeapon = AntiCheatOdyssey->GetOrCreateAntiCheatWeapon(Weapon->ItemEntryGuid);
			//if (!AntiCheatWeapon) return;
			auto AntiCheatWeapon = WeaponAntiCheats.Search([&](FWeaponAntiCheat& WeaponAC)
			{
				return WeaponAC.ItemEntryGuid == Weapon->ItemEntryGuid;
			});
			if (!AntiCheatWeapon)
			{
				FWeaponAntiCheat NewWeaponAC;

				NewWeaponAC.ItemEntryGuid = Weapon->ItemEntryGuid;
				AntiCheatWeapon = &WeaponAntiCheats.Add(NewWeaponAC);
			}

			float LastFireTime = AntiCheatWeapon->LastFireTime;
			int32 AmmoCount = Weapon->AmmoCount;

			if (LastFireTime != -1.0f)
			{
				FFortRangedWeaponStats* RangedWeaponStats = (FFortRangedWeaponStats*)Inventory::GetStats(Weapon->WeaponData);
				if (!RangedWeaponStats) return;

				float ExpectedFireInterval = 1.0f / RangedWeaponStats->FiringRate;
				float FireInterval = TimeSeconds - LastFireTime;
				float Tolerance = 0.02f;

				int32 BulletsPerCartridge = RangedWeaponStats->BulletsPerCartridge;
				int32 CartridgePerFire = RangedWeaponStats->CartridgePerFire;

				if (AntiCheatWeapon->LastActor != Actor)
				{
					//AntiCheatOdyssey->SetLastBulletsPerCartridgeAntiCheatWeapon(Weapon->ItemEntryGuid, AntiCheatOdyssey->GetNullCartridgeAmmo());
					//AntiCheatOdyssey->SetLastCartridgePerFireAntiCheatWeapon(Weapon->ItemEntryGuid, AntiCheatOdyssey->GetNullCartridgeAmmo());
					AntiCheatWeapon->LastBulletsPerCartridge = -9999;
					AntiCheatWeapon->LastCartridgePerFire = -9999;

					AntiCheatWeapon->LastFireTime = TimeSeconds;
					AntiCheatWeapon->LastAmmoCount = AmmoCount;

					//AntiCheatOdyssey->SetLastActorAntiCheatWeapon(Weapon->ItemEntryGuid, Pawn);
					AntiCheatWeapon->LastActor = Actor;
					return;
				}

				bool bIsValid = (FireInterval >= (ExpectedFireInterval - Tolerance));

				if (CartridgePerFire > 1 && !bIsValid)
				{
					//FN_LOG(LogGameMode, Log, L"CartridgePerFire bullet detect!");

					if ((AntiCheatWeapon->LastCartridgePerFire == -9999) || (AntiCheatWeapon->LastCartridgePerFire >= 0))
					{
						Log(L"Valid CartridgePerFire bullet!");
					}
					else
						Log(L"Haram CartridgePerFire bullet!");

					if (AntiCheatWeapon->LastCartridgePerFire == -9999)
						AntiCheatWeapon->LastCartridgePerFire = CartridgePerFire - 1;
					//AntiCheatOdyssey->SetLastCartridgePerFireAntiCheatWeapon(Weapon->ItemEntryGuid, (CartridgePerFire - 1));

					AntiCheatWeapon->LastCartridgePerFire = AntiCheatWeapon->LastCartridgePerFire - 1;
					//AntiCheatOdyssey->SetLastCartridgePerFireAntiCheatWeapon(Weapon->ItemEntryGuid, (AntiCheatWeapon->LastCartridgePerFire - 1));
				}
				else if (CartridgePerFire > 1)
					AntiCheatWeapon->LastCartridgePerFire = -9999;
					//AntiCheatOdyssey->SetLastCartridgePerFireAntiCheatWeapon(Weapon->ItemEntryGuid, AntiCheatOdyssey->GetNullCartridgeAmmo());


				if (BulletsPerCartridge > 1 && !bIsValid)
				{
					//FN_LOG(LogGameMode, Log, L"BulletsPerCartridge bullet detect!");

					if ((AntiCheatWeapon->LastBulletsPerCartridge == -9999) || (AntiCheatWeapon->LastBulletsPerCartridge >= 0))
					{
						Log(L"Valid BulletsPerCartridge bullet!");
					}
					else
						Log(L"Haram BulletsPerCartridge bullet!");

					if (AntiCheatWeapon->LastBulletsPerCartridge == -9999)
						AntiCheatWeapon->LastBulletsPerCartridge = BulletsPerCartridge - 1;
					//AntiCheatOdyssey->SetLastBulletsPerCartridgeAntiCheatWeapon(Weapon->ItemEntryGuid, (BulletsPerCartridge - 1));

					AntiCheatWeapon->LastBulletsPerCartridge = AntiCheatWeapon->LastBulletsPerCartridge - 1;
					//AntiCheatOdyssey->SetLastBulletsPerCartridgeAntiCheatWeapon(Weapon->ItemEntryGuid, (AntiCheatWeapon->LastBulletsPerCartridge - 1));
				}
				else if (BulletsPerCartridge > 1)
					AntiCheatWeapon->LastBulletsPerCartridge = -9999;
					//AntiCheatOdyssey->SetLastBulletsPerCartridgeAntiCheatWeapon(Weapon->ItemEntryGuid, AntiCheatOdyssey->GetNullCartridgeAmmo());

				if (CartridgePerFire == 1 && BulletsPerCartridge == 1 && !bIsValid)
				{
					Log(L"Haram bullet!");
				}

				// Faire pareil pour les BulletsPerCartridge

				bool bIsBulletsPerCartridge = ((AmmoCount == AntiCheatWeapon->LastAmmoCount) && RangedWeaponStats->BulletsPerCartridge > 1);
				bool bIsValidCartridgePerFireBullet = (FireInterval >= ((ExpectedFireInterval - Tolerance)) / RangedWeaponStats->CartridgePerFire);
				// bool bIsValid = (FireInterval >= (ExpectedFireInterval - Tolerance) /*&& FireInterval <= (ExpectedFireInterval + Tolerance)*/);

				/*FN_LOG(LogGameMode, Log, L"BulletsPerCartridge: %i", RangedWeaponStats->BulletsPerCartridge);
				FN_LOG(LogGameMode, Log, L"CartridgePerFire: %i", RangedWeaponStats->CartridgePerFire);

				FN_LOG(LogGameMode, Log, L"AmmoCount: %i", AmmoCount);
				FN_LOG(LogGameMode, Log, L"LastAmmoCount: %i", AntiCheatWeapon->LastAmmoCount);
				FN_LOG(LogGameMode, Log, L"LastCartridgePerFire: %i", AntiCheatWeapon->LastCartridgePerFire);

				FN_LOG(LogGameMode, Log, L"ExpectedFireInterval: %.2f", ExpectedFireInterval);
				FN_LOG(LogGameMode, Log, L"FireInterval: %.2f", FireInterval);

				FN_LOG(LogGameMode, Log, L"bIsBulletsPerCartridge: %i", bIsBulletsPerCartridge);
				FN_LOG(LogGameMode, Log, L"bIsValidCartridgePerFireBullet: %i", bIsValidCartridgePerFireBullet);*/
				//FN_LOG(LogGameMode, Log, L"bIsValid: %i", bIsValid);

				/*float MaxCriticalDamage = ((RangedWeaponStats->DmgPB * RangedWeaponStats->BulletsPerCartridge) * RangedWeaponStats->DamageZone_Critical);
				float MaxDamage = (RangedWeaponStats->DmgPB * RangedWeaponStats->BulletsPerCartridge);

				FN_LOG(LogGameMode, Log, L"MaxCriticalDamage: %.2f", MaxCriticalDamage);
				FN_LOG(LogGameMode, Log, L"MaxDamage: %.2f", MaxDamage);*/

				AntiCheatWeapon->LastFireTime = TimeSeconds;
				AntiCheatWeapon->LastAmmoCount = AmmoCount;
			}
			else
			{
				AntiCheatWeapon->LastFireTime = TimeSeconds;
				AntiCheatWeapon->LastAmmoCount = AmmoCount;
			}
		}
	}

	return PawnOnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
}

void Building::Hook() {
	Utils::ExecHook(L"/Script/FortniteGame.FortPlayerController.ServerCreateBuildingActor", ServerCreateBuildingActor, ServerCreateBuildingActorOG);
	Utils::ExecHook(L"/Script/FortniteGame.FortPlayerController.ServerBeginEditingBuildingActor", ServerBeginEditingBuildingActor);
	Utils::ExecHook(L"/Script/FortniteGame.FortPlayerController.ServerEditBuildingActor", ServerEditBuildingActor);
	Utils::ExecHook(L"/Script/FortniteGame.FortPlayerController.ServerEndEditingBuildingActor", ServerEndEditingBuildingActor);
	Utils::ExecHook(L"/Script/FortniteGame.FortPlayerController.ServerRepairBuildingActor", ServerRepairBuildingActor);
	Utils::Hook(ImageBase + 0x69e3008, OnDamageServer, OnDamageServerOG);
	//Utils::Hook(ImageBase + 0x6BDE3F4, PawnOnDamageServer, PawnOnDamageServerOG);
	Utils::ExecHook(L"/Script/FortniteGame.FortDecoTool.ServerSpawnDeco", ServerSpawnDeco, ServerSpawnDecoOG);
	Utils::ExecHook(L"/Script/FortniteGame.FortDecoTool.ServerCreateBuildingAndSpawnDeco", ServerCreateBuildingAndSpawnDeco);
	Utils::ExecHook(L"/Script/FortniteGame.FortDecoTool_ContextTrap.ServerSpawnDeco_Implementation", ServerSpawnDeco, ServerSpawnDecoOG);
	Utils::ExecHook(L"/Script/FortniteGame.FortDecoTool_ContextTrap.ServerCreateBuildingAndSpawnDeco_Implementation", ServerCreateBuildingAndSpawnDeco);
}
