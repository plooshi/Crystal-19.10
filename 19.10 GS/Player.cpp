#include "pch.h"
#include "Player.h"
#include "Abilities.h"
#include "AC.h"
#include "Inventory.h"
#include "Options.h"
#include "Lategame.h"
#include "Results.h"
#include "Tournaments.h"
#include "XP.h"
#include "SDK/GAB_AthenaDBNO_classes.hpp"
#include "Misc.h"

void Player::ServerReadyToStartMatch(UObject* Context, FFrame& Stack)
{
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerController*)Context;

	callOG(PlayerController, Stack.CurrentNativeFunction, ServerReadyToStartMatch);
}

void Player::ServerAcknowledgePossession(UObject* Context, FFrame& Stack)
{
	APawn* Pawn;
	Stack.StepCompiledIn(&Pawn);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerController*)Context;

	AC::CheckUser((AFortPlayerControllerAthena*)PlayerController);
	PlayerController->AcknowledgedPawn = Pawn;
}

void Player::GetPlayerViewPoint(APlayerController* PlayerController, FVector& Loc, FRotator& Rot)
{
	static auto SFName = FName(L"Spectating");
	if (PlayerController->StateName == SFName)
	{
		Loc = PlayerController->LastSpectatorSyncLocation;
		Rot = PlayerController->LastSpectatorSyncRotation;
	}
	else if (PlayerController->GetViewTarget())
	{
		Loc = PlayerController->GetViewTarget()->K2_GetActorLocation();
		Rot = PlayerController->GetControlRotation();
	}
	else return GetPlayerViewPointOG(PlayerController, Loc, Rot);
}

void Player::ServerExecuteInventoryItem(UObject* Context, FFrame& Stack)
{
	FGuid ItemGuid;
	Stack.StepCompiledIn(&ItemGuid);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerController*)Context;
	if (!PlayerController) [[unlikely]] return;
	auto entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
		return entry.ItemGuid == ItemGuid;
		});

	if (!entry || !PlayerController->MyFortPawn) [[unlikely]] return;
	UFortWeaponItemDefinition* ItemDefinition = (UFortWeaponItemDefinition*)entry->ItemDefinition;
	PlayerController->MyFortPawn->EquipWeaponDefinition(ItemDefinition, ItemGuid, entry->TrackerGuid, false);
}

void Player::ServerReturnToMainMenu(UObject* Context, FFrame& Stack)
{
	Stack.IncrementCode();
	return ((AFortPlayerController*)Context)->ClientReturnToMainMenu(L"");
}

void Player::ServerAttemptAircraftJump(UObject* Context, FFrame& Stack)
{
	FRotator Rotation;
	Stack.StepCompiledIn(&Rotation);
	Stack.IncrementCode();

	auto Component = (UFortControllerComponent_Aircraft*)Context;
	auto PlayerController = (AFortPlayerController*)Component->GetOwner();
	auto PlayerState = (AFortPlayerState*)PlayerController->PlayerState;
	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

	GameMode->RestartPlayer(PlayerController);
	if (bLateGame && PlayerController->MyFortPawn) {
		FVector AircraftLocation = GameState->Aircrafts[0]->K2_GetActorLocation();

		float Angle = UKismetMathLibrary::RandomFloatInRange(0.0f, 6.2831853f);
		float Radius = (float)(rand() % 1000);

		float OffsetX = cosf(Angle) * Radius;
		float OffsetY = sinf(Angle) * Radius;

		FVector Offset;
		Offset.X = OffsetX;
		Offset.Y = OffsetY;
		Offset.Z = 0.0f;

		FVector NewLoc = AircraftLocation + Offset;

		PlayerController->MyFortPawn->K2_SetActorLocation(NewLoc, false, nullptr, false);
	}
	PlayerController->ClientSetRotation(Rotation, true);
	if (PlayerController->MyFortPawn)
	{
		PlayerController->MyFortPawn->BeginSkydiving(true);
		PlayerController->MyFortPawn->SetHealth(100);

		if (bLateGame)
		{
			PlayerController->MyFortPawn->SetShield(100);

			auto Shotgun = Lategame::GetShotguns();
			auto AssaultRifle = Lategame::GetAssaultRifles();
			auto Sniper = Lategame::GetSnipers();
			auto Heal = Lategame::GetHeals();
			auto HealSlot2 = Lategame::GetHeals();

			int ShotgunClipSize = Inventory::GetStats((UFortWeaponItemDefinition*)Shotgun.Item)->ClipSize;
			int AssaultRifleClipSize = Inventory::GetStats((UFortWeaponItemDefinition*)AssaultRifle.Item)->ClipSize;
			int SniperClipSize = Inventory::GetStats((UFortWeaponItemDefinition*)Sniper.Item)->ClipSize;
			// for grappler
			int HealClipSize = Heal.Item->IsA<UFortWeaponItemDefinition>() ? Inventory::GetStats((UFortWeaponItemDefinition*)Heal.Item)->ClipSize : 0;
			int HealSlot2ClipSize = HealSlot2.Item->IsA<UFortWeaponItemDefinition>() ? Inventory::GetStats((UFortWeaponItemDefinition*)HealSlot2.Item)->ClipSize : 0;

			Inventory::GiveItem(PlayerController, Lategame::GetResource(EFortResourceType::Wood), 500);
			Inventory::GiveItem(PlayerController, Lategame::GetResource(EFortResourceType::Stone), 500);
			Inventory::GiveItem(PlayerController, Lategame::GetResource(EFortResourceType::Metal), 500);

			Inventory::GiveItem(PlayerController, Lategame::GetAmmo(EAmmoType::Assault), 250);
			Inventory::GiveItem(PlayerController, Lategame::GetAmmo(EAmmoType::Shotgun), 50);
			Inventory::GiveItem(PlayerController, Lategame::GetAmmo(EAmmoType::Submachine), 400);
			Inventory::GiveItem(PlayerController, Lategame::GetAmmo(EAmmoType::Rocket), 6);
			Inventory::GiveItem(PlayerController, Lategame::GetAmmo(EAmmoType::Sniper), 20);

			Inventory::GiveItem(PlayerController, AssaultRifle.Item, AssaultRifle.Count, AssaultRifleClipSize, true);
			Inventory::GiveItem(PlayerController, Shotgun.Item, Shotgun.Count, ShotgunClipSize, true);
			Inventory::GiveItem(PlayerController, Sniper.Item, Sniper.Count, SniperClipSize, true);
			Inventory::GiveItem(PlayerController, Heal.Item, Heal.Count, HealClipSize, true);
			Inventory::GiveItem(PlayerController, HealSlot2.Item, HealSlot2.Count, HealSlot2ClipSize, true);
		}
	}

	//Inventory::GiveItem(PlayerController, Utils::FindObject<UFortItemDefinition>(L"/CampsiteGameplay/Items/Campsite/WID_Athena_DeployableCampsite_Thrown.WID_Athena_DeployableCampsite_Thrown"), 5);
}

