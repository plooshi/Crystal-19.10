// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include "Options.h"
#include "Utils.h"

DefHookOg(void, Test, UAthenaNavSystem*, UWorld*, char);

void Test(UAthenaNavSystem* NavSys, UWorld* World, char a3)
{
    for (auto& Agent : NavSys->SupportedAgents)
    {
        if (Agent.NavDataClass.Get())
        {
            Log(L"NavCl: %s", Agent.NavDataClass.Get()->Name.GetRawWString().c_str());
            Log(L"NavClPath: %s", Agent.NavDataClass.ObjectID.AssetPathName.GetRawWString().c_str());
        }
        else
        {
            Log(L"[null]");
        }
        if (Agent.NavigationDataClass.Get())
            Log(L"NavCl2: %s", Agent.NavigationDataClass.Get()->Name.GetRawWString().c_str());
    }
    Log(L"Enabled: %d", NavSys->SupportedAgentsMask.bSupportsAgent0);
    Log(L"Enabled: %d", NavSys->SupportedAgentsMask.bSupportsAgent1);
    Log(L"Enabled: %d", NavSys->SupportedAgentsMask.bSupportsAgent2);
    Log(L"Enabled: %d", NavSys->SupportedAgentsMask.bSupportsAgent3);
    Log(L"Enabled: %d", NavSys->SupportedAgentsMask.bSupportsAgent4);
    Log(L"Enabled: %d", NavSys->SupportedAgentsMask.bSupportsAgent5);
    Log(L"Enabled: %d", NavSys->SupportedAgentsMask.bSupportsAgent6);
    Log(L"Enabled: %d", NavSys->SupportedAgentsMask.bSupportsAgent7);
    while (true) {}
    return TestOG(NavSys, World, a3);
}
void Main() {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    SetConsoleTitleA("19.10: Setting up");
    LogCategory = FName(L"LogGameserver");
    auto FrontEndGameMode = (AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode;
    //while (FrontEndGameMode->MatchState != FName(L"InProgress"));
    Sleep(5000);

    MH_Initialize();
    //Utils::Hook(ImageBase + 0x3F00E08, Test, TestOG);
    for (auto& HookFunc : _HookFuncs) HookFunc();
    MH_EnableHook(MH_ALL_HOOKS);
    srand((uint32_t)time(0));

    *(bool*)(ImageBase + Sarah::Offsets::GIsClient) = false;
   
    //UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);
    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), bCreative ? L"open Creative_NoApollo_Terrain" : L"open Artemis_Terrain", nullptr);
}

BOOL APIENTRY DllMain(HMODULE Module, DWORD Reason, LPVOID Reserved)
{
    switch (Reason)
    {
    case 1: std::thread(Main).detach();
    case 0: break;
    }
    return 1;
}

