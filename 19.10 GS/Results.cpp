#include "pch.h"
#include "Results.h"
#include "API.h"

void Results::SendBattleRoyaleResult(const std::string& username,
    int placement,
    int elims,
    int xp,
    int StartingCount)
{
    const std::string url = "https://flairfn.xyz/fortnite/api/game/v2/profile/" + username + "/dedicated_server/EndBattleRoyaleGameV2?profileId=athena";

    int placementPoints = placement * 15;
    const int pointsGained = (elims * 20) + placementPoints;
    
    const std::string postData =
        "{"
        "\"Placement\":" + std::to_string(placement) + "," +
        "\"Eliminations\":" + std::to_string(elims) + "," +
        "\"XP\":" + std::to_string(xp) + "," +
        "\"Playlist\":\"Playlist_ShowdownAlt_Solo\"," +
        "\"IsEvent\":true,"
        "\"TournamentInfo\":{"
        "\"TournamentType\":\"NormalHype\","
        "\"TournamentId\":\"ArenaSolo\","
        "\"PointsGained\":" + std::to_string(pointsGained) + "," +
        "\"StartingPlayerCount\":" + std::to_string(StartingCount) +
        "}}";


    std::thread([url, postData]
        {
            curl_global_init(CURL_GLOBAL_ALL);
            if (auto* curl = curl_easy_init())
            {
                std::string response;
                struct curl_slist* hdrs = nullptr;
                hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
                hdrs = curl_slist_append(hdrs, "X-Api-Key: a9f3e5c2-38b4-4d0f-9a7e-84ec4bdb9b35");

                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, API::CallBack);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

                curl_easy_perform(curl);

                curl_slist_free_all(hdrs);
                curl_easy_cleanup(curl);
            }
            curl_global_cleanup();
        }).detach();
}

void Results::SendEventMatchResults(const std::string& username, const std::string& eventId, const std::vector<std::string>& teammateIds, const std::vector<std::string>& matchHistory, const std::string& sessionId, int placement, int elims, int timeAlive)
{
    const std::string url = "https://flairfn.xyz/fortnite/api/game/v2/profile/" + username + "/dedicated_server/SendEventMatchResults?profileId=athena";

    std::string teammateIdsJson = "[";
    for (size_t i = 0; i < teammateIds.size(); ++i)
    {
        teammateIdsJson += "\"" + teammateIds[i] + "\"";
        if (i < teammateIds.size() - 1) teammateIdsJson += ",";
    }
    teammateIdsJson += "]";

    std::string matchHistoryJson = "[";
    for (size_t i = 0; i < matchHistory.size(); ++i)
    {
        matchHistoryJson += "\"" + matchHistory[i] + "\"";
        if (i < matchHistory.size() - 1) matchHistoryJson += ",";
    }
    matchHistoryJson += "]";

    
    int placementPoints = placement * 15;
    const int pointsGained = (elims * 20) + placementPoints;
    

    const std::string postData =
        "{"
        "\"eventId\":\"" + eventId + "\","
        "\"matchHistory\":" + matchHistoryJson + ","
        "\"teammateIds\":" + teammateIdsJson + ","
        "\"eliminations\":" + std::to_string(elims) + ","
        "\"placement\":" + std::to_string(placement) + ","
        "\"pointsEarned\":" + std::to_string(pointsGained) + ","
        "\"sessionId\":\"" + sessionId + "\","
        "\"timeAlive\":" + std::to_string(timeAlive) +
        "}";

    std::thread([url, postData]
    {
        curl_global_init(CURL_GLOBAL_ALL);
        if (auto* curl = curl_easy_init())
        {
            std::string response;
            struct curl_slist* hdrs = nullptr;
            hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
            hdrs = curl_slist_append(hdrs, "X-Api-Key: a9f3e5c2-38b4-4d0f-9a7e-84ec4bdb9b35");

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, API::CallBack);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

            curl_easy_perform(curl);

            curl_slist_free_all(hdrs);
            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();
    }).detach();
}