void Player::ServerPlayEmoteItem(UObject* Context, FFrame& Stack)
{
	UFortMontageItemDefinitionBase* Asset;
	float RandomNumber;
	Stack.StepCompiledIn(&Asset);
	Stack.StepCompiledIn(&RandomNumber);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerController*)Context;

	if (!PlayerController || !PlayerController->MyFortPawn || !Asset) return;

	auto* AbilitySystemComponent = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->AbilitySystemComponent;
	FGameplayAbilitySpec NewSpec = {};
	UObject* AbilityToUse = nullptr;

	if (Asset->IsA<UAthenaSprayItemDefinition>()) {
		static auto SprayAbilityClass = Utils::FindObject<UBlueprintGeneratedClass>(L"/Game/Abilities/Sprays/GAB_Spray_Generic.GAB_Spray_Generic_C");
		AbilityToUse = SprayAbilityClass->DefaultObject;
	}
	//else if (auto ToyAsset = Asset->Cast<UAthenaToyItemDefinition>()) {
	//	AbilityToUse = ToyAsset->ToySpawnAbility->DefaultObject;
	//}
	else if (auto DanceAsset = Asset->Cast<UAthenaDanceItemDefinition>())
	{
		PlayerController->MyFortPawn->bMovingEmote = DanceAsset->bMovingEmote;
		PlayerController->MyFortPawn->EmoteWalkSpeed = DanceAsset->WalkForwardSpeed;
		static auto EmoteAbilityClass = Utils::FindObject<UBlueprintGeneratedClass>(L"/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C");
		AbilityToUse = EmoteAbilityClass->DefaultObject;
	}

	if (AbilityToUse) {
		((void (*)(FGameplayAbilitySpec*, UObject*, int, int, UObject*))(ImageBase + 0x1210fa4))(&NewSpec, AbilityToUse, 1, -1, Asset);
		FGameplayAbilitySpecHandle handle;
		((void (*)(UFortAbilitySystemComponent*, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec*, void*))(ImageBase + 0x4e01290))(AbilitySystemComponent, &handle, &NewSpec, nullptr);
	}

	if (bDev) {
		UFortAccoladeItemDefinition* Accolade = Utils::FindObject<UFortAccoladeItemDefinition>(L"/BattlePassPermanentQuests/Items/Accolades/AccoladeId_011_SearchAmmoBox.AccoladeId_011_SearchAmmoBox");
		XP::GiveAccolade((AFortPlayerControllerAthena*)PlayerController, Accolade, 100);
	}
}

void Player::ServerSendZiplineState(UObject* Context, FFrame& Stack)
{
	FZiplinePawnState State;

	Stack.StepCompiledIn(&State);
	Stack.IncrementCode();

	auto Pawn = (AFortPlayerPawn*)Context;

	if (!Pawn)
		return;

	Pawn->ZiplineState = State;

	((void (*)(AFortPlayerPawn*))(ImageBase + 0x673363c))(Pawn);

	if (State.bJumped)
	{
		auto Velocity = Pawn->CharacterMovement->Velocity;
		auto VelocityX = Velocity.X * -0.5f;
		auto VelocityY = Velocity.Y * -0.5f;
		Pawn->LaunchCharacterJump({ VelocityX >= -750 ? min(VelocityX, 750) : -750, VelocityY >= -750 ? min(VelocityY, 750) : -750, 1200 }, false, false, true, true);
	}
}

void Player::ServerHandlePickupInfo(UObject* Context, FFrame& Stack)
{
	AFortPickup* Pickup;
	FFortPickupRequestInfo Params;
	Stack.StepCompiledIn(&Pickup);
	Stack.StepCompiledIn(&Params);
	Stack.IncrementCode();
	auto Pawn = (AFortPlayerPawn*)Context;

	if (!Pawn || !Pickup || Pickup->bPickedUp)
		return;

	if ((Params.bTrySwapWithWeapon || Params.bUseRequestedSwap) && Pawn->CurrentWeapon && Inventory::GetQuickbar(Pawn->CurrentWeapon->WeaponData) == EFortQuickBars::Primary && Inventory::GetQuickbar(Pickup->PrimaryPickupItemEntry.ItemDefinition) == EFortQuickBars::Primary)
	{
		auto PC = (AFortPlayerControllerAthena*)Pawn->Controller;
		auto SwapEntry = PC->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
			{ return entry.ItemGuid == Params.SwapWithItem; });
		PC->SwappingItemDefinition = (UFortWorldItemDefinition*)SwapEntry; // proper af
	}
	Pawn->IncomingPickups.Add(Pickup);

	Pickup->PickupLocationData.bPlayPickupSound = Params.bPlayPickupSound;
	Pickup->PickupLocationData.FlyTime = 0.4f;
	Pickup->PickupLocationData.ItemOwner = Pawn;
	Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
	Pickup->PickupLocationData.PickupTarget = Pawn;
	//Pickup->PickupLocationData.StartDirection = Params.Direction.QuantizeNormal();
	Pickup->OnRep_PickupLocationData();

	Pickup->bPickedUp = true;
	Pickup->OnRep_bPickedUp();
}

