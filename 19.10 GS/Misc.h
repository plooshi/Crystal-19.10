#pragma once
#include "pch.h"
#include "Utils.h"

struct FServicePermissionsMcp{
public:
	char Unk_0[0x10];
	class FString Id;
};

class Misc {
private:
	DefHookOg(const wchar_t*, GetCommandLet);
	static int GetNetMode();
public:
	static inline bool PlayersToDestroyLocked = false;
	static inline std::vector<AFortPlayerPawn*> PlayersToDestroy;
	static inline int __Port;
	DefHookOg(float, GetMaxTickRate, UEngine*, float, bool);
	static bool Listen();
private:
	static FServicePermissionsMcp* MatchMakingServicePerms(int64, int64);
	static bool RetTrue();
	static bool RetFalse();
	static void ReturnNull();
	DefHookOg(void, TickFlush, UNetDriver*, float);
	DefHookOg(void*, DispatchRequest, void*, void*, int);
	DefHookOg(void*, ProcessEvent,UObject*, void*, void*);
	static void SetDynamicFoundationEnabled(UObject*, FFrame&);
	static void SetDynamicFoundationTransform(UObject*, FFrame&);
	static uint32 CheckCheckpointHeartBeat();
	DefHookOg(void, StartNewSafeZonePhase, AFortGameModeAthena*, int);
	DefHookOg(bool, StartAircraftPhase, AFortGameModeAthena*, char);
public:
	DefUHookOg(Athena_MedConsumable_Triggered);

	InitHooks;
};