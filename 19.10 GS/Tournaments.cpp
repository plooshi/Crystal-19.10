#include "pch.h"
#include "Tournaments.h"
#include "API.h"
#include "Options.h"
#include "Results.h"

void Tournaments::Kill(AFortPlayerControllerAthena* Controller)
{
	if (bDev) return;
	auto PlayerState = (AFortPlayerStateAthena*)Controller->PlayerState;

	if (PlayerState->IsPlayerDead() == true) {
		return;
	}

	auto PlayerName = PlayerState->GetPlayerName().ToString();
	
	Results::SendBattleRoyaleResult(PlayerName.c_str(), 0, 1, 0, StartingCount);
}

void Tournaments::Placement(int32 Placement, int32 Points)
{
	if (bDev) return;
	auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

	for (int i = 0; i < GameMode->AlivePlayers.Num(); i++) {
		GameMode->AlivePlayers[i]->ClientReportTournamentPlacementPointsScored(Placement, Points);
		auto PlayerState = (AFortPlayerStateAthena*)GameMode->AlivePlayers[i]->PlayerState;
		auto PlayerName = PlayerState->GetPlayerName().ToString();

		if (bTournament) {
			//API::SendGetRequest("https://flairfn.xyz/api/v1/crystal/tournament/" + std::string(PlayerName) + "/Top5");
		}
		else {
			Results::SendBattleRoyaleResult(PlayerName.c_str(), Placement, 0, 0, StartingCount);
		}
	}
}

void Tournaments::PlacementForController(AFortPlayerControllerAthena* Controller, int32 Placement)
{
	if (bDev) return;
	auto PlayerState = (AFortPlayerStateAthena*)Controller->PlayerState;
	auto PlayerName = PlayerState->GetPlayerName().ToString();
	Results::SendBattleRoyaleResult(PlayerName.c_str(), Placement, 0, 0, StartingCount);
}