void Player::MovingEmoteStopped(UObject* Context, FFrame& Stack)
{
	Stack.IncrementCode();

	AFortPawn* Pawn = (AFortPawn*)Context;
	Pawn->bMovingEmote = false;
	Pawn->bMovingEmoteFollowingOnly = false;
}

void Player::InternalPickup(AFortPlayerControllerAthena* PlayerController, FFortItemEntry PickupEntry)
{
	auto MaxStack = (int32)Utils::EvaluateScalableFloat(PickupEntry.ItemDefinition->MaxStackSize);
	int ItemCount = 0;
	for (auto& Item : PlayerController->WorldInventory->Inventory.ReplicatedEntries)
	{
		if (Inventory::GetQuickbar(Item.ItemDefinition) == EFortQuickBars::Primary)
			ItemCount += ((UFortWorldItemDefinition*)Item.ItemDefinition)->NumberOfSlotsToTake;
	}
	auto GiveOrSwap = [&]() {
		if (ItemCount == 5 && Inventory::GetQuickbar(PickupEntry.ItemDefinition) == EFortQuickBars::Primary) {
			if (Inventory::GetQuickbar(PlayerController->MyFortPawn->CurrentWeapon->WeaponData) == EFortQuickBars::Primary) {
				auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([PlayerController](FFortItemEntry& entry)
					{ return entry.ItemGuid == PlayerController->MyFortPawn->CurrentWeapon->ItemEntryGuid; });
				Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), *itemEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerController->MyFortPawn);
				Inventory::Remove(PlayerController, PlayerController->MyFortPawn->CurrentWeapon->ItemEntryGuid);
				Inventory::GiveItem(PlayerController, PickupEntry, PickupEntry.Count, true);
			}
			else {
				Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), (FFortItemEntry&)PickupEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerController->MyFortPawn);
			}
		}
		else
			Inventory::GiveItem(PlayerController, PickupEntry, PickupEntry.Count, true);
		};
	auto GiveOrSwapStack = [&](int32 OriginalCount) {
		if (PickupEntry.ItemDefinition->bAllowMultipleStacks && ItemCount < 5)
			Inventory::GiveItem(PlayerController, PickupEntry, OriginalCount - MaxStack, true);
		else
			Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), (FFortItemEntry&)PickupEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerController->MyFortPawn, OriginalCount - MaxStack);
		};
	if (PickupEntry.ItemDefinition->IsStackable()) {
		auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([PickupEntry, MaxStack](FFortItemEntry& entry)
			{ return entry.ItemDefinition == PickupEntry.ItemDefinition && entry.Count < MaxStack; });
		if (itemEntry) {
			auto State = itemEntry->StateValues.Search([](FFortItemEntryStateValue& Value)
				{ return Value.StateType == EFortItemEntryState::ShouldShowItemToast; });
			if (!State) {
				FFortItemEntryStateValue Value{};
				Value.StateType = EFortItemEntryState::ShouldShowItemToast;
				Value.IntValue = true;
				itemEntry->StateValues.Add(Value);
			}
			else State->IntValue = true;

			if ((itemEntry->Count += PickupEntry.Count) > MaxStack) {
				auto OriginalCount = itemEntry->Count;
				itemEntry->Count = MaxStack;

				GiveOrSwapStack(OriginalCount);
			}
			Inventory::ReplaceEntry(PlayerController, *itemEntry);
		}
		else {
			if (PickupEntry.Count > MaxStack) {
				auto OriginalCount = PickupEntry.Count;
				PickupEntry.Count = MaxStack;

				GiveOrSwapStack(OriginalCount);
			}
			GiveOrSwap();
		}
	}
	else {
		GiveOrSwap();
	}
}

bool Player::CompletePickupAnimation(AFortPickup* Pickup) {
	auto Pawn = (AFortPlayerPawnAthena*)Pickup->PickupLocationData.PickupTarget;
	if (!Pawn)
		return CompletePickupAnimationOG(Pickup);
	auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
	if (!PlayerController)
		return CompletePickupAnimationOG(Pickup);
	if (auto entry = (FFortItemEntry*)PlayerController->SwappingItemDefinition)
	{
		Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), *entry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerController->MyFortPawn);
		// SwapEntry(PC, *entry, Pickup->PrimaryPickupItemEntry);
		Inventory::Remove(PlayerController, entry->ItemGuid);
		Inventory::GiveItem(PlayerController, Pickup->PrimaryPickupItemEntry);
		PlayerController->SwappingItemDefinition = nullptr;
	}
	else
	{
		InternalPickup(PlayerController, Pickup->PrimaryPickupItemEntry);
	}
	return CompletePickupAnimationOG(Pickup);
}

void Player::NetMulticast_Athena_BatchedDamageCues(AFortPlayerPawnAthena* Pawn, FAthenaBatchedDamageGameplayCues_Shared SharedData, FAthenaBatchedDamageGameplayCues_NonShared NonSharedData)
{
	if (!Pawn || !Pawn->Controller || !Pawn->CurrentWeapon) return;

	if (Pawn->CurrentWeapon && !Pawn->CurrentWeapon->WeaponData->bUsesPhantomReserveAmmo && Inventory::GetStats(Pawn->CurrentWeapon->WeaponData) && Inventory::GetStats(Pawn->CurrentWeapon->WeaponData)->ClipSize > 0)
	{
		auto ent = ((AFortPlayerControllerAthena*)Pawn->Controller)->WorldInventory->Inventory.ReplicatedEntries.Search([Pawn](FFortItemEntry& entry)
			{ return entry.ItemGuid == Pawn->CurrentWeapon->ItemEntryGuid; });
		if (ent)
		{
			ent->LoadedAmmo = Pawn->CurrentWeapon->AmmoCount;
			Inventory::ReplaceEntry((AFortPlayerControllerAthena*)Pawn->Controller, *ent);
		}
	}
	else if (Pawn->CurrentWeapon && Pawn->CurrentWeapon->WeaponData->bUsesPhantomReserveAmmo)
	{
		auto ent = ((AFortPlayerControllerAthena*)Pawn->Controller)->WorldInventory->Inventory.ReplicatedEntries.Search([Pawn](FFortItemEntry& entry)
			{ return entry.ItemGuid == Pawn->CurrentWeapon->ItemEntryGuid; });
		if (ent)
		{
			ent->LoadedAmmo = Pawn->CurrentWeapon->AmmoCount;
			Inventory::ReplaceEntry((AFortPlayerControllerAthena*)Pawn->Controller, *ent);
		}
	}

	return NetMulticast_Athena_BatchedDamageCuesOG(Pawn, SharedData, NonSharedData);
}

