#include "pch.h"
#include "Misc.h"
#include "Options.h"
#include <wininet.h>
#include <vector>
#include <string>
#include <algorithm>
#include "API.h"
#include "XP.H"
#pragma comment(lib, "wininet.lib")

int Misc::GetNetMode() {
	return 1;
}

float Misc::GetMaxTickRate(UEngine* Engine, float DeltaTime, bool bAllowFrameRateSmoothing) {
	// improper, DS is supposed to do hitching differently
	return std::clamp(1.f / DeltaTime, 1.f, 120.f);
}

bool Misc::RetTrue() { return true; }

void Misc::TickFlush(UNetDriver* Driver, float DeltaTime)
{
	if (Driver->ReplicationDriver)
	{
		if (!bDev)
		{
			static bool hasAClientConnected = false;
			if (!hasAClientConnected && Driver->ClientConnections.Num() > 0) hasAClientConnected = true;
			else if (hasAClientConnected && Driver->ClientConnections.Num() == 0) {
				TerminateProcess(GetCurrentProcess(), 0);
			}
		}

		if (!PlayersToDestroyLocked && PlayersToDestroy.size() > 0)
		{
			for (int i = 0; i < PlayersToDestroy.size(); i++)
				PlayersToDestroy[i]->K2_DestroyActor();

			PlayersToDestroy.clear();
		}

		Funcs::ServerReplicateActors(Driver->ReplicationDriver, DeltaTime);
	}

	return TickFlushOG(Driver, DeltaTime);
}

void* Misc::DispatchRequest(void* Arg1, void* MCPData, int)
{
	return DispatchRequestOG(Arg1, MCPData, 3);
}

bool Misc::Listen() {
	auto World = UWorld::GetWorld();
	auto Engine = UEngine::GetEngine();
	auto NetDriverName = FName(L"GameNetDriver");
	FWorldContext* WorldCtx = ((FWorldContext * (*)(UEngine*, UWorld*)) (ImageBase + Sarah::Offsets::GetWorldContext))(Engine, World);
	auto NetDriver = World->NetDriver = ((UNetDriver * (*)(UEngine*, FWorldContext*, FName))(ImageBase + Sarah::Offsets::CreateNetDriverWorldContext))(Engine, WorldCtx, NetDriverName);

	NetDriver->NetDriverName = NetDriverName;
	NetDriver->World = World;

	for (auto& Collection : World->LevelCollections) Collection.NetDriver = NetDriver;

	FURL __Url_Cpy = World->PersistentLevel->URL;
	auto __Rnd = rand();
	//__Url_Cpy.Port = __Port = __Rnd > 512 ? (__Rnd * 2) + 1 : __Rnd + 1024;
	FString Err;
	if (Funcs::InitListen(NetDriver, World, __Url_Cpy, false, Err)) {
		Funcs::SetWorld(NetDriver, World);
		//NetDriver->NetServerMaxTickRate = 15; // zero delay method (dont leak)
	}
	else {
		Log(L"Failed to listen");
	}
	return true;
}

bool Misc::RetFalse()
{
	return false;
}


void Misc::SetDynamicFoundationEnabled(UObject* Context, FFrame& Stack)
{
	auto Foundation = (ABuildingFoundation*)Context;
	bool bEnabled;
	Stack.StepCompiledIn(&bEnabled);
	Stack.IncrementCode();
	Foundation->DynamicFoundationRepData.EnabledState = bEnabled ? EDynamicFoundationEnabledState::Enabled : EDynamicFoundationEnabledState::Disabled;
	Foundation->OnRep_DynamicFoundationRepData();
	Foundation->FoundationEnabledState = bEnabled ? EDynamicFoundationEnabledState::Enabled : EDynamicFoundationEnabledState::Disabled;
}

