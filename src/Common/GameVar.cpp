/*
    Copyright 2012 bitHeads inc.

    This file is part of the BaboViolent 2 source code.

    The BaboViolent 2 source code is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    The BaboViolent 2 source code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with the
    BaboViolent 2 source code. If not, see http://www.gnu.org/licenses/.
*/

#include "GameVar.h"
#include "netPacket.h"
#include "RemoteAdminPackets.h"
#include "baboNet.h"
#include "sqlite3.h"

GameVar gameVar;

// function to fetch db infos, version and web url
void FetchDBInfos()
{
    sqlite3* db = 0;
    sqlite3_open("./bv2.db", &db);

    //some infos to load the data
    char    *zErrMsg;       // holds error msg if any
    char    **azResult;     // contains the actual returned data
    int nRow;           // number of record
    int nColumn;        // number of column
    char    SQL[256];       // the query

    sprintf(SQL, "Select Value From LauncherSettings Where Name = 'DBVersion';");
    sqlite3_get_table(db, SQL, &azResult, &nRow, &nColumn, &zErrMsg);

    gameVar.db_version = atoi(azResult[1]);
    sqlite3_free_table(azResult);

    sqlite3_close(db);
}

//
// Constructeur
//
GameVar::GameVar()
{
}

void GameVar::init()
{
    languageFile = "main/languages/en.lang";
    dksvarRegister(CString("languageFile [string : \"\"]"), &languageFile, true);

    weapons[WEAPON_DUAL_MACHINE_GUN] = new Weapon(.2f, "Dual Machine Gun",
        .2f, 5, 1, .5f, 2, WEAPON_DUAL_MACHINE_GUN, PROJECTILE_DIRECT);
    weapons[WEAPON_SMG] = new Weapon(.1f, "SMG",
        .1f, 8, 1, .5f, 1, WEAPON_SMG, PROJECTILE_DIRECT);
    weapons[WEAPON_CHAIN_GUN] = new Weapon(.05f, "ChainGun",
        .19f, 15, 1, 2.00f, 5, WEAPON_CHAIN_GUN, PROJECTILE_DIRECT);
    weapons[WEAPON_SHOTGUN] = new Weapon(0.85f, "Shotgun",
        .21f, 20, 5, 3.0f, 12, WEAPON_SHOTGUN, PROJECTILE_DIRECT);
    weapons[WEAPON_SNIPER] = new Weapon(2.0f, "Sniper Rifle",
        .30f, 0, 1, 3.0f, 0, WEAPON_SNIPER, PROJECTILE_DIRECT);
    weapons[WEAPON_BAZOOKA] = new Weapon(1.75f, "Bazooka",
        .75f, 0, 1, 3.0f, 0, WEAPON_BAZOOKA, PROJECTILE_ROCKET);
    weapons[WEAPON_GRENADE] = new Weapon(1.0f, "Grenade",
        1.5f, 0, 1, -1.0f, 0, WEAPON_GRENADE, PROJECTILE_GRENADE);
    weapons[WEAPON_COCKTAIL_MOLOTOV] = new Weapon(1.0f, "Flame",
        0.15f, 0, 1, -1.0f, 0, WEAPON_COCKTAIL_MOLOTOV, PROJECTILE_COCKTAIL_MOLOTOV);
    weapons[WEAPON_KNIVES] = new Weapon(1.0f, "Popup Knives",
        0.60f, 0, 1, 0, 0, WEAPON_KNIVES, PROJECTILE_NONE);
    weapons[WEAPON_PHOTON_RIFLE] = new Weapon(1.5f, "Photon Rifle",
        0.24f, 0, 1, 5.0f, 0, WEAPON_PHOTON_RIFLE, PROJECTILE_DIRECT);
    weapons[WEAPON_FLAME_THROWER] = new Weapon(.1f, "Flame Thrower",
        .08f, 6, 1, 0, 6, WEAPON_FLAME_THROWER, PROJECTILE_DIRECT);
    weapons[WEAPON_SHIELD] = new Weapon(3.0f, "Instant Shield",
        0, 0, 1, 0, 0, WEAPON_SHIELD, PROJECTILE_NONE);

    sv_timeToSpawn = 5;
    dksvarRegister(CString("sv_timeToSpawn [float : (default 5)]"), &sv_timeToSpawn, 0, 60,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_friendlyFire = false;
    dksvarRegister(CString("sv_friendlyFire [bool : (default false)]"), &sv_friendlyFire, true);
    sv_reflectedDamage = false;
    dksvarRegister(CString("sv_reflectedDamage [bool : (default false)]"), &sv_reflectedDamage, true);
    sv_topView = true;
    dksvarRegister(CString("sv_topView [bool : (default true)]"), &sv_topView, true);
    sv_minSendInterval = 2;
    dksvarRegister(CString("sv_minSendInterval [int : (default 2)]"), &sv_minSendInterval, 0, 5,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_forceRespawn = false;
    dksvarRegister(CString("sv_forceRespawn [bool : (default false)]"), &sv_forceRespawn, true);
    sv_baboStats = false;
    dksvarRegister(CString("sv_baboStats [bool : (default false)]"), &sv_baboStats, true);
    sv_roundTimeLimit = 3 * 60;
    dksvarRegister(CString("sv_roundTimeLimit [float : 0 = unlimited (default 180)]"), &sv_roundTimeLimit,
        0, 0, LIMIT_MIN, true);
    sv_gameTimeLimit = 30 * 60;
    dksvarRegister(CString("sv_gameTimeLimit [float : 0 = unlimited (default 1800)]"), &sv_gameTimeLimit,
        0, 0, LIMIT_MIN, true);
    sv_scoreLimit = 50;
    dksvarRegister(CString("sv_scoreLimit [int : 0 = unlimited (default 50)]"), &sv_scoreLimit,
        0, 0, LIMIT_MIN, true);
    sv_winLimit = 7;
    dksvarRegister(CString("sv_winLimit [int : 0 = unlimited (default 7)]"), &sv_winLimit,
        0, 0, LIMIT_MIN, true);

    sv_serverType = 0;
    dksvarRegister(CString("sv_serverType [int : 0=Normal, 1=Pro]"),
        &sv_serverType, 0, 1, LIMIT_MIN | LIMIT_MAX, true);

    sv_spawnType = 0;
    dksvarRegister(CString("sv_spawnType [int : 0=Normal, 1=Ladder]"),
        &sv_spawnType, 0, 1, LIMIT_MIN | LIMIT_MAX, true);

    sv_gameType = 1;
    dksvarRegister(CString("sv_gameType [int : 0=Deathmatch, 1=Team Deathmatch, 2=Capture The Flag]"),
        &sv_gameType, 0, 2, LIMIT_MIN | LIMIT_MAX, true);

    sv_subGameType = 0;
    dksvarRegister(CString("sv_subGameType [int : 0=Normal, 1=Instagib, 2=RandomWeapon]"),
        &sv_subGameType, 0, 2, LIMIT_MIN | LIMIT_MAX, true);

    sv_bombTime = 60;
    dksvarRegister(CString("sv_bombTime [int : (default 60)]"), &sv_bombTime, 10, 0, LIMIT_MIN, true);
    sv_gameName = "Babo Violent 2 - Server";
    dksvarRegister(CString("sv_gameName [string : \"\"]"), &sv_gameName, true);
    sv_port = 3333;
    dksvarRegister(CString("sv_port [int : valid port (default 3333)]"), &sv_port, 1024, 65536,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_maxPlayer = 16; // Max player in the game
    dksvarRegister(CString("sv_maxPlayer [int : 1 to 32 (default 16)]"), &sv_maxPlayer, 1, 32,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_maxPlayerInGame = 0; // Max player in the game
    dksvarRegister(CString("sv_maxPlayerInGame [int : 0 to 32 (default 0, no limit)]"), &sv_maxPlayerInGame, 0, 32,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_password = "";
    dksvarRegister(CString("sv_password [string : \"\""), &sv_password, true);
    sv_enableSMG = true;
    dksvarRegister(CString("sv_enableSMG [bool : true | false (default true)]"), &sv_enableSMG, true);
    sv_enableShotgun = true;
    dksvarRegister(CString("sv_enableShotgun [bool : true | false (default true)]"), &sv_enableShotgun, true);
    sv_enableSniper = true;
    dksvarRegister(CString("sv_enableSniper [bool : true | false (default true)]"), &sv_enableSniper, true);
    sv_enableDualMachineGun = true;
    dksvarRegister(CString("sv_enableDualMachineGun [bool : true | false (default true)]"), &sv_enableDualMachineGun, true);
    sv_enableChainGun = true;
    dksvarRegister(CString("sv_enableChainGun [bool : true | false (default true)]"), &sv_enableChainGun, true);
    sv_enableBazooka = true;
    dksvarRegister(CString("sv_enableBazooka [bool : true | false (default true)]"), &sv_enableBazooka, true);
    sv_enablePhotonRifle = true;
    dksvarRegister(CString("sv_enablePhotonRifle [bool : true | false (default true)]"), &sv_enablePhotonRifle, true);
    sv_enableFlameThrower = true;
    dksvarRegister(CString("sv_enableFlameThrower [bool : true | false (default true)]"), &sv_enableFlameThrower, true);
    sv_enableShotgunReload = true;
    dksvarRegister(CString("sv_enableShotgunReload [bool : true | false (default true)]"), &sv_enableShotgunReload, true);
    sv_slideOnIce = false;
    dksvarRegister(CString("sv_slideOnIce [bool : true | false (default false)]"), &sv_slideOnIce, true);
    sv_showEnemyTag = false;
    dksvarRegister(CString("sv_showEnemyTag [bool : true | false (default false)]"), &sv_showEnemyTag, true);
    zsv_adminUser = "";
    dksvarRegister(CString("zsv_adminUser [string : \"\""), &zsv_adminUser, true);
    zsv_adminPass = "";
    dksvarRegister(CString("zsv_adminPass [string : \"\""), &zsv_adminPass, true);
    sv_enableSecondary = true;
    dksvarRegister(CString("sv_enableSecondary [bool : true | false (default true)]"), &sv_enableSecondary, true);
    sv_enableKnives = true;
    dksvarRegister(CString("sv_enableKnives [bool : true | false (default true)]"), &sv_enableKnives, true);
    sv_enableNuclear = true;
    dksvarRegister(CString("sv_enableNuclear [bool : true | false (default true)]"), &sv_enableNuclear, true);
    sv_enableShield = true;
    dksvarRegister(CString("sv_enableShield [bool : true | false (default true)]"), &sv_enableShield, true);
    sv_autoBalance = true;
    dksvarRegister(CString("sv_autoBalance [bool : true | false (default true)]"), &sv_autoBalance, true);
    sv_autoBalanceTime = 4;
    dksvarRegister(CString("sv_autoBalanceTime [int : 1 to 15 (default 4)]"), &sv_autoBalanceTime,
        1, 15, LIMIT_MIN | LIMIT_MAX, true);
    sv_enableVote = true;
    dksvarRegister(CString("sv_enableVote [bool : true | false (default true)]"), &sv_enableVote, true);
    sv_gamePublic = true;
    dksvarRegister(CString("sv_gamePublic [bool : true | false (default true)]"), &sv_gamePublic, true);
    sv_maxUploadRate = 8.0f;
    dksvarRegister(CString("sv_maxUploadRate [float : 0 = unlimited (default 8.0)]"), &sv_maxUploadRate,
        0, 0, LIMIT_MIN, true);
    sv_showKills = false;
    dksvarRegister(CString("sv_showKills [bool : true | false (default false)]"), &sv_showKills, true);
    sv_matchcode = "";
    dksvarRegister(CString("sv_matchcode [string : \"\" (default \"\")]"), &sv_matchcode, true);
    sv_matchmode = 0;
    dksvarRegister(CString("sv_matchmode [int : 0 = unlimited (default 0)]"), &sv_matchmode, 0, 0, LIMIT_MIN, true);
    sv_report = false;
    dksvarRegister(CString("sv_report [bool : true | false (default false)]"), &sv_report, true);
    sv_maxPing = 1000;
    dksvarRegister(CString("sv_maxPing [int : 0 to 1000 (default 1000)]"), &sv_maxPing, 0, 1000,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_autoSpectateWhenIdle = true;
    dksvarRegister(CString("sv_autoSpectateWhenIdle [bool : true | false (default true)]"), &sv_autoSpectateWhenIdle, true);
    sv_autoSpectateIdleMaxTime = 180;
    dksvarRegister(CString("sv_autoSpectateIdleMaxTime [int : 60 to unlimited (default 180)]"), &sv_autoSpectateIdleMaxTime, 60, 0,
        LIMIT_MIN, true);
    sv_beGoodServer = true;
    dksvarRegister(CString("sv_beGoodServer [bool : true | false (default true)]"), &sv_beGoodServer, true);
    sv_validateWeapons = true;
    dksvarRegister(CString("sv_validateWeapons [bool : true | false (default true)]"), &sv_validateWeapons, true);
    sv_minTilesPerBabo = 55.0;
    dksvarRegister(CString("sv_minTilesPerBabo [float : 0.0 to 250.0 (default 55.0)]"), &sv_minTilesPerBabo, 0, 250,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_maxTilesPerBabo = 80.0;
    dksvarRegister(CString("sv_maxTilesPerBabo [float : 0.0 to 500.0 (default 80.0)]"), &sv_maxTilesPerBabo, 0, 500,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_zookaRemoteDet = true;
    dksvarRegister(CString("sv_zookaRemoteDet [bool : true | false (default true)]"), &sv_zookaRemoteDet, true);
    sv_sendJoinMessage = true;
    dksvarRegister(CString("sv_sendJoinMessage [bool : true | false (default true)]"), &sv_sendJoinMessage, true);
    sv_enableMolotov = true;
    dksvarRegister(CString("sv_enableMolotov [bool : true | false (default true)]"), &sv_enableMolotov, true);
    sv_joinMessage = "Welcome to the server!\0";
    dksvarRegister(CString("sv_joinMessage [string : \"\" ]"), &sv_joinMessage, true);
    sv_shottyDropRadius = 0.40f;
    dksvarRegister(CString("sv_shottyDropRadius [float : 0 to 2 (default 0.40)]"), &sv_shottyDropRadius, 0, 2,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_shottyRange = 6.75f;
    dksvarRegister(CString("sv_shottyRange [float : 1 to 24 (default 6.75)]"), &sv_shottyRange, 1, 24,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_ftMaxRange = 8.0f;
    dksvarRegister(CString("sv_ftMaxRange [float : 1 to 24  (default 8.0)]"), &sv_ftMaxRange, 1, 24,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_ftMinRange = 1.0f;
    dksvarRegister(CString("sv_ftMinRange [float : 0 to 24  (default 1.0)]"), &sv_ftMinRange, 0, 24,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_ftExpirationTimer = 1.5f;
    dksvarRegister(CString("sv_ftExpirationTimer [float : 0 to 30  (default 1.5)]"), &sv_ftExpirationTimer, 0, 30,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_photonDamageCoefficient = 0.5f;
    dksvarRegister(CString("sv_photonDamageCoefficient [float : -100 to 100 (default 0.5)]"), &sv_photonDamageCoefficient, -100, 100,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_photonVerticalShift = 0.325f;
    dksvarRegister(CString("sv_photonVerticalShift [float : -100 to 100 (default 0.325)]"), &sv_photonVerticalShift, -100, 100,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_photonDistMult = 0.25;
    dksvarRegister(CString("sv_photonDistMult [float : -100 to 100 (default 0.25)]"), &sv_photonDistMult, -100, 100,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_photonHorizontalShift = 5.0;
    dksvarRegister(CString("sv_photonHorizontalShift [float : -100 to 100 (default 5.00)]"), &sv_photonHorizontalShift, -100, 100,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_ftDamage = 0.12f;
    dksvarRegister(CString("sv_ftDamage [float : -100 to 100 (actual damage 100 times this, default 0.12)]"), &sv_ftDamage, -100, 100,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_smgDamage = 0.1f;
    dksvarRegister(CString("sv_smgDamage [float : -100 to 100 (actual damage 100 times this, default 0.10)]"), &sv_smgDamage, -100, 100,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_dmgDamage = 0.14f;
    dksvarRegister(CString("sv_dmgDamage [float : -100 to 100 (actual damage 100 times this, default 0.14)]"), &sv_dmgDamage, -100, 100,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_cgDamage = 0.16f;
    dksvarRegister(CString("sv_cgDamage [float : -100 to 100 (actual damage 100 times this, default 0.16)]"), &sv_cgDamage, -100, 100,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_sniperDamage = 0.2f;
    dksvarRegister(CString("sv_sniperDamage [float : -100 to 100 (actual damage 100 times this, default 0.20)]"), &sv_sniperDamage, -100, 100,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_shottyDamage = 0.21f;
    dksvarRegister(CString("sv_shottyDamage [float : -100 to 100 (actual damage 100 times this, default 0.21)]"), &sv_shottyDamage, -100, 100,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_zookaDamage = 0.85f;
    dksvarRegister(CString("sv_zookaDamage [float : -100 to 100 (actual damage 100 times this, default 0.85)]"), &sv_zookaDamage, -100, 100,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_zookaRadius = 2.0f;
    dksvarRegister(CString("sv_zookaRadius [float : 1 to 8 (default 2.0)]"), &sv_zookaRadius, 1, 8,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_nukeRadius = 6.0f;
    dksvarRegister(CString("sv_nukeRadius [float : 4 to 16 (default 6.0)]"), &sv_nukeRadius, 4, 16,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_nukeTimer = 3.0f;
    dksvarRegister(CString("sv_nukeTimer [float : 0 to 12 (default 3.0)]"), &sv_nukeTimer, 0, 12,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_nukeReload = 12.0f;
    dksvarRegister(CString("sv_nukeReload [float : 0 to 48 (default 12.0)]"), &sv_nukeReload, 0, 48,
        LIMIT_MIN | LIMIT_MAX, true);
    sv_photonType = 1;
    dksvarRegister(CString("sv_photonType [int : 0 to 3 (default 1)"), &sv_photonType, 0, 3,
        LIMIT_MIN | LIMIT_MAX, true);

    sv_spawnImmunityTime = 2.0f;
    dksvarRegister(CString("sv_spawnImmunityTime [float : 0 to 3 (default 2.0)]"), &sv_spawnImmunityTime, 0, 3, LIMIT_MIN | LIMIT_MAX, true);

    db_version = 0;
    FetchDBInfos();

    cl_playerName = "Unnamed Babo";
    dksvarRegister(CString("cl_playerName [string : \"\" (default \"Unnamed Babo\")]"), &cl_playerName, true);
    cl_mapAuthorName = "";
    dksvarRegister(CString("cl_mapAuthorName [string : \"\" (default \"\", max 24 characters)]"), &cl_mapAuthorName, true);
    cl_cubicMotion = true;
    dksvarRegister(CString("cl_cubicMotion [bool : true | false (default true)]"), &cl_cubicMotion, true);
    cl_lastUsedIP = "0.0.0.0";
    dksvarRegister(CString("cl_lastUsedIP [string : \"\"]"), &cl_lastUsedIP, true);
    cl_port = 3333;
    dksvarRegister(CString("cl_port [int : (default 3333)"), &cl_port, 1024, 65536,
        LIMIT_MIN | LIMIT_MAX, true);
    cl_password = "";
    dksvarRegister(CString("cl_password [string : \"\"]"), &cl_password, true);
    cl_redDecal.set(.5f, .5f, 1);
    dksvarRegister(CString("cl_redDecal [vector3f : (default .5 .5 1)]"), &cl_redDecal, true);
    cl_greenDecal.set(0, 0, 1);
    dksvarRegister(CString("cl_greenDecal [vector3f : (default 0 0 1)]"), &cl_greenDecal, true);
    cl_blueDecal.set(0, 0, .5f);
    dksvarRegister(CString("cl_blueDecal [vector3f : (default 0 0 .5)]"), &cl_blueDecal, true);
    cl_skin = "skin10";
    dksvarRegister(CString("cl_skin [string : \"skin10\"]"), &cl_skin, true);
    cl_teamIndicatorType = 0;
    dksvarRegister(CString("cl_teamIndicatorType [int : 0=Colourize, 1=Halos 2=TeamHalosOnly(default 0)]"),
        &cl_teamIndicatorType, 0, 2, LIMIT_MIN | LIMIT_MAX, true);
    cl_glowSize = 0.5f;
    dksvarRegister(CString("cl_glowSize [float : 0.25 to 0.5 (default 0.5)]"), &cl_glowSize, 0.25, 0.5, LIMIT_MIN | LIMIT_MAX, true);
    cl_preciseCursor = true;
    dksvarRegister(CString("cl_preciseCursor [bool : true | false (default true)]"), &cl_preciseCursor, true);
    cl_enableXBox360Controller = false;
    dksvarRegister(CString("cl_enableXBox360Controller [bool : true | false (default false)]"), &cl_enableXBox360Controller, true);
    cl_grassTextureForAllMaps = false;
    dksvarRegister(CString("cl_grassTextureForAllMaps [bool : true | false (default false)]"), &cl_grassTextureForAllMaps, true);
    cl_enableVSync = true;
    dksvarRegister(CString("cl_enableVSync [bool : true | false (default true)]"), &cl_enableVSync, true);
    cl_affinityMode = 0;
    dksvarRegister(CString("cl_affinityMode [int : 0=Default, 1=PartialBias, 2=FullBias]"),
        &cl_affinityMode, 0, 2, LIMIT_MIN | LIMIT_MAX, true);
    cl_primaryWeapon = 0;
    dksvarRegister(CString("cl_primaryWeapon [int : 0=SMG, 1=Shotgun, 2=Sniper, 3=DMG, 4=Chaingun, 5=Bazooka, 6=Photon, 7=Flamethrower]"),
        &cl_primaryWeapon, 0, 7, LIMIT_MIN | LIMIT_MAX, true);
    //cl_weaponSideRight = true;
    //dksvarRegister(CString("cl_weaponSideRight [bool : true | false (default true)]"),&cl_weaponSideRight,true);
    cl_secondaryWeapon = 0;
    dksvarRegister(CString("cl_secondaryWeapon [int : 0=Knives, 1=Shield]"),
        &cl_secondaryWeapon, 0, 2, LIMIT_MIN | LIMIT_MAX, true);

    // quick messages
    cl_qMsg01 = "aHello!";
    dksvarRegister(CString("cl_qMsg01 [string]"), &cl_qMsg01, true);
    cl_qMsg02 = "aHelp!";
    dksvarRegister(CString("cl_qMsg02 [string]"), &cl_qMsg02, true);
    cl_qMsg03 = "aRun!";
    dksvarRegister(CString("cl_qMsg03 [string]"), &cl_qMsg03, true);
    cl_qMsg04 = "aHaha!!!";
    dksvarRegister(CString("cl_qMsg04 [string]"), &cl_qMsg04, true);
    cl_qMsg05 = "aArgh...";
    dksvarRegister(CString("cl_qMsg05 [string]"), &cl_qMsg05, true);
    cl_qMsg06 = "tI\'m on defence";
    dksvarRegister(CString("cl_qMsg06 [string]"), &cl_qMsg06, true);
    cl_qMsg07 = "tI\'m on offence";
    dksvarRegister(CString("cl_qMsg07 [string]"), &cl_qMsg07, true);
    cl_qMsg08 = "tStay here!";
    dksvarRegister(CString("cl_qMsg08 [string]"), &cl_qMsg08, true);
    cl_qMsg09 = "tWait for me!";
    dksvarRegister(CString("cl_qMsg09 [string]"), &cl_qMsg09, true);
    cl_qMsg10 = "tGO! GO! GO!";
    dksvarRegister(CString("cl_qMsg10 [string]"), &cl_qMsg10, true);

    scl_honor = 10;
    scl_xp = 25;
    scl_leftToNextLevel = 75;
    scl_totalKill = 75;
    scl_totalDeath = 50;
    scl_ratio = 75.0f / 50.0f;
    scl_weaponOfChoice = 0;
    for(int i = 0; i < 20; scl_killWeapon[i++] = 0);

    r_showStats = false;
    dksvarRegister(CString("r_showStats [bool : true | false (default false)]"), &r_showStats, true);
#ifdef _DEBUG
    r_fullScreen = false;
#else
    r_fullScreen = true;
#endif
    dksvarRegister(CString("r_fullScreen [bool : true | false (default true)]"), &r_fullScreen, true);
    r_resolution.set(800, 600);
    dksvarRegister(CString("r_resolution [vector2i : (default 800 600)]"), &r_resolution, true);
    r_refreshRate = -1;
    dksvarRegister(CString("r_refreshRate [int : -1 | 60 | 70 | 72 | 75 | 85 (default -1)]"), &r_refreshRate, 60, 85,
        LIMIT_MIN | LIMIT_MAX, true);
    r_weatherEffects = false;
    dksvarRegister(CString("r_weatherEffects [bool : true | false (default false)]"), &r_weatherEffects, true);
    r_shadowQuality = 2;
    dksvarRegister(CString("r_shadowQuality [int : 0=none, 1=hard, 2=soft (default 2)]"), &r_shadowQuality,
        0, 2, LIMIT_MIN | LIMIT_MAX, true);
    r_playerShadow = true;
    dksvarRegister(CString("r_playerShadow [bool : true | false (default true)]"), &r_playerShadow, true);
    r_projectileShadow = true;
    dksvarRegister(CString("r_projectileShadow [bool : true | false (default true)]"), &r_projectileShadow, true);
    r_showCasing = true;
    dksvarRegister(CString("r_showCasing [bool : true | false (default true)]"), &r_showCasing, true);
    r_showGroundMark = true;
    dksvarRegister(CString("r_showGroundMark [bool : true | false (default true)]"), &r_showGroundMark, true);
    r_showLatency = false;
    dksvarRegister(CString("r_showLatency [bool : true | false (default false)]"), &r_showLatency, true);
    r_simpleText = false;
    dksvarRegister(CString("r_simpleText [bool : true | false (default false)]"), &r_simpleText, true);
    r_maxNameLenOverBabo = 0;
    dksvarRegister(CString("r_maxNameLenOverBabo [int : (default 0=no limit)]"), &r_maxNameLenOverBabo, 0, 0, LIMIT_MIN, true);

    r_chatTextSize = 28;
    dksvarRegister(CString("r_chatTextSize [int : (default 28)]"), &r_chatTextSize,
        8, 28, LIMIT_MIN | LIMIT_MAX, true);
    r_eventTextSize = 28;
    dksvarRegister(CString("r_eventTextSize [int : (default 28)]"), &r_eventTextSize,
        8, 28, LIMIT_MIN | LIMIT_MAX, true);
    r_showEventText = true;
    dksvarRegister(CString("r_showEventText [bool : true | false (default true)]"), &r_showEventText, true);

    s_mixRate = 22050;
    dksvarRegister(CString("s_mixRate [int : 22050 | 44100 (default 22050)]"), &s_mixRate, 22050, 44100,
        LIMIT_MIN | LIMIT_MAX, true);
    s_maxSoftwareChannels = 16;
    dksvarRegister(CString("s_maxSoftwareChannels [int : (default 16)]"), &s_maxSoftwareChannels,
        4, 64, LIMIT_MIN | LIMIT_MAX, true);
    s_masterVolume = 1;
    dksvarRegister(CString("s_masterVolume [float : 0-1 (default 1)]"), &s_masterVolume, 0, 100,
        LIMIT_MIN | LIMIT_MAX, true);
    s_inGameMusic = true;
    dksvarRegister(CString("s_inGameMusic [bool : true | false (default true)]"), &s_inGameMusic, true);

    c_debug = false; // Default
    dksvarRegister(CString("c_debug [bool : true | false (default false)]"), &c_debug, true);

    c_huge = false;
    dksvarRegister(CString("c_huge [bool : true | false (default false)]"), &c_huge, true);
    d_showPath = false;
    d_showNodes = false;
    dksvarRegister(CString("d_showPath [bool : true | false (default false)]"), &d_showPath, false);
    dksvarRegister(CString("d_showNodes [bool : true | false (default false)]"), &d_showNodes, false);
}



//
// Destructeur
//
GameVar::~GameVar()
{
    delete weapons[WEAPON_SMG];
    delete weapons[WEAPON_SHOTGUN];
    delete weapons[WEAPON_SNIPER];
    delete weapons[WEAPON_DUAL_MACHINE_GUN];
    delete weapons[WEAPON_CHAIN_GUN];
    delete weapons[WEAPON_BAZOOKA];
    delete weapons[WEAPON_PHOTON_RIFLE];
    delete weapons[WEAPON_GRENADE];
    delete weapons[WEAPON_FLAME_THROWER];
    delete weapons[WEAPON_COCKTAIL_MOLOTOV];
    delete weapons[WEAPON_KNIVES];
    delete weapons[WEAPON_SHIELD];
}



//
// Pour envoyer les variables servers au autres tayouin
//
void GameVar::sendSVVar(uint32_t babonetID)
{
    sendOne("sv_friendlyFire", babonetID);
    sendOne("sv_reflectedDamage", babonetID);
    sendOne("sv_timeToSpawn", babonetID);
    sendOne("sv_topView", babonetID);
    sendOne("sv_minSendInterval", babonetID);
    sendOne("sv_forceRespawn", babonetID);
    sendOne("sv_baboStats", babonetID);
    sendOne("sv_roundTimeLimit", babonetID);
    sendOne("sv_gameTimeLimit", babonetID);
    sendOne("sv_scoreLimit", babonetID);
    sendOne("sv_winLimit", babonetID);
    sendOne("sv_gameType", babonetID);
    sendOne("sv_serverType", babonetID);
    sendOne("sv_spawnType", babonetID);
    sendOne("sv_subGameType", babonetID);
    sendOne("sv_bombTime", babonetID);
    sendOne("sv_gameName", babonetID);
    sendOne("sv_port", babonetID);
    sendOne("sv_maxPlayer", babonetID);
    sendOne("sv_maxPlayerInGame", babonetID);
    sendOne("sv_password", babonetID);
    sendOne("sv_enableSMG", babonetID);
    sendOne("sv_enableShotgun", babonetID);
    sendOne("sv_enableSniper", babonetID);
    sendOne("sv_enableDualMachineGun", babonetID);
    sendOne("sv_enableChainGun", babonetID);
    sendOne("sv_enableBazooka", babonetID);
    sendOne("sv_enablePhotonRifle", babonetID);
    sendOne("sv_enableFlameThrower", babonetID);
    sendOne("sv_enableShotgunReload", babonetID);
    sendOne("sv_slideOnIce", babonetID);
    sendOne("sv_showEnemyTag", babonetID);
    sendOne("sv_enableSecondary", babonetID);
    sendOne("sv_enableKnives", babonetID);
    sendOne("sv_enableNuclear", babonetID);
    sendOne("sv_enableShield", babonetID);
    sendOne("sv_autoBalance", babonetID);
    sendOne("sv_autoBalanceTime", babonetID);
    sendOne("sv_gamePublic", babonetID);
    sendOne("sv_matchcode", babonetID);
    sendOne("sv_matchmode", babonetID);
    sendOne("sv_maxPing", babonetID);
    sendOne("sv_shottyDropRadius", babonetID);
    sendOne("sv_shottyRange", babonetID);
    sendOne("sv_enableMolotov", babonetID);
    sendOne("sv_ftMaxRange", babonetID);
    sendOne("sv_ftMinRange", babonetID);
    sendOne("sv_photonDamageCoefficient", babonetID);
    sendOne("sv_zookaRemoteDet", babonetID);
    sendOne("sv_smgDamage", babonetID);
    sendOne("sv_ftDamage", babonetID);
    sendOne("sv_dmgDamage", babonetID);
    sendOne("sv_cgDamage", babonetID);
    sendOne("sv_shottyDamage", babonetID);
    sendOne("sv_sniperDamage", babonetID);
    sendOne("sv_zookaDamage", babonetID);
    sendOne("sv_photonType", babonetID);
    sendOne("sv_zookaRadius", babonetID);
    sendOne("sv_nukeRadius", babonetID);
    sendOne("sv_nukeTimer", babonetID);
    sendOne("sv_nukeReload", babonetID);
    sendOne("sv_minTilesPerBabo", babonetID);
    sendOne("sv_maxTilesPerBabo", babonetID);
    sendOne("sv_photonDistMult", babonetID);
    sendOne("sv_photonVerticalShift", babonetID);
    sendOne("sv_photonHorizontalShift", babonetID);
    sendOne("sv_joinMessage", babonetID);
    sendOne("sv_sendJoinMessage", babonetID);
    sendOne("sv_ftExpirationTimer", babonetID);
    sendOne("sv_enableVote", babonetID);
}

void GameVar::sendSVVar(int32_t peerId)
{
    sendOne("sv_friendlyFire", peerId);
    sendOne("sv_reflectedDamage", peerId);
    sendOne("sv_timeToSpawn", peerId);
    sendOne("sv_topView", peerId);
    sendOne("sv_minSendInterval", peerId);
    sendOne("sv_forceRespawn", peerId);
    sendOne("sv_baboStats", peerId);
    sendOne("sv_roundTimeLimit", peerId);
    sendOne("sv_gameTimeLimit", peerId);
    sendOne("sv_scoreLimit", peerId);
    sendOne("sv_winLimit", peerId);
    sendOne("sv_gameType", peerId);
    sendOne("sv_serverType", peerId);
    sendOne("sv_spawnType", peerId);
    sendOne("sv_subGameType", peerId);
    sendOne("sv_bombTime", peerId);
    sendOne("sv_gameName", peerId);
    sendOne("sv_port", peerId);
    sendOne("sv_maxPlayer", peerId);
    sendOne("sv_maxPlayerInGame", peerId);
    sendOne("sv_password", peerId);
    sendOne("sv_enableSMG", peerId);
    sendOne("sv_enableShotgun", peerId);
    sendOne("sv_enableSniper", peerId);
    sendOne("sv_enableDualMachineGun", peerId);
    sendOne("sv_enableChainGun", peerId);
    sendOne("sv_enableBazooka", peerId);
    sendOne("sv_enablePhotonRifle", peerId);
    sendOne("sv_enableFlameThrower", peerId);
    sendOne("sv_enableShotgunReload", peerId);
    sendOne("sv_slideOnIce", peerId);
    sendOne("sv_showEnemyTag", peerId);
    sendOne("sv_enableSecondary", peerId);
    sendOne("sv_enableKnives", peerId);
    sendOne("sv_enableNuclear", peerId);
    sendOne("sv_enableShield", peerId);
    sendOne("sv_showKills", peerId);
    sendOne("sv_beGoodServer", peerId);
    sendOne("sv_validateWeapons", peerId);
    sendOne("sv_autoBalance", peerId);
    sendOne("sv_autoBalanceTime", peerId);
    sendOne("sv_gamePublic", peerId);
    sendOne("sv_matchcode", peerId);
    sendOne("sv_matchmode", peerId);
    sendOne("sv_maxPing", peerId);
    sendOne("sv_shottyDropRadius", peerId);
    sendOne("sv_shottyRange", peerId);
    sendOne("sv_enableMolotov", peerId);
    sendOne("sv_ftMaxRange", peerId);
    sendOne("sv_ftMinRange", peerId);
    sendOne("sv_photonDamageCoefficient", peerId);
    sendOne("sv_zookaRemoteDet", peerId);
    sendOne("sv_smgDamage", peerId);
    sendOne("sv_ftDamage", peerId);
    sendOne("sv_dmgDamage", peerId);
    sendOne("sv_cgDamage", peerId);
    sendOne("sv_shottyDamage", peerId);
    sendOne("sv_sniperDamage", peerId);
    sendOne("sv_zookaDamage", peerId);
    sendOne("sv_photonType", peerId);
    sendOne("sv_zookaRadius", peerId);
    sendOne("sv_nukeRadius", peerId);
    sendOne("sv_nukeTimer", peerId);
    sendOne("sv_nukeReload", peerId);
    sendOne("sv_minTilesPerBabo", peerId);
    sendOne("sv_maxTilesPerBabo", peerId);
    sendOne("sv_photonDistMult", peerId);
    sendOne("sv_photonVerticalShift", peerId);
    sendOne("sv_photonHorizontalShift", peerId);
    sendOne("sv_joinMessage", peerId);
    sendOne("sv_sendJoinMessage", peerId);
    sendOne("sv_ftExpirationTimer", peerId);
    sendOne("sv_enableVote", peerId);
}

void GameVar::sendOne(char * varName, int32_t peerId)
{
    CString varCom;
    dksvarGetFormatedVar(varName, &varCom);
    //varCom.insert("set ", 0);

    if(varCom.len() > 79) varCom.resize(79);
    net_ra_var ra_var;
    memcpy(ra_var.var, varCom.s, varCom.len() + 1);
    bb_peerSend(peerId, (char*)&ra_var, RA_VAR, sizeof(net_ra_var), true);
}

void GameVar::sendOne(char * varName, uint32_t babonetID)
{
    CString varCom;
    dksvarGetFormatedVar(varName, &varCom);
    varCom.insert("set ", 0);
    if(varCom.len() > 79) varCom.resize(79);
    net_svcl_sv_change svChange;
    memcpy(svChange.svChange, varCom.s, varCom.len() + 1);
    bb_serverSend((char*)&svChange, sizeof(net_svcl_sv_change), NET_SVCL_SV_CHANGE, babonetID);
}