void Player::ReloadWeapon(AFortWeapon* Weapon, int AmmoToRemove)
{
	AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)((AFortPlayerPawnAthena*)Weapon->Owner)->Controller;
	AFortInventory* Inventory;
	if (auto Bot = PC->Cast<AFortAthenaAIBotController>())
	{
		Inventory = Bot->Inventory;
	}
	else
	{
		Inventory = PC->WorldInventory;
	}
	if (!PC || !Inventory || !Weapon)
		return;
	if (Weapon->WeaponData->bUsesPhantomReserveAmmo)
	{
		Weapon->PhantomReserveAmmo -= AmmoToRemove;
		Weapon->OnRep_PhantomReserveAmmo();
		return;
	}
	auto Ammo = Weapon->WeaponData->GetAmmoWorldItemDefinition_BP();
	auto ent = Inventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{ return Weapon->WeaponData == Ammo ? entry.ItemGuid == Weapon->ItemEntryGuid : entry.ItemDefinition == Ammo; });
	auto WeaponEnt = Inventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{ return entry.ItemGuid == Weapon->ItemEntryGuid; });
	if (!WeaponEnt)
		return;

	if (ent)
	{
		ent->Count -= AmmoToRemove;
		if (ent->Count <= 0)
			Inventory::Remove(PC, ent->ItemGuid);
		else
			Inventory::ReplaceEntry(PC, *ent);
	}
	WeaponEnt->LoadedAmmo += AmmoToRemove;
	Inventory::ReplaceEntry(PC, *WeaponEnt);
}