void Misc::SetDynamicFoundationTransform(UObject* Context, FFrame& Stack)
{
	auto Foundation = (ABuildingFoundation*)Context;
	auto& Transform = Stack.StepCompiledInRef<FTransform>();
	Stack.IncrementCode();
	Foundation->DynamicFoundationTransform = Transform;
	Foundation->DynamicFoundationRepData.Rotation = Transform.Rotation.Rotator();
	Foundation->DynamicFoundationRepData.Translation = Transform.Translation;
	Foundation->StreamingData.FoundationLocation = Transform.Translation;
	Foundation->StreamingData.BoundingBox = Foundation->StreamingBoundingBox;
	Foundation->OnRep_DynamicFoundationRepData();
}



void Misc::StartNewSafeZonePhase(AFortGameModeAthena* GameMode, int a2)
{
	auto GameState = (AFortGameStateAthena*)GameMode->GameState;

	FFortSafeZoneDefinition* SafeZoneDefinition = &GameState->MapInfo->SafeZoneDefinition;
	TArray<float>& Durations = *(TArray<float>*)(__int64(SafeZoneDefinition) + 0x248);
	TArray<float>& HoldDurations = *(TArray<float>*)(__int64(SafeZoneDefinition) + 0x238);

	constexpr static std::array<float, 8> LateGameDurations{
		0.f,
		65.f,
		60.f,
		50.f,
		45.f,
		35.f,
		30.f,
		40.f,
	};

	constexpr static std::array<float, 8> LateGameHoldDurations{
		0.f,
		60.f,
		55.f,
		50.f,
		45.f,
		30.f,
		0.f,
		0.f,
	};


	if (bLateGame && GameMode->SafeZonePhase < 3)
	{
		GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
		GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + 0.15f;
	}
	else
	{
		if (bLateGame && GameMode->SafeZonePhase == 3)
		{
			auto Duration = /*bLateGame ? LateGameDurations[GameMode->SafeZonePhase - 1] : */Durations[GameMode->SafeZonePhase + 1];

			GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 30.f;
			GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Duration;
		}
		else
		{
			auto Duration = /*bLateGame ? LateGameDurations[GameMode->SafeZonePhase - 1] : */Durations[GameMode->SafeZonePhase + 1];
			auto HoldDuration = /*bLateGame ? LateGameHoldDurations[GameMode->SafeZonePhase - 1] : */HoldDurations[GameMode->SafeZonePhase + 1];

			GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + HoldDuration;
			GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Duration;
		}

		for (auto& Player : GameMode->AlivePlayers)
		{
			static auto AccoladeID_SurviveStormCircle = Utils::FindObject<UFortAccoladeItemDefinition>(L"/BattlePassPermanentQuests/Items/Accolades/AccoladeID_SurviveStormCircle.AccoladeID_SurviveStormCircle");
			XP::GiveAccolade(Player, AccoladeID_SurviveStormCircle, 150);
		}
	}


	StartNewSafeZonePhaseOG(GameMode, a2);
}

bool Misc::StartAircraftPhase(AFortGameModeAthena* GameMode, char a2)
{
	auto Ret = StartAircraftPhaseOG(GameMode, a2);
        std::string playlist = bCreative ? "Playlist_PlaygroundV2" : "Playlist_ShowdownAlt_Solo";

	if (!bDev)
	{
		API::GameServer("http://185.206.149.110:45011/solstice/api/v1/matchmaking/stop/by-address", IP, 7777);
	}

	auto GameState = (AFortGameStateAthena*)GameMode->GameState;
	
	if (bLateGame)
	{
		GameState->GamePhase = EAthenaGamePhase::SafeZones;
		GameState->GamePhaseStep = EAthenaGamePhaseStep::StormHolding;
		GameState->OnRep_GamePhase(EAthenaGamePhase::Aircraft);

		auto Aircraft = GameState->Aircrafts[0];
		Aircraft->FlightInfo.FlightSpeed = 0.f;
		FVector Loc = GameMode->SafeZoneLocations[3];
		Loc.Z = 17500.f;

		Aircraft->FlightInfo.FlightStartLocation = (FVector_NetQuantize100) Loc;
		
		Aircraft->FlightInfo.TimeTillFlightEnd = 7.f;
		Aircraft->FlightInfo.TimeTillDropEnd = 0.f;
		Aircraft->FlightInfo.TimeTillDropStart = 0.f;
		Aircraft->FlightStartTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
		Aircraft->FlightEndTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 7.f;
		//GameState->bAircraftIsLocked = false;
		GameState->SafeZonesStartTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 7.f;
	}

	return Ret;
}

