#include "pch.h"
#include "Lategame.h"
#include "Utils.h"

FItemAndCount Lategame::GetShotguns()
{
    static UEAllocatedVector<FItemAndCount> Shotguns
    {
        FItemAndCount(1, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/FlipperGameplay/Items/Weapons/BurstShotgun/WID_Shotgun_CoreBurst_Athena_SR.WID_Shotgun_CoreBurst_Athena_SR")), // striker 
        FItemAndCount(1, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/FlipperGameplay/Items/Weapons/BurstShotgun/WID_Shotgun_CoreBurst_Athena_VR.WID_Shotgun_CoreBurst_Athena_VR")), // striker
        FItemAndCount(1, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_VR_Ore_T03.WID_Shotgun_Standard_Athena_VR_Ore_T03")), // pump 
        FItemAndCount(1, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_SR_Ore_T03.WID_Shotgun_Standard_Athena_SR_Ore_T03")), // pump 
    }; 

    return Shotguns[rand() % (Shotguns.size() - 1)];
}

FItemAndCount Lategame::GetAssaultRifles()
{
    static UEAllocatedVector<FItemAndCount> AssaultRifles
    {
        FItemAndCount(1, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/FlipperGameplay/Items/Weapons/CoreAR/WID_Assault_CoreAR_Athena_SR.WID_Assault_CoreAR_Athena_SR")), // ranger 
        FItemAndCount(1, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/FlipperGameplay/Items/Weapons/CoreAR/WID_Assault_CoreAR_Athena_VR.WID_Assault_CoreAR_Athena_VR")), // ranger
        FItemAndCount(1, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/FlipperGameplay/Items/Weapons/RedDotAR/WID_Assault_RedDotAR_Athena_SR.WID_Assault_RedDotAR_Athena_SR")), // mk seven
        FItemAndCount(1, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/FlipperGameplay/Items/Weapons/RedDotAR/WID_Assault_RedDotAR_Athena_VR.WID_Assault_RedDotAR_Athena_VR")), // mk seven
        FItemAndCount(1, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_SR_Ore_T03.WID_Assault_AutoHigh_Athena_SR_Ore_T03")), // scar 
        FItemAndCount(1, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_VR_Ore_T03.WID_Assault_AutoHigh_Athena_VR_Ore_T03")), // scar
    };

    return AssaultRifles[rand() % (AssaultRifles.size() - 1)];
}


FItemAndCount Lategame::GetSnipers()
{
    static UEAllocatedVector<FItemAndCount> Heals
    {
        FItemAndCount(1, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/FlipperGameplay/Items/Weapons/CoreSniper/WID_Sniper_CoreSniper_Athena_SR.WID_Sniper_CoreSniper_Athena_SR")), // hunter bolt 
        FItemAndCount(1, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/FlipperGameplay/Items/Weapons/CoreSMG/WID_SMG_CoreSMG_Athena_SR.WID_SMG_CoreSMG_Athena_SR")), // stinger
        //FItemAndCount(1, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/FlipperGameplay/Items/Weapons/CoreSMG/WID_SMG_CoreSMG_Athena_UR_IOBrute.WID_SMG_CoreSMG_Athena_UR_IOBrute")), // stinger
        FItemAndCount(6, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Consumables/ShockwaveGrenade/Athena_ShockGrenade.Athena_ShockGrenade")), // shockwave
        FItemAndCount(1, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/ParallelGameplay/Items/WestSausage/WID_WestSausage_Parallel_L_M.WID_WestSausage_Parallel_L_M")),
    };

    return Heals[rand() % (Heals.size() - 1)];
}

FItemAndCount Lategame::GetHeals()
{
    static UEAllocatedVector<FItemAndCount> Heals
    {
        FItemAndCount(3, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Consumables/Shields/Athena_Shields.Athena_Shields")), // big pots
        FItemAndCount(6, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Consumables/ShieldSmall/Athena_ShieldSmall.Athena_ShieldSmall")), // minis
        FItemAndCount(6, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Consumables/ChillBronco/Athena_ChillBronco.Athena_ChillBronco")), // chug
        FItemAndCount(2, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Consumables/PurpleStuff/Athena_PurpleStuff.Athena_PurpleStuff")), // slurp
        FItemAndCount(6, {}, Utils::FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Consumables/ShockwaveGrenade/Athena_ShockGrenade.Athena_ShockGrenade")), // shockwave
    };

    return Heals[rand() % (Heals.size() - 1)];
}

UFortAmmoItemDefinition* Lategame::GetAmmo(EAmmoType AmmoType)
{
    static UEAllocatedVector<UFortAmmoItemDefinition*> Ammos
    {
        Utils::FindObject<UFortAmmoItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsLight.AthenaAmmoDataBulletsLight"),
        Utils::FindObject<UFortAmmoItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataShells.AthenaAmmoDataShells"),
        Utils::FindObject<UFortAmmoItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsMedium.AthenaAmmoDataBulletsMedium"),
        Utils::FindObject<UFortAmmoItemDefinition>(L"/Game/Athena/Items/Ammo/AmmoDataRockets.AmmoDataRockets"),
        Utils::FindObject<UFortAmmoItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsHeavy.AthenaAmmoDataBulletsHeavy")
    };

    return Ammos[(uint8)AmmoType];
}

UFortResourceItemDefinition* Lategame::GetResource(EFortResourceType ResourceType)
{
    static UEAllocatedVector<UFortResourceItemDefinition*> Resources
    {
        Utils::FindObject<UFortResourceItemDefinition>(L"/Game/Items/ResourcePickups/WoodItemData.WoodItemData"),
        Utils::FindObject<UFortResourceItemDefinition>(L"/Game/Items/ResourcePickups/StoneItemData.StoneItemData"),
        Utils::FindObject<UFortResourceItemDefinition>(L"/Game/Items/ResourcePickups/MetalItemData.MetalItemData")
    };

    return Resources[(uint8)ResourceType];
}