void Player::ClientOnPawnDied(AFortPlayerControllerAthena* PlayerController, FFortPlayerDeathReport& DeathReport)
{
	if (!PlayerController)
		return ClientOnPawnDiedOG(PlayerController, DeathReport);
	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	auto GameState = (AFortGameStateAthena*)GameMode->GameState;
	auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;


	if (!GameState->IsRespawningAllowed(PlayerState) && PlayerController->WorldInventory && PlayerController->MyFortPawn)
	{
		bool bHasMats = false;
		for (auto& entry : PlayerController->WorldInventory->Inventory.ReplicatedEntries)
		{
			if (!entry.ItemDefinition->IsA<UFortWeaponMeleeItemDefinition>() && (entry.ItemDefinition->IsA<UFortResourceItemDefinition>() || entry.ItemDefinition->IsA<UFortWeaponRangedItemDefinition>() || entry.ItemDefinition->IsA<UFortConsumableItemDefinition>() || entry.ItemDefinition->IsA<UFortAmmoItemDefinition>()))
			{
				Inventory::SpawnPickup(PlayerController->MyFortPawn->K2_GetActorLocation(), entry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination, PlayerController->MyFortPawn);
			}
		}

		/*AFortAthenaMutator_ItemDropOnDeath* Mutator = (AFortAthenaMutator_ItemDropOnDeath*)GameState->GetMutatorByClass(GameMode, AFortAthenaMutator_ItemDropOnDeath::StaticClass());

		if (Mutator)
		{
			for (FItemsToDropOnDeath& Items : Mutator->ItemsToDrop)
			{
				Inventory::SpawnPickup(PlayerState->DeathInfo.DeathLocation, Items.ItemToDrop, (int)Utils::EvaluateScalableFloat(Items.NumberToDrop), 0, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination, PlayerController->MyFortPawn);
			}
		}*/
	}

	auto KillerPlayerState = (AFortPlayerStateAthena*)DeathReport.KillerPlayerState;
	auto KillerPawn = (AFortPlayerPawnAthena*)DeathReport.KillerPawn;

	//Log(L"DBNO: %d\n", PlayerController->MyFortPawn ? PlayerController->MyFortPawn->IsDBNO() : false);


	PlayerState->PawnDeathLocation = PlayerController->MyFortPawn ? PlayerController->MyFortPawn->K2_GetActorLocation() : FVector();
	PlayerState->DeathInfo.bDBNO = PlayerController->MyFortPawn ? PlayerController->MyFortPawn->IsDBNO() : false;
	PlayerState->DeathInfo.DeathLocation = PlayerState->PawnDeathLocation;
	PlayerState->DeathInfo.DeathTags = PlayerController->MyFortPawn ? *(FGameplayTagContainer*)(__int64(PlayerController->MyFortPawn) + 0x20a8) : DeathReport.Tags;
	PlayerState->DeathInfo.DeathCause = AFortPlayerStateAthena::ToDeathCause(PlayerState->DeathInfo.DeathTags, PlayerState->DeathInfo.bDBNO);
	PlayerState->DeathInfo.Downer = KillerPlayerState;
	PlayerState->DeathInfo.FinisherOrDowner = KillerPlayerState ? KillerPlayerState : PlayerState;
	PlayerState->DeathInfo.Distance = PlayerController->MyFortPawn ? (PlayerState->DeathInfo.DeathCause != EDeathCause::FallDamage ? (KillerPawn ? KillerPawn->GetDistanceTo(PlayerController->MyFortPawn) : 0) : PlayerController->MyFortPawn->Cast<AFortPlayerPawnAthena>()->LastFallDistance) : 0;
	PlayerState->DeathInfo.bInitialized = true;
	PlayerState->OnRep_DeathInfo();
	
	int playerCount = GameMode->AlivePlayers.Num() - 1; // we havent called removefromaliveplayer yet, so -1

	if (playerCount == 50)
	{
		static auto AccoladeId_026_Survival_Default_Bronze = Utils::FindObject<UFortAccoladeItemDefinition>(L"/BattlePassPermanentQuests/Items/Accolades/AccoladeId_026_Survival_Default_Bronze.AccoladeId_026_Survival_Default_Bronze");

		XP::GiveAccolade((AFortPlayerControllerAthena*)KillerPlayerState->Owner, AccoladeId_026_Survival_Default_Bronze);
	}
	else if (playerCount == 25)
	{
		static auto AccoladeId_027_Survival_Default_Silver = Utils::FindObject<UFortAccoladeItemDefinition>(L"/BattlePassPermanentQuests/Items/Accolades/AccoladeId_027_Survival_Default_Silver.AccoladeId_027_Survival_Default_Silver");

		XP::GiveAccolade((AFortPlayerControllerAthena*)KillerPlayerState->Owner, AccoladeId_027_Survival_Default_Silver);
	}
	else if (playerCount == 10)
	{
		static auto AccoladeId_028_Survival_Default_Gold = Utils::FindObject<UFortAccoladeItemDefinition>(L"/BattlePassPermanentQuests/Items/Accolades/AccoladeId_028_Survival_Default_Gold.AccoladeId_028_Survival_Default_Gold");

		XP::GiveAccolade((AFortPlayerControllerAthena*)KillerPlayerState->Owner, AccoladeId_028_Survival_Default_Gold);
	}

	if (playerCount == 5 || playerCount == 10 || playerCount == 25)
	{
		int points = 10;
		if (playerCount == 10)
			points = 15;

		for (auto& Player : GameMode->AlivePlayers)
		{
			auto Controller = (AFortPlayerControllerAthena*)Player;
			Controller->ClientReportTournamentPlacementPointsScored(5, points);
		}

		Tournaments::Placement(playerCount, points);
	}


	if (KillerPlayerState && KillerPawn && KillerPawn->Controller && KillerPawn->Controller->IsA<AFortPlayerControllerAthena>() && KillerPawn->Controller != PlayerController)
	{
		KillerPlayerState->KillScore++;
		KillerPlayerState->OnRep_Kills();
		KillerPlayerState->TeamKillScore++;
		KillerPlayerState->OnRep_TeamKillScore();
		
		KillerPlayerState->ClientReportKill(PlayerState);
		KillerPlayerState->ClientReportTeamKill(KillerPlayerState->TeamKillScore);

		auto KillerPC = (AFortPlayerControllerAthena*)KillerPlayerState->Owner;
		Tournaments::Kill(KillerPC);
		if (bTournament) Results::SendEventMatchResults(PlayerState->GetPlayerName().ToString().c_str(), eventId, { PlayerState->GetPlayerName().ToString().c_str() }, {}, "", PlayerState->Place, PlayerState->KillScore, 0);
		Log(L"Player %s killed %s", KillerPlayerState->GetPlayerName().ToWString().c_str(), PlayerController->PlayerState->GetPlayerName().ToWString().c_str());
	}
	if (PlayerController->MyFortPawn && KillerPlayerState && (KillerPlayerState->Place != 1 || PlayerState->Place != 1))
	{
		static auto AccoladeId_012_Elimination = Utils::FindObject<UFortAccoladeItemDefinition>(L"/BattlePassPermanentQuests/Items/Accolades/AccoladeId_012_Elimination.AccoladeId_012_Elimination");

		XP::GiveAccolade((AFortPlayerControllerAthena*)KillerPlayerState->Owner, AccoladeId_012_Elimination);
	}
	static bool bFirstDied = false;
	if (!bFirstDied && KillerPlayerState && KillerPlayerState->Place != 1 && PlayerState->Place != 1)
	{
		bFirstDied = true;
		static auto AccoladeId_017_First_Elimination = Utils::FindObject<UFortAccoladeItemDefinition>(L"/BattlePassPermanentQuests/Items/Accolades/AccoladeId_017_First_Elimination.AccoladeId_017_First_Elimination");

		XP::GiveAccolade((AFortPlayerControllerAthena*)KillerPlayerState->Owner, AccoladeId_017_First_Elimination);
	}

	if (!GameState->IsRespawningAllowed(PlayerState) && (PlayerController->MyFortPawn ? !PlayerController->MyFortPawn->IsDBNO() : true))
	{
		PlayerState->Place = GameState->PlayersLeft;
		PlayerState->OnRep_Place();
		FAthenaMatchStats& Stats = PlayerController->MatchReport->MatchStats;
		FAthenaMatchTeamStats& TeamStats = PlayerController->MatchReport->TeamStats;

		Stats.Stats[3] = PlayerState->KillScore;
		Stats.Stats[8] = PlayerState->SquadId;
		PlayerController->ClientSendMatchStatsForPlayer(Stats);

		TeamStats.Place = PlayerState->Place;
		TeamStats.TotalPlayers = GameState->TotalPlayers;
		PlayerController->ClientSendTeamStatsForPlayer(TeamStats);


		AFortWeapon* DamageCauser = nullptr;
		if (auto Projectile = DeathReport.DamageCauser ? DeathReport.DamageCauser->Cast<AFortProjectileBase>() : nullptr)
			DamageCauser = Projectile->GetOwnerWeapon();
		else if (auto Weapon = DeathReport.DamageCauser ? DeathReport.DamageCauser->Cast<AFortWeapon>() : nullptr)
			DamageCauser = Weapon;

		((void (*)(AFortGameModeAthena*, AFortPlayerController*, APlayerState*, AFortPawn*, UFortWeaponItemDefinition*, EDeathCause, char))(ImageBase + 0x5f9d3a8))(GameMode, PlayerController, KillerPlayerState == PlayerState ? nullptr : KillerPlayerState, KillerPawn, DamageCauser ? DamageCauser->WeaponData : nullptr, PlayerState->DeathInfo.DeathCause, 0);

		PlayerController->ClientSendEndBattleRoyaleMatchForPlayer(true, PlayerController->MatchReport->EndOfMatchResults);

		PlayerController->StateName = FName(L"Spectating");
		
		if (PlayerController->MyFortPawn && KillerPlayerState && KillerPawn && KillerPawn->Controller != PlayerController)
		{
			auto Handle = KillerPlayerState->AbilitySystemComponent->MakeEffectContext();
			FGameplayTag Tag;
			static auto Cue = FName(L"GameplayCue.Shield.PotionConsumed");
			Tag.TagName = Cue;
			KillerPlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(Tag, FPredictionKey(), Handle);
			KillerPlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(Tag, FPredictionKey(), Handle);

			auto Health = KillerPawn->GetHealth();
			auto Shield = KillerPawn->GetShield();

			if (Health == 100)
			{
				Shield += Shield + 50;
			}
			else if (Health + 50 > 100)
			{
				Health = 100;
				Shield += (Health + 50) - 100;
			}
			else if (Health + 50 <= 100)
			{
				Health += 50;
			}

			KillerPawn->SetHealth(Health);
			KillerPawn->SetShield(Shield);
			//forgot to add this back
		}
		if (PlayerController->MyFortPawn && ((KillerPlayerState && KillerPlayerState->Place == 1) || PlayerState->Place == 1))
		{
			if (PlayerState->Place == 1)
			{
				KillerPlayerState = PlayerState;
				KillerPawn = (AFortPlayerPawnAthena*)PlayerController->MyFortPawn;
			}
			
			auto KillerPlayerController = (AFortPlayerControllerAthena*)KillerPlayerState->Owner;
			auto KillerWeapon = DamageCauser ? DamageCauser->WeaponData : nullptr;

			Tournaments::PlacementForController(KillerPlayerController, 1);
			
			KillerPlayerController->PlayWinEffects(KillerPawn, KillerWeapon, PlayerState->DeathInfo.DeathCause, false);
			KillerPlayerController->ClientNotifyWon(KillerPawn, KillerWeapon, PlayerState->DeathInfo.DeathCause);
			KillerPlayerController->ClientNotifyTeamWon(KillerPawn, KillerWeapon, PlayerState->DeathInfo.DeathCause);
			
			if (KillerPlayerState != PlayerState)
			{
				auto Crown = Utils::FindObject<UFortItemDefinition>(L"/VictoryCrownsGameplay/Items/AGID_VictoryCrown.AGID_VictoryCrown");
				Inventory::GiveItem(KillerPlayerController, Crown, 1);

				KillerPlayerController->ClientSendEndBattleRoyaleMatchForPlayer(true, KillerPlayerController->MatchReport->EndOfMatchResults);

				FAthenaMatchStats& KillerStats = KillerPlayerController->MatchReport->MatchStats;
				FAthenaMatchTeamStats& KillerTeamStats = KillerPlayerController->MatchReport->TeamStats;


				KillerStats.Stats[3] = KillerPlayerState->KillScore;
				KillerStats.Stats[8] = KillerPlayerState->SquadId;
				KillerPlayerController->ClientSendMatchStatsForPlayer(KillerStats);

				KillerTeamStats.Place = KillerPlayerState->Place;
				KillerTeamStats.TotalPlayers = GameState->TotalPlayers;
				KillerPlayerController->ClientSendTeamStatsForPlayer(KillerTeamStats);
			}

			GameState->WinningTeam = KillerPlayerState->TeamIndex;
			GameState->OnRep_WinningTeam();
			GameState->WinningPlayerState = KillerPlayerState;
			GameState->OnRep_WinningPlayerState();

			
			//Utils::KillGameserver();
		}
	}

	std::thread([PlayerController]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (PlayerController && PlayerController->MyFortPawn)
		{
			Misc::PlayersToDestroyLocked = true;
			Misc::PlayersToDestroy.push_back(PlayerController->MyFortPawn);
			Misc::PlayersToDestroyLocked = false;
		}
		}).detach();
	return ClientOnPawnDiedOG(PlayerController, DeathReport);
}