uint32 Misc::CheckCheckpointHeartBeat()
{
	return -1;
}

void Misc::ReturnNull()
{
	return;
}

static std::vector<std::string> LoggedFunctions;
void* Misc::ProcessEvent(UObject* Object, void* pFunction, void* Parms)
{
	if (!Object || !pFunction || !Parms)
		return ProcessEventOG(Object, pFunction, Parms);

	UFunction* Function = static_cast<UFunction*>(pFunction);
	if (!Function)
		return ProcessEventOG(Object, pFunction, Parms);

	std::string FuncName = Function->GetFullName().c_str();

	if (std::find(LoggedFunctions.begin(), LoggedFunctions.end(), FuncName) == LoggedFunctions.end()) {
		std::cout << FuncName << std::endl;
		LoggedFunctions.push_back(FuncName);
	}

	return ProcessEventOG(Object, pFunction, Parms);
}

void Misc::Athena_MedConsumable_Triggered(UObject* Context, FFrame& Stack)
{
	UGA_Athena_MedConsumable_Parent_C* Consumable = (UGA_Athena_MedConsumable_Parent_C*)Context;

	if (!Consumable || (!Consumable->HealsShields && !Consumable->HealsHealth) || !Consumable->PlayerPawn)
		return Athena_MedConsumable_TriggeredOG(Context, Stack);

	auto Handle = Consumable->PlayerPawn->AbilitySystemComponent->MakeEffectContext();
	FGameplayTag Tag{};
	static auto ShieldCue = FName(L"GameplayCue.Shield.PotionConsumed");
	static auto HealthCue = FName(L"GameplayCue.Athena.Health.HealUsed");
	FName CueName = Consumable->HealsShields ? ShieldCue : HealthCue;
	if (Consumable->HealsHealth && Consumable->HealsShields) {
		//if (Consumable->PlayerPawn->GetHealth() + Consumable->HealthHealAmount <= 100) CueName = HealthCue;
	}
	Tag.TagName = CueName;
	Consumable->PlayerPawn->AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(Tag, FPredictionKey(), Handle);
	Consumable->PlayerPawn->AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(Tag, FPredictionKey(), Handle);

	return Athena_MedConsumable_TriggeredOG(Context, Stack);
}

const wchar_t* Misc::GetCommandLet()
{
	static auto cmdLine = UEAllocatedWString(GetCommandLetOG()) + L" -AllowAllPlaylistsInShipping";
	return cmdLine.c_str();
}

FServicePermissionsMcp* Misc::MatchMakingServicePerms(int64, int64)
{
	FServicePermissionsMcp* Perms = new FServicePermissionsMcp();
	Perms->Id = L"ec684b8c687f479fadea3cb2ad83f5c6";
	return Perms;
}

