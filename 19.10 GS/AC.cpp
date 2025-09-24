#include "pch.h"
#include "AC.h"
#include "API.h"
#include "Options.h"

void AC::CheckUser(AFortPlayerControllerAthena* Controller)
{
    if (bDev) return;
    auto PlayerState = (AFortPlayerStateAthena*)Controller->PlayerState;
    auto name = PlayerState->GetPlayerName().ToString();
    std::thread([=]()
    {
            auto Response = API::GetResponse("https://flairfn.xyz/api/v1/checkUser/" + std::string(name));
            if (Response != "Valid") {
                uint8_t _NetRes[24];
                auto ConstructNetRes = (void (*)(void *, uint32)) (ImageBase + 0x1599788);
                void (*CloseNetConnection)(UNetConnection*, void*) = decltype(CloseNetConnection)(ImageBase + 0x1599018);
                ConstructNetRes(_NetRes, 4);
                CloseNetConnection(Controller->NetConnection, _NetRes);
            }
    }).detach();
}