void Player::ServerAttemptInventoryDrop(UObject* Context, FFrame& Stack)
{
	FGuid Guid;
	int32 Count;
	bool bTrash;
	Stack.StepCompiledIn(&Guid);
	Stack.StepCompiledIn(&Count);
	Stack.StepCompiledIn(&bTrash);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	if (!PlayerController || !PlayerController->Pawn)
		return;
	auto ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{ return entry.ItemGuid == Guid; });
	if (!ItemEntry || (ItemEntry->Count - Count) < 0)
		return;
	ItemEntry->Count -= Count;
	Inventory::SpawnPickup(PlayerController->Pawn->K2_GetActorLocation() + PlayerController->Pawn->GetActorForwardVector() * 70.f + FVector(0, 0, 50), *ItemEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerController->MyFortPawn, Count);
	if (ItemEntry->Count == 0)
		Inventory::Remove(PlayerController, Guid);
	else
		Inventory::ReplaceEntry(PlayerController, *ItemEntry);
}

void Player::ServerClientIsReadyToRespawn(UObject* Context, FFrame& Stack)
{
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	auto PlayerState = PlayerController->PlayerState->Cast<AFortPlayerStateAthena>();

	if (PlayerState->RespawnData.bRespawnDataAvailable && PlayerState->RespawnData.bServerIsReady)
	{
		PlayerState->RespawnData.bClientIsReady = true;

		FTransform Transform(PlayerState->RespawnData.RespawnLocation, PlayerState->RespawnData.RespawnRotation);
		auto Pawn = (AFortPlayerPawnAthena*)UWorld::GetWorld()->AuthorityGameMode->SpawnDefaultPawnAtTransform(PlayerController, Transform);
		PlayerController->Possess(Pawn);
		Pawn->SetHealth(100);
		Pawn->SetShield(100);

		PlayerController->RespawnPlayerAfterDeath(true);
		Pawn->BeginSkydiving(true);
	}
}