// 7FF66ECBAF00
bool (*NetSerializeOG)(FGameplayAbilityTargetDataHandle* AbilityTargetDataHandle, __int64& a2, UPackageMap* Map, bool& bOutSuccess);
bool NetSerialize(FGameplayAbilityTargetDataHandle* AbilityTargetDataHandle, __int64& a2, UPackageMap* Map, bool& bOutSuccess)
{
	bool bResult = NetSerializeOG(AbilityTargetDataHandle, a2, Map, bOutSuccess);

	if (bResult)
	{
		UIpConnection* IpConnection = Map->Outer->Cast<UIpConnection>();

		if (!IpConnection)
			return bResult;

		auto PlayerController = (AFortPlayerControllerAthena*) IpConnection->PlayerController;

		if (PlayerController)
		{
			uint8 DataNum = 0;

			if (*(int32*)(__int64(AbilityTargetDataHandle) + 0x20))
				DataNum = *(int32*)(__int64(AbilityTargetDataHandle) + 0x20);

			if (DataNum > 25)
			{
				uint8_t _NetRes[24];
				auto ConstructNetRes = (void (*)(void*, uint32)) (ImageBase + 0x1599788);
				void (*CloseNetConnection)(UNetConnection*, void*) = decltype(CloseNetConnection)(ImageBase + 0x1599018);
				ConstructNetRes(_NetRes, 4);
				CloseNetConnection(PlayerController->NetConnection, _NetRes);
			}
		}
	}

	return bResult;
}

void Misc::Hook() {
	//if (bDev && bCreative) Utils::Hook(ImageBase + SDK::Offsets::ProcessEvent, ProcessEvent, ProcessEventOG);
	Utils::Hook(ImageBase + 0x7a4891c, Listen);
	Utils::Hook(ImageBase + Sarah::Offsets::GetNetMode, GetNetMode);
	Utils::Hook(ImageBase + Sarah::Offsets::GetMaxTickRate, GetMaxTickRate, GetMaxTickRateOG);
	Utils::Hook(ImageBase + Sarah::Offsets::TickFlush, TickFlush, TickFlushOG);
	Utils::Hook(ImageBase + Sarah::Offsets::DispatchRequest, DispatchRequest, DispatchRequestOG);
	Utils::Patch<uint8_t>(ImageBase + 0x677f0e4, 0xc3);
	Utils::Patch<uint8_t>(ImageBase + 0x44a9b00, 0xc3);
	Utils::Patch<uint16>(ImageBase + 0x65b510f, 0xe990);
	Utils::Patch<uint16>(ImageBase + 0x65F9112, 0x9090);
	Utils::Patch<uint16>(ImageBase + 0x5F8D1CF, 0xe990);
	Utils::Patch<uint8>(ImageBase + 0x66A3F7B, 0xeb);
	Utils::ExecHook(L"/Script/FortniteGame.BuildingFoundation.SetDynamicFoundationTransform", SetDynamicFoundationTransform);
	Utils::ExecHook(L"/Script/FortniteGame.BuildingFoundation.SetDynamicFoundationEnabled", SetDynamicFoundationEnabled);
	Utils::Hook(ImageBase + 0x5fa67bc, StartNewSafeZonePhase, StartNewSafeZonePhaseOG);
	//Utils::Patch<uint8_t>(ImageBase + 0x7a7533e, 0x74);
	Utils::Patch<uint8_t>(ImageBase + GameSessionPatch, 0x85);
	for (auto& NullFunc : Sarah::Offsets::NullFuncs)
		Utils::Patch<uint8_t>(ImageBase + NullFunc, 0xc3);
	for (auto& RetTrueFunc : Sarah::Offsets::RetTrueFuncs) {
		Utils::Patch<uint32_t>(ImageBase + RetTrueFunc, 0xc0ffc031);
		Utils::Patch<uint8_t>(ImageBase + RetTrueFunc + 4, 0xc3);
	}
	//
	Utils::Hook(ImageBase + 0x44a9b00, CheckCheckpointHeartBeat);
	Utils::Hook(ImageBase + 0x5fa4538, StartAircraftPhase, StartAircraftPhaseOG);
	Utils::Hook(ImageBase + 0xB71D9C, GetCommandLet, GetCommandLetOG);
	Utils::Hook(ImageBase + 0x17F5A4C, MatchMakingServicePerms);
	Utils::Hook(ImageBase + 0x4E03B50, NetSerialize, NetSerializeOG);
	Utils::Patch<uint16>(ImageBase + 0x65B510F, 0xe990);
	Utils::Patch<uint64>(ImageBase + 0x65F6E0B, 0x3d80909090909090);
}