void Player::ServerGiveCreativeItem(AFortPlayerControllerAthena* Controller, FFortItemEntry& CreativeItem, FGuid& ItemToRemoveGuid)
{
	auto WeaponStats = CreativeItem.ItemDefinition->IsA<UFortWeaponItemDefinition>() ? Inventory::GetStats((UFortWeaponItemDefinition*)CreativeItem.ItemDefinition) : nullptr;
	Inventory::GiveItem(Controller, CreativeItem.ItemDefinition, CreativeItem.Count, WeaponStats ? WeaponStats->ClipSize : 0, CreativeItem.Level);
}

void Player::OnCapsuleBeginOverlap(UObject* Context, FFrame& Stack)
{
	UPrimitiveComponent* OverlappedComp;
	AActor* OtherActor;
	UPrimitiveComponent* OtherComp;
	int32 OtherBodyIndex;
	bool bFromSweep;
	FHitResult SweepResult;
	Stack.StepCompiledIn(&OverlappedComp);
	Stack.StepCompiledIn(&OtherActor);
	Stack.StepCompiledIn(&OtherComp);
	Stack.StepCompiledIn(&OtherBodyIndex);
	Stack.StepCompiledIn(&bFromSweep);
	Stack.StepCompiledIn(&SweepResult);
	Stack.IncrementCode();
	auto Pawn = (AFortPlayerPawn*)Context;
	if (!Pawn || !Pawn->Controller)
		return callOG(Pawn, Stack.CurrentNativeFunction, OnCapsuleBeginOverlap, OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	auto Pickup = OtherActor->Cast<AFortPickup>();
	if (!Pickup || !Pickup->PrimaryPickupItemEntry.ItemDefinition)
		return callOG(Pawn, Stack.CurrentNativeFunction, OnCapsuleBeginOverlap, OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	auto MaxStack = (int32) Utils::EvaluateScalableFloat(Pickup->PrimaryPickupItemEntry.ItemDefinition->MaxStackSize);
	auto itemEntry = ((AFortPlayerControllerAthena*)Pawn->Controller)->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{ return entry.ItemDefinition == Pickup->PrimaryPickupItemEntry.ItemDefinition && entry.Count <= MaxStack; });
	if (Pickup && Pickup->PawnWhoDroppedPickup != Pawn)
	{
		if ((!itemEntry && Inventory::GetQuickbar(Pickup->PrimaryPickupItemEntry.ItemDefinition) == EFortQuickBars::Secondary) || (itemEntry && itemEntry->Count < MaxStack))
			Pawn->ServerHandlePickup(Pickup, 0.4f, FVector(), true);
	}
	return callOG(Pawn, Stack.CurrentNativeFunction, OnCapsuleBeginOverlap, OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void Player::ServerReviveFromDBNO(AFortPlayerPawn* Pawn, AController* EventInstigator)
{
	auto Controller = reinterpret_cast<AFortPlayerControllerAthena*>(Pawn->GetController());
	if (!Controller) return;

	auto PlayerState = reinterpret_cast<AFortPlayerState*>(Controller->PlayerState);
	if (!PlayerState) return;

	auto AbilitySystemComp = (UFortAbilitySystemComponentAthena*)PlayerState->AbilitySystemComponent;
	
	FGameplayEventData Data{};
	Data.EventTag = Pawn->EventReviveTag;
	Data.ContextHandle = PlayerState->AbilitySystemComponent->MakeEffectContext();
	Data.Instigator = EventInstigator;
	Data.Target = Pawn;
	Data.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Pawn);
	Data.TargetTags = Pawn->GameplayTags;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn, Pawn->EventReviveTag, Data);

	for (auto& Ability : AbilitySystemComp->ActivatableAbilities.Items)
	{
		if (Ability.Ability->Class == UGAB_AthenaDBNO_C::StaticClass())
		{
			AbilitySystemComp->ServerCancelAbility(Ability.Handle, Ability.ActivationInfo);
			AbilitySystemComp->ServerEndAbility(Ability.Handle, Ability.ActivationInfo, Ability.ActivationInfo.PredictionKeyWhenActivated);
			AbilitySystemComp->ClientCancelAbility(Ability.Handle, Ability.ActivationInfo);
			AbilitySystemComp->ClientEndAbility(Ability.Handle, Ability.ActivationInfo);
			break;
		}
	}
	
	Pawn->bIsDBNO = false;
	Pawn->bPlayedDying = false;

	Pawn->SetHealth(30); 
	Pawn->OnRep_IsDBNO();

	Controller->ClientOnPawnRevived(EventInstigator); 
	Controller->RespawnPlayerAfterDeath(false);
	if (auto PawnAthena = reinterpret_cast<AFortPlayerPawnAthena*>(Pawn))
	{
		if (!PawnAthena->IsDBNO())
		{
			PawnAthena->DBNORevivalStacking = 0;
		}
	}
}

void Player::OnPlayImpactFX(AFortWeapon* Weapon, FHitResult& HitResult, EPhysicalSurface ImpactPhysicalSurface, UFXSystemComponent* SpawnedPSC)
{
	auto Pawn = (AFortPlayerPawnAthena*)Weapon->GetOwner();
	auto Controller = (AFortPlayerControllerAthena*)Pawn->Controller;
	auto ent = Controller->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
		return entry.ItemGuid == Pawn->CurrentWeapon->ItemEntryGuid;
		});

	if (ent)
	{
		ent->LoadedAmmo = Pawn->CurrentWeapon->AmmoCount;
		Inventory::ReplaceEntry(Controller, *ent);
	}
	return OnPlayImpactFXOG(Weapon, HitResult, ImpactPhysicalSurface, SpawnedPSC);
}

void TeleportPlayerPawn(UObject* Context, FFrame& Stack, bool* Ret)
{
	UObject* WorldContextObject;
	AFortPlayerPawn* PlayerPawn;
	FVector DestLocation;
	FRotator DestRotation;
	bool bIgnoreCollision;
	bool bIgnoreSupplementalKillVolumeSweep;

	Stack.StepCompiledIn(&WorldContextObject);
	Stack.StepCompiledIn(&PlayerPawn);
	Stack.StepCompiledIn(&DestLocation);
	Stack.StepCompiledIn(&DestRotation);
	Stack.StepCompiledIn(&bIgnoreCollision);
	Stack.StepCompiledIn(&bIgnoreSupplementalKillVolumeSweep);
	Stack.IncrementCode();

	PlayerPawn->K2_TeleportTo(DestLocation, DestRotation);
	*Ret = true;
}

void ServerChangeName(AFortPlayerControllerAthena* Controller, FFrame& Stack)
{
	Stack.IncrementCode();
	uint8_t _NetRes[24];
	auto ConstructNetRes = (void (*)(void*, uint32)) (ImageBase + 0x1599788);
	void (*CloseNetConnection)(UNetConnection*, void*) = decltype(CloseNetConnection)(ImageBase + 0x1599018);
	ConstructNetRes(_NetRes, 4);
	CloseNetConnection(Controller->NetConnection, _NetRes);
}

bool IsTeamDead(AFortPlayerControllerAthena* PlayerController)
{
	for (auto& TeamMember : ((AFortPlayerStateAthena*)PlayerController->PlayerState)->PlayerTeam->TeamMembers)
	{
		auto PlayerController = TeamMember->Cast<AFortPlayerControllerAthena>();

		if (!PlayerController)
			continue;

		if (!PlayerController->bMarkedAlive || (PlayerController->MyFortPawn && PlayerController->MyFortPawn->bIsDBNO))
			return true;
	}

	return false;
}

TArray<TWeakObjectPtr<AFortPlayerStateAthena>>* FPlayerStateGroupTable__Find(AFortGameStateAthena* GameState, int32 TeamIndex)
{
	static TArray<TWeakObjectPtr<AFortPlayerStateAthena>> TeamMemberList[100];
	auto& TeamMembers = TeamMemberList[TeamIndex % 100];

	for (auto& Player : GameState->PlayerArray)
	{
		auto PlayerState = Player->Cast<AFortPlayerStateAthena>();

		if (!PlayerState || PlayerState->TeamIndex != TeamIndex)
			continue;

		TeamMembers.Add(PlayerState);
	}

	return &TeamMembers;
}
void Player::Hook()
{
	Utils::ExecHook(L"/Script/FortniteGame.FortPlayerControllerAthena.ServerClientIsReadyToRespawn", ServerClientIsReadyToRespawn);
	Utils::ExecHook(L"/Script/FortniteGame.FortPlayerController.ServerReadyToStartMatch", ServerReadyToStartMatch, ServerReadyToStartMatchOG);
	Utils::ExecHook(L"/Script/Engine.PlayerController.ServerAcknowledgePossession", ServerAcknowledgePossession);
	Utils::Hook(ImageBase + 0xee8834, GetPlayerViewPoint, GetPlayerViewPointOG);
	Utils::ExecHook(L"/Script/FortniteGame.FortPlayerController.ServerPlayEmoteItem", ServerPlayEmoteItem);
	Utils::ExecHook(L"/Script/FortniteGame.FortPlayerController.ServerExecuteInventoryItem", ServerExecuteInventoryItem);
	Utils::ExecHook(L"/Script/FortniteGame.FortPlayerController.ServerReturnToMainMenu", ServerReturnToMainMenu);
	Utils::ExecHook(L"/Script/FortniteGame.FortControllerComponent_Aircraft.ServerAttemptAircraftJump", ServerAttemptAircraftJump, ServerAttemptAircraftJumpOG);
	Utils::ExecHook(L"/Script/FortniteGame.FortPawn.MovingEmoteStopped", MovingEmoteStopped);
	Utils::ExecHook(L"/Script/FortniteGame.FortPlayerPawn.ServerSendZiplineState", ServerSendZiplineState);
	Utils::ExecHook(L"/Script/FortniteGame.FortPlayerPawn.ServerHandlePickupInfo", ServerHandlePickupInfo);
	Utils::Hook(ImageBase + 0x64de9fc, CompletePickupAnimation, CompletePickupAnimationOG);
	Utils::Hook<AFortPlayerPawnAthena>(uint32(0x12c), NetMulticast_Athena_BatchedDamageCues, NetMulticast_Athena_BatchedDamageCuesOG);
	Utils::Hook<AFortPlayerPawnAthena>(uint32(0x204), ServerReviveFromDBNO);
	Utils::Hook<AFortPlayerControllerAthena>(uint32(0x4e8), ServerGiveCreativeItem);
	// 0x204
	Utils::Hook(ImageBase + 0x1c853e4, OnPlayImpactFX, OnPlayImpactFXOG);
	Utils::Hook(ImageBase + 0x69b25b8, ReloadWeapon);
	Utils::Hook(ImageBase + 0x6c26888, ClientOnPawnDied, ClientOnPawnDiedOG);
	Utils::ExecHook(L"/Script/FortniteGame.FortPlayerController.ServerAttemptInventoryDrop", ServerAttemptInventoryDrop);
	Utils::ExecHook(L"/Script/FortniteGame.FortPlayerPawn.OnCapsuleBeginOverlap", OnCapsuleBeginOverlap, OnCapsuleBeginOverlapOG);
	Utils::ExecHook(L"/Script/FortniteGame.FortMissionLibrary.TeleportPlayerPawn", TeleportPlayerPawn);
	Utils::ExecHook(L"/Script/Engine.PlayerController.ServerChangeName", ServerChangeName);
	Utils::Hook(ImageBase + 0x5FFEF6C, IsTeamDead);
	Utils::Hook(ImageBase + 0x12BB564, FPlayerStateGroupTable__Find); // alternative to teamsarraycontainer
}
