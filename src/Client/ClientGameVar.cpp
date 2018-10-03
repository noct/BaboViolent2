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

#include "ClientGameVar.h"
#include "KeyManager.h"

ClientGameVar clientVar;

ClientGameVar::ClientGameVar()
{
}

ClientGameVar::~ClientGameVar()
{
    deleteModels();
    langs.clear();
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


void ClientGameVar::init()
{
    languageLoaded = false;
    // LES STRING DIFF�RENT POUR CHAQUE LANGUE
    langs.push_back(SLangText("lang_gameName", &lang_gameName));

    langs.push_back(SLangText("lang_dualMachineGun", &lang_dualMachineGun));
    langs.push_back(SLangText("lang_subMachineGun", &lang_subMachineGun));
    langs.push_back(SLangText("lang_changGun", &lang_changGun));
    langs.push_back(SLangText("lang_shotgun", &lang_shotgun));
    langs.push_back(SLangText("lang_sniper", &lang_sniper));
    langs.push_back(SLangText("lang_bazooka", &lang_bazooka));
    langs.push_back(SLangText("lang_grenade", &lang_grenade));
    langs.push_back(SLangText("lang_flame", &lang_flame));

    langs.push_back(SLangText("lang_autoAssignTeam", &lang_autoAssignTeam));
    langs.push_back(SLangText("lang_joinBlueTeam", &lang_joinBlueTeam));
    langs.push_back(SLangText("lang_joinRedTeam", &lang_joinRedTeam));
    langs.push_back(SLangText("lang_spectator", &lang_spectator));
    langs.push_back(SLangText("lang_mainMenu", &lang_mainMenu));
    langs.push_back(SLangText("lang_disconnect", &lang_disconnect));

    langs.push_back(SLangText("lang_serverDisconnected", &lang_serverDisconnected));

    langs.push_back(SLangText("lang_dead", &lang_dead));
    langs.push_back(SLangText("lang_team", &lang_team));

    langs.push_back(SLangText("lang_joinedTheGame", &lang_joinedTheGame));
    langs.push_back(SLangText("lang_playerDisconnected", &lang_playerDisconnected));
    langs.push_back(SLangText("lang_player", &lang_player));
    langs.push_back(SLangText("lang_playerChangedHisNameFor", &lang_playerChangedHisNameFor));
    langs.push_back(SLangText("lang_returnBlueFlag", &lang_returnBlueFlag));
    langs.push_back(SLangText("lang_returnRedFlag", &lang_returnRedFlag));
    langs.push_back(SLangText("lang_tookBlueFlag", &lang_tookBlueFlag));
    langs.push_back(SLangText("lang_tookRedFlag", &lang_tookRedFlag));
    langs.push_back(SLangText("lang_redScore", &lang_redScore));
    langs.push_back(SLangText("lang_blueScore", &lang_blueScore));

    langs.push_back(SLangText("lang_reloading", &lang_reloading));

    langs.push_back(SLangText("lang_spawnIn", &lang_spawnIn));
    langs.push_back(SLangText("lang_pressToRespawn", &lang_pressToRespawn));

    langs.push_back(SLangText("lang_blueTeamWin", &lang_blueTeamWin));
    langs.push_back(SLangText("lang_redTeamWin", &lang_redTeamWin));
    langs.push_back(SLangText("lang_roundDraw", &lang_roundDraw));
    langs.push_back(SLangText("lang_changingMap", &lang_changingMap));

    langs.push_back(SLangText("lang_deathmatchC", &lang_deathmatchC));
    langs.push_back(SLangText("lang_deathmatchD", &lang_deathmatchD));
    langs.push_back(SLangText("lang_teamDeathmatchC", &lang_teamDeathmatchC));
    langs.push_back(SLangText("lang_teamDeathmatchD", &lang_teamDeathmatchD));
    langs.push_back(SLangText("lang_captureTheFlagC", &lang_captureTheFlagC));
    langs.push_back(SLangText("lang_captureTheFlagD", &lang_captureTheFlagD));
    langs.push_back(SLangText("lang_counterBaboristC", &lang_counterBaboristC));
    langs.push_back(SLangText("lang_counterBaboristD", &lang_counterBaboristD));

    langs.push_back(SLangText("lang_connectingC", &lang_connectingC));
    langs.push_back(SLangText("lang_pressF10ToCancel", &lang_pressF10ToCancel));

    langs.push_back(SLangText("lang_goesSpectator", &lang_goesSpectator));
    langs.push_back(SLangText("lang_joinBlueTeamP", &lang_joinBlueTeamP));
    langs.push_back(SLangText("lang_joinRedTeamP", &lang_joinRedTeamP));

    langs.push_back(SLangText("lang_blueTeamC", &lang_blueTeamC));
    langs.push_back(SLangText("lang_redTeamC", &lang_redTeamC));
    langs.push_back(SLangText("lang_freeForAllC", &lang_freeForAllC));
    langs.push_back(SLangText("lang_spectatorC", &lang_spectatorC));

    langs.push_back(SLangText("lang_playerNameC", &lang_playerNameC));
    langs.push_back(SLangText("lang_scoreC", &lang_scoreC));
    langs.push_back(SLangText("lang_pingC", &lang_pingC));

    langs.push_back(SLangText("lang_laggerC", &lang_laggerC));

    langs.push_back(SLangText("lang_friendlyFire", &lang_friendlyFire));

    langs.push_back(SLangText("lang_serverVersion", &lang_serverVersion));
    langs.push_back(SLangText("lang_clientVersion", &lang_clientVersion));

    langs.push_back(SLangText("lang_connectionFailed", &lang_connectionFailed));
    langs.push_back(SLangText("lang_back", &lang_back));

    langs.push_back(SLangText("lang_launch", &lang_launch));
    langs.push_back(SLangText("lang_dedicate", &lang_dedicate));

    langs.push_back(SLangText("lang_generalGameOption", &lang_generalGameOption));
    langs.push_back(SLangText("lang_gameNameS", &lang_gameNameS));
    langs.push_back(SLangText("lang_gameType", &lang_gameType));

    langs.push_back(SLangText("lang_serverType", &lang_serverType));
    langs.push_back(SLangText("lang_spawnType", &lang_spawnType));
    langs.push_back(SLangText("lang_subGameType", &lang_subGameType));

    langs.push_back(SLangText("lang_freeForAll", &lang_freeForAll));
    langs.push_back(SLangText("lang_teamDeathmatch", &lang_teamDeathmatch));
    langs.push_back(SLangText("lang_captureTheFlag", &lang_captureTheFlag));
    langs.push_back(SLangText("lang_TCPnUDPport", &lang_TCPnUDPport));
    langs.push_back(SLangText("lang_gameLimits", &lang_gameLimits));
    langs.push_back(SLangText("lang_scoreLimit", &lang_scoreLimit));
    langs.push_back(SLangText("lang_teamWinLimit", &lang_teamWinLimit));
    langs.push_back(SLangText("lang_timeLimit", &lang_timeLimit));
    langs.push_back(SLangText("lang_maxPlayer", &lang_maxPlayer));
    langs.push_back(SLangText("lang_spawning", &lang_spawning));
    langs.push_back(SLangText("lang_timeToSpawn", &lang_timeToSpawn));
    langs.push_back(SLangText("lang_forceRespawn", &lang_forceRespawn));
    langs.push_back(SLangText("lang_no", &lang_no));
    langs.push_back(SLangText("lang_yes", &lang_yes));
    langs.push_back(SLangText("lang_reflectDamage", &lang_reflectDamage));
    langs.push_back(SLangText("lang_maps", &lang_maps));
    langs.push_back(SLangText("lang_includedMaps", &lang_includedMaps));

    langs.push_back(SLangText("lang_RndLabsTeam", &lang_RndLabsTeam));
    langs.push_back(SLangText("lang_gameDesigner", &lang_gameDesigner));
    langs.push_back(SLangText("lang_programmers", &lang_programmers));
    langs.push_back(SLangText("lang_soundDesigners", &lang_soundDesigners));
    langs.push_back(SLangText("lang_musics", &lang_musics));
    langs.push_back(SLangText("lang_thanksTo", &lang_thanksTo));
    langs.push_back(SLangText("lang_betaTesters", &lang_betaTesters));
    langs.push_back(SLangText("lang_buyAlbum", &lang_buyAlbum));

    langs.push_back(SLangText("lang_internet", &lang_internet));
    langs.push_back(SLangText("lang_localNetwork", &lang_localNetwork));
    langs.push_back(SLangText("lang_refreshList", &lang_refreshList));
    langs.push_back(SLangText("lang_players", &lang_players));
    langs.push_back(SLangText("lang_mapName", &lang_mapName));
    //  langs.push_back(SLangText("lang_ping", &lang_ping));
    langs.push_back(SLangText("lang_join", &lang_join));
    langs.push_back(SLangText("lang_IPAdress", &lang_IPAdress));
    langs.push_back(SLangText("lang_canRefreshIn", &lang_canRefreshIn));
    langs.push_back(SLangText("lang_nbGamesRefreshed", &lang_nbGamesRefreshed));
    langs.push_back(SLangText("lang_noGameFound", &lang_noGameFound));
    langs.push_back(SLangText("lang_problemConnectingToMaster", &lang_problemConnectingToMaster));

    langs.push_back(SLangText("lang_resumeCurrentGame", &lang_resumeCurrentGame));
    langs.push_back(SLangText("lang_joinGame", &lang_joinGame));
    langs.push_back(SLangText("lang_createServer", &lang_createServer));
    langs.push_back(SLangText("lang_options", &lang_options));
    langs.push_back(SLangText("lang_credits", &lang_credits));
    langs.push_back(SLangText("lang_quit", &lang_quit));

    langs.push_back(SLangText("lang_noGameRunning", &lang_noGameRunning));

    langs.push_back(SLangText("lang_selectAtLeastOneMap", &lang_selectAtLeastOneMap));

    langs.push_back(SLangText("lang_apply", &lang_apply));
    langs.push_back(SLangText("lang_cancel", &lang_cancel));
    langs.push_back(SLangText("lang_someOptionRequireRestart", &lang_someOptionRequireRestart));
    langs.push_back(SLangText("lang_clientOptions", &lang_clientOptions));
    langs.push_back(SLangText("lang_playerName", &lang_playerName));
    langs.push_back(SLangText("lang_inGameMusic", &lang_inGameMusic));
    langs.push_back(SLangText("lang_renderingOptions", &lang_renderingOptions));
    langs.push_back(SLangText("lang_showStats", &lang_showStats));
    langs.push_back(SLangText("lang_fullscreen", &lang_fullscreen));
    langs.push_back(SLangText("lang_screenResolution", &lang_screenResolution));
    langs.push_back(SLangText("lang_colorDepth", &lang_colorDepth));
    langs.push_back(SLangText("lang_bits", &lang_bits));
    langs.push_back(SLangText("lang_wallShadowQuality", &lang_wallShadowQuality));
    langs.push_back(SLangText("lang_noShadows", &lang_noShadows));
    langs.push_back(SLangText("lang_hardEdges", &lang_hardEdges));
    langs.push_back(SLangText("lang_softEdges", &lang_softEdges));
    langs.push_back(SLangText("lang_baboShadow", &lang_baboShadow));
    langs.push_back(SLangText("lang_projectileShadow", &lang_projectileShadow));
    langs.push_back(SLangText("lang_particleLighting", &lang_particleLighting));
    langs.push_back(SLangText("lang_particleSorting", &lang_particleSorting));
    langs.push_back(SLangText("lang_terrainSplatter", &lang_terrainSplatter));
    langs.push_back(SLangText("lang_showCasing", &lang_showCasing));
    langs.push_back(SLangText("lang_groundMarkNBlood", &lang_groundMarkNBlood));
    langs.push_back(SLangText("lang_soundOptions", &lang_soundOptions));
    langs.push_back(SLangText("lang_mixRate", &lang_mixRate));
    langs.push_back(SLangText("lang_hertz", &lang_hertz));
    langs.push_back(SLangText("lang_maxSoftwareChannels", &lang_maxSoftwareChannels));
    langs.push_back(SLangText("lang_masterVolume", &lang_masterVolume));
    langs.push_back(SLangText("lang_keyOptions", &lang_keyOptions));
    langs.push_back(SLangText("lang_moveUp", &lang_moveUp));
    langs.push_back(SLangText("lang_moveDown", &lang_moveDown));
    langs.push_back(SLangText("lang_moveRight", &lang_moveRight));
    langs.push_back(SLangText("lang_moveLeft", &lang_moveLeft));
    langs.push_back(SLangText("lang_shoot", &lang_shoot));
    langs.push_back(SLangText("lang_throwGrenade", &lang_throwGrenade));
    langs.push_back(SLangText("lang_throwCocktailMolotov", &lang_throwCocktailMolotov));
    langs.push_back(SLangText("lang_use", &lang_use));
    langs.push_back(SLangText("lang_chatToAll", &lang_chatToAll));
    langs.push_back(SLangText("lang_chatToTeam", &lang_chatToTeam));
    langs.push_back(SLangText("lang_showScoreBoard", &lang_showScoreBoard));
    langs.push_back(SLangText("lang_toggleMenu", &lang_toggleMenu));

    langs.push_back(SLangText("lang_areYouSureYouWantToQuit", &lang_areYouSureYouWantToQuit));
    langs.push_back(SLangText("lang_yesQ", &lang_yesQ));
    langs.push_back(SLangText("lang_noQ", &lang_noQ));

    langs.push_back(SLangText("lang_serverHasClosedTheConnection", &lang_serverHasClosedTheConnection));

    langs.push_back(SLangText("lang_language", &lang_language));
    langs.push_back(SLangText("lang_incorrectName", &lang_incorrectName));

    loadLanguage(gameVar.languageFile.s);
    weapons[WEAPON_DUAL_MACHINE_GUN] = new ClientWeapon("main/models/DualMachineGun.DKO", "main/sounds/DualMachineGun.wav", .1f, lang_dualMachineGun.s,
        .13f, 10, 1, .8f, 2, WEAPON_DUAL_MACHINE_GUN, PROJECTILE_DIRECT);
    weapons[WEAPON_SMG] = new ClientWeapon("main/models/SMG.DKO", "main/sounds/SMG.wav", .1f, lang_subMachineGun.s,
        .1f, 8, 1, .5f, 1, WEAPON_SMG, PROJECTILE_DIRECT);
    weapons[WEAPON_CHAIN_GUN] = new ClientWeapon("main/models/ChainGun.DKO", "main/sounds/ChainGun.wav", .1f, lang_changGun.s,
        .19f, 15, 1, 2.00f, 5, WEAPON_CHAIN_GUN, PROJECTILE_DIRECT);
    weapons[WEAPON_SHOTGUN] = new ClientWeapon("main/models/ShotGun.DKO", "main/sounds/Shotgun.wav", 0.85f, lang_shotgun.s,
        .21f, 20, 5, 3.0f, 12, WEAPON_SHOTGUN, PROJECTILE_DIRECT);
    weapons[WEAPON_SNIPER] = new ClientWeapon("main/models/Sniper.DKO", "main/sounds/Sniper.wav", 2.0f, lang_sniper.s,
        .30f, 0, 1, 3.0f, 0, WEAPON_SNIPER, PROJECTILE_DIRECT);
    weapons[WEAPON_BAZOOKA] = new ClientWeapon("main/models/Bazooka.DKO", "main/sounds/Bazooka.wav", 1.75f, lang_bazooka.s,
        .75f, 0, 1, 3.0f, 0, WEAPON_BAZOOKA, PROJECTILE_ROCKET);
    weapons[WEAPON_GRENADE] = new ClientWeapon("main/models/Hand.DKO", "main/sounds/Grenade.wav", 1.0f, lang_grenade.s,
        1.5f, 0, 1, -1.0f, 0, WEAPON_GRENADE, PROJECTILE_GRENADE);
    weapons[WEAPON_COCKTAIL_MOLOTOV] = new ClientWeapon("main/models/Hand.DKO", "main/sounds/Grenade.wav", 1.0f, "Flame",
        0.15f, 0, 1, -1.0f, 0, WEAPON_COCKTAIL_MOLOTOV, PROJECTILE_COCKTAIL_MOLOTOV);
    weapons[WEAPON_KNIVES] = new ClientWeapon("main/models/Knifes.DKO", "main/sounds/knifes.wav", 1.0f, "Popup Knives",
        0.60f, 0, 1, 0, 0, WEAPON_KNIVES, PROJECTILE_NONE);
    weapons[WEAPON_PHOTON_RIFLE] = new ClientWeapon("main/models/PhotonRifle.DKO", "main/sounds/PhotonRifle.wav", 1.5f, "Photon Rifle",
        0.24f, 0, 1, 5.0f, 0, WEAPON_PHOTON_RIFLE, PROJECTILE_DIRECT);
    weapons[WEAPON_FLAME_THROWER] = new ClientWeapon("main/models/FlameThrower.DKO", "main/sounds/FlameThrower.wav", .1f, "Flame Thrower",
        .08f, 10, 1, 0, 10, WEAPON_FLAME_THROWER, PROJECTILE_DIRECT);
    weapons[WEAPON_SHIELD] = new ClientWeapon("main/models/Shield.DKO", "main/sounds/shield.wav", 3.0f, "Instant Shield",
        0, 0, 1, 0, 0, WEAPON_SHIELD, PROJECTILE_NONE);


    k_moveUp = keyManager.getKeyByName("W");
    dksvarRegister(CString("k_moveUp [int]"), &k_moveUp, 0, 0, 0, true);
    k_moveDown = keyManager.getKeyByName("S");
    dksvarRegister(CString("k_moveDown [int]"), &k_moveDown, 0, 0, 0, true);
    k_moveRight = keyManager.getKeyByName("D");
    dksvarRegister(CString("k_moveRight [int]"), &k_moveRight, 0, 0, 0, true);
    k_moveLeft = keyManager.getKeyByName("A");
    dksvarRegister(CString("k_moveLeft [int]"), &k_moveLeft, 0, 0, 0, true);
    k_shoot = keyManager.getKeyByName("Mouse1");
    dksvarRegister(CString("k_shoot [int]"), &k_shoot, 0, 0, 0, true);
    k_throwGrenade = keyManager.getKeyByName("Mouse2");
    dksvarRegister(CString("k_throwGrenade [int]"), &k_throwGrenade, 0, 0, 0, true);
    k_throwMolotov = keyManager.getKeyByName("Mouse3");
    dksvarRegister(CString("k_throwMolotov [int]"), &k_throwMolotov, 0, 0, 0, true);
    k_pickUp = keyManager.getKeyByName("F");
    dksvarRegister(CString("k_pickUp [int]"), &k_pickUp, 0, 0, 0, true);
    k_chatAll = keyManager.getKeyByName("T");
    dksvarRegister(CString("k_chatAll [int]"), &k_chatAll, 0, 0, 0, true);
    k_chatTeam = keyManager.getKeyByName("Y");
    dksvarRegister(CString("k_chatTeam [int]"), &k_chatTeam, 0, 0, 0, true);
    k_showScore = keyManager.getKeyByName("TAB");
    dksvarRegister(CString("k_showScore [int]"), &k_showScore, 0, 0, 0, true);
    k_menuAccess = keyManager.getKeyByName("Escape");
    dksvarRegister(CString("k_menuAccess [int]"), &k_menuAccess, 0, 0, 0, true);
    k_melee = keyManager.getKeyByName("Space");
    dksvarRegister(CString("k_melee [int]"), &k_melee, 0, 0, 0, true);
    k_screenShot = keyManager.getKeyByName("P");
    dksvarRegister(CString("k_screenShot [int]"), &k_screenShot, 0, 0, 0, true);

    k_stats = keyManager.getKeyByName("L");
    dksvarRegister(CString("k_stats [int]"), &k_stats, 0, 0, 0, true);

    k_qMsg01 = keyManager.getKeyByName("1");
    dksvarRegister(CString("k_qMsg01 [int]"), &k_qMsg01, 0, 0, 0, true);
    k_qMsg02 = keyManager.getKeyByName("2");
    dksvarRegister(CString("k_qMsg02 [int]"), &k_qMsg02, 0, 0, 0, true);
    k_qMsg03 = keyManager.getKeyByName("3");
    dksvarRegister(CString("k_qMsg03 [int]"), &k_qMsg03, 0, 0, 0, true);
    k_qMsg04 = keyManager.getKeyByName("4");
    dksvarRegister(CString("k_qMsg04 [int]"), &k_qMsg04, 0, 0, 0, true);
    k_qMsg05 = keyManager.getKeyByName("5");
    dksvarRegister(CString("k_qMsg05 [int]"), &k_qMsg05, 0, 0, 0, true);
    k_qMsg06 = keyManager.getKeyByName("6");
    dksvarRegister(CString("k_qMsg06 [int]"), &k_qMsg06, 0, 0, 0, true);
    k_qMsg07 = keyManager.getKeyByName("7");
    dksvarRegister(CString("k_qMsg07 [int]"), &k_qMsg07, 0, 0, 0, true);
    k_qMsg08 = keyManager.getKeyByName("8");
    dksvarRegister(CString("k_qMsg08 [int]"), &k_qMsg08, 0, 0, 0, true);
    k_qMsg09 = keyManager.getKeyByName("9");
    dksvarRegister(CString("k_qMsg09 [int]"), &k_qMsg09, 0, 0, 0, true);
    k_qMsg10 = keyManager.getKeyByName("0");
    dksvarRegister(CString("k_qMsg10 [int]"), &k_qMsg10, 0, 0, 0, true);

    ro_nbParticle = 0;
    ro_hitPoint.set(0, 0, 0);
}

void ClientGameVar::loadModels()
{
    dko_rocket = dkoLoadFile("main/models/Rocket.DKO");
    dko_grenade = dkoLoadFile("main/models/Grenade.DKO");
    dko_douille = dkoLoadFile("main/models/Douille.DKO");
    dko_lifePack = dkoLoadFile("main/models/LifePack.DKO");
    dko_cocktailMolotov = dkoLoadFile("main/models/CocktailMolotov.DKO");
    dko_gib = dkoLoadFile("main/models/Gib.DKO");

    weapons[WEAPON_DUAL_MACHINE_GUN]->loadModels();
    weapons[WEAPON_SMG]->loadModels();
    weapons[WEAPON_CHAIN_GUN]->loadModels();
    weapons[WEAPON_SHOTGUN]->loadModels();
    weapons[WEAPON_SNIPER]->loadModels();
    weapons[WEAPON_BAZOOKA]->loadModels();
    weapons[WEAPON_GRENADE]->loadModels();
    weapons[WEAPON_COCKTAIL_MOLOTOV]->loadModels();
    weapons[WEAPON_KNIVES]->loadModels();
    weapons[WEAPON_PHOTON_RIFLE]->loadModels();
    weapons[WEAPON_FLAME_THROWER]->loadModels();
    weapons[WEAPON_SHIELD]->loadModels();

    tex_nuzzleFlash = dktCreateTextureFromFile("main/textures/nuzzleFlash.png", DKT_FILTER_LINEAR);
    tex_smoke1 = dktCreateTextureFromFile("main/textures/Smoke1.png", DKT_FILTER_LINEAR);
    tex_shotGlow = dktCreateTextureFromFile("main/textures/shotGlow.png", DKT_FILTER_LINEAR);
    tex_smokeTrail = dktCreateTextureFromFile("main/textures/Smoke2.png", DKT_FILTER_LINEAR);
    tex_blood[0] = dktCreateTextureFromFile("main/textures/blood01.png", DKT_FILTER_LINEAR);
    tex_blood[1] = dktCreateTextureFromFile("main/textures/blood02.png", DKT_FILTER_LINEAR);
    tex_blood[2] = dktCreateTextureFromFile("main/textures/blood03.png", DKT_FILTER_LINEAR);
    tex_blood[3] = dktCreateTextureFromFile("main/textures/blood04.png", DKT_FILTER_LINEAR);
    tex_blood[4] = dktCreateTextureFromFile("main/textures/blood05.png", DKT_FILTER_LINEAR);
    tex_blood[5] = dktCreateTextureFromFile("main/textures/blood06.png", DKT_FILTER_LINEAR);
    tex_blood[6] = dktCreateTextureFromFile("main/textures/blood07.png", DKT_FILTER_LINEAR);
    tex_blood[7] = dktCreateTextureFromFile("main/textures/blood08.png", DKT_FILTER_LINEAR);
    tex_blood[8] = dktCreateTextureFromFile("main/textures/blood09.png", DKT_FILTER_LINEAR);
    tex_blood[9] = dktCreateTextureFromFile("main/textures/blood10.png", DKT_FILTER_LINEAR);
    tex_explosionMark = dktCreateTextureFromFile("main/textures/ExplosionMark.png", DKT_FILTER_LINEAR);

    //tex_sniperScope = dktCreateTextureFromFile("main/textures/sniperScope.png", DKT_FILTER_LINEAR);
    tex_sniperScope = dktCreateEmptyTexture(256, 256, 4, DKT_FILTER_BILINEAR);
    {
        const int w = 256;
        const int h = 256;
        const int w_2 = int(256 / 2.0f);
        const int h_2 = int(256 / 2.0f);
        int centerx = 0;//int(w / 2.0f);
        int centery = 0;//int(h / 2.0f);
        int x, y;
        unsigned char imgData[w*h * 4];
        memset(imgData, 0, w*h * 4);

        for(int i = 0; i < w*h; i++)
        {
            x = (i % w);
            y = int(i / (float)w);
            int distx = centerx - (x - w_2);
            int disty = centery - (y - h_2);
            float len = float(sqrt(double(distx*distx + disty * disty)));
            if(len < 118 && x != w_2 && y != h_2)
                imgData[i * 4 + 3] = 0;
            else
                imgData[i * 4 + 3] = 255;
        }

        // update
        dktCreateTextureFromBuffer(&tex_sniperScope, imgData, 256, 256, 4, DKT_FILTER_LINEAR);
    }

    tex_medals[0] = dktCreateTextureFromFile("main/textures/medals/Medal01.png", DKT_FILTER_LINEAR);
    tex_medals[1] = dktCreateTextureFromFile("main/textures/medals/Medal02.png", DKT_FILTER_LINEAR);
    tex_medals[2] = dktCreateTextureFromFile("main/textures/medals/Medal03.png", DKT_FILTER_LINEAR);
    tex_medals[3] = dktCreateTextureFromFile("main/textures/medals/Medal04.png", DKT_FILTER_LINEAR);
    tex_medals[4] = dktCreateTextureFromFile("main/textures/medals/Medal05.png", DKT_FILTER_LINEAR);
    tex_medals[5] = dktCreateTextureFromFile("main/textures/medals/Medal06.png", DKT_FILTER_LINEAR);
    tex_medals[6] = dktCreateTextureFromFile("main/textures/medals/Medal07.png", DKT_FILTER_LINEAR);
    tex_medals[7] = dktCreateTextureFromFile("main/textures/medals/Medal08.png", DKT_FILTER_LINEAR);
    tex_medals[8] = dktCreateTextureFromFile("main/textures/medals/Medal09.png", DKT_FILTER_LINEAR);
    tex_medals[9] = dktCreateTextureFromFile("main/textures/medals/Medal10.png", DKT_FILTER_LINEAR);
    tex_medals[10] = dktCreateTextureFromFile("main/textures/medals/Medal11.png", DKT_FILTER_LINEAR);
    tex_drip = dktCreateTextureFromFile("main/textures/drip.png", DKT_FILTER_LINEAR);
    tex_sky = dktCreateTextureFromFile("main/textures/sky.png", DKT_FILTER_LINEAR);
    tex_glowTrail = dktCreateTextureFromFile("main/textures/glowTrail.png", DKT_FILTER_LINEAR);

    sfx_ric[0] = dksCreateSoundFromFile("main/sounds/ric1.wav", false);
    sfx_ric[1] = dksCreateSoundFromFile("main/sounds/ric2.wav", false);
    sfx_ric[2] = dksCreateSoundFromFile("main/sounds/ric3.wav", false);
    sfx_ric[3] = dksCreateSoundFromFile("main/sounds/ric4.wav", false);
    sfx_ric[4] = dksCreateSoundFromFile("main/sounds/ric5.wav", false);
    sfx_hit[0] = dksCreateSoundFromFile("main/sounds/hit1.wav", false);
    sfx_hit[1] = dksCreateSoundFromFile("main/sounds/hit2.wav", false);
    sfx_baboCreve[0] = dksCreateSoundFromFile("main/sounds/BaboCreve1.wav", false);
    sfx_baboCreve[1] = dksCreateSoundFromFile("main/sounds/BaboCreve2.wav", false);
    sfx_baboCreve[2] = dksCreateSoundFromFile("main/sounds/BaboCreve3.wav", false);
    sfx_explosion[0] = dksCreateSoundFromFile("main/sounds/Explosion1.wav", false);
    sfx_grenadeRebond = dksCreateSoundFromFile("main/sounds/GrenadeRebond.wav", false);
    sfx_douille[0] = dksCreateSoundFromFile("main/sounds/douille1.wav", false);
    sfx_douille[1] = dksCreateSoundFromFile("main/sounds/douille2.wav", false);
    sfx_douille[2] = dksCreateSoundFromFile("main/sounds/douille3.wav", false);
    sfx_equip = dksCreateSoundFromFile("main/sounds/equip.wav", false);
    sfx_lifePack = dksCreateSoundFromFile("main/sounds/LifePack.wav", false);
    sfx_cocktailMolotov = dksCreateSoundFromFile("main/sounds/cocktailmolotov.wav", false);
    sfx_lavaSteam = dksCreateSoundFromFile("main/sounds/lavasteam.wav", false);
    sfx_overHeat = dksCreateSoundFromFile("main/sounds/overHeat.wav", false);
    sfx_photonStart = dksCreateSoundFromFile("main/sounds/PhotonStart.wav", false);

    dkpp_firingSmoke.angleFrom = 0;
    dkpp_firingSmoke.angleTo = 360;
    dkpp_firingSmoke.angleSpeedFrom = -30;
    dkpp_firingSmoke.angleSpeedTo = 30;
    dkpp_firingSmoke.srcBlend = DKP_SRC_ALPHA;
    dkpp_firingSmoke.dstBlend = DKP_ONE_MINUS_SRC_ALPHA;
    dkpp_firingSmoke.durationFrom = .25f;
    dkpp_firingSmoke.durationTo = .5f;
    dkpp_firingSmoke.endColorFrom.set(1, 1, .7f, .0f);
    dkpp_firingSmoke.endColorTo.set(1, 1, .7f, .0f);
    dkpp_firingSmoke.startColorFrom.set(.7f, .7f, .7f, 1.0f);
    dkpp_firingSmoke.startColorTo.set(.7f, .7f, .7f, 1.0f);
    dkpp_firingSmoke.endSizeFrom = .25f;
    dkpp_firingSmoke.endSizeTo = .30f;
    dkpp_firingSmoke.startSizeFrom = .15f;
    dkpp_firingSmoke.startSizeTo = .18f;
    dkpp_firingSmoke.airResistanceInfluence = 0;
    dkpp_firingSmoke.gravityInfluence = 0;
    dkpp_firingSmoke.pitchFrom = 0;
    dkpp_firingSmoke.pitchTo = 180;
    dkpp_firingSmoke.speedFrom = .0f;
    dkpp_firingSmoke.speedTo = 1.8f;
    dkpp_firingSmoke.textureFrameCount = 0;
    dkpp_firingSmoke.texture = &tex_smoke1;
    dkpp_firingSmoke.particleCountFrom = 2;
    dkpp_firingSmoke.particleCountTo = 4;
}

void ClientGameVar::deleteModels()
{
    dkoDeleteModel(&dko_rocket);
    dkoDeleteModel(&dko_grenade);
    dkoDeleteModel(&dko_douille);
    dkoDeleteModel(&dko_lifePack);
    dkoDeleteModel(&dko_cocktailMolotov);
    dkoDeleteModel(&dko_gib);

    dktDeleteTexture(&tex_nuzzleFlash);
    dktDeleteTexture(&tex_smoke1);
    dktDeleteTexture(&tex_shotGlow);
    dktDeleteTexture(&tex_smokeTrail);
    dktDeleteTexture(&tex_blood[0]);
    dktDeleteTexture(&tex_blood[1]);
    dktDeleteTexture(&tex_blood[2]);
    dktDeleteTexture(&tex_blood[3]);
    dktDeleteTexture(&tex_blood[4]);
    dktDeleteTexture(&tex_blood[5]);
    dktDeleteTexture(&tex_blood[6]);
    dktDeleteTexture(&tex_blood[7]);
    dktDeleteTexture(&tex_blood[8]);
    dktDeleteTexture(&tex_blood[9]);
    dktDeleteTexture(&tex_explosionMark);
    dktDeleteTexture(&tex_sniperScope);
    dktDeleteTexture(tex_medals + 0);
    dktDeleteTexture(tex_medals + 1);
    dktDeleteTexture(tex_medals + 2);
    dktDeleteTexture(tex_medals + 3);
    dktDeleteTexture(tex_medals + 4);
    dktDeleteTexture(tex_medals + 5);
    dktDeleteTexture(tex_medals + 6);
    dktDeleteTexture(tex_medals + 7);
    dktDeleteTexture(tex_medals + 8);
    dktDeleteTexture(tex_medals + 9);
    dktDeleteTexture(tex_medals + 10);
    dktDeleteTexture(&tex_drip);
    dktDeleteTexture(&tex_sky);
    dktDeleteTexture(&tex_glowTrail);

    dksDeleteSound(sfx_ric[0]);
    dksDeleteSound(sfx_ric[1]);
    dksDeleteSound(sfx_ric[2]);
    dksDeleteSound(sfx_ric[3]);
    dksDeleteSound(sfx_ric[4]);
    dksDeleteSound(sfx_hit[3]);
    dksDeleteSound(sfx_hit[4]);
    dksDeleteSound(sfx_baboCreve[0]);
    dksDeleteSound(sfx_baboCreve[1]);
    dksDeleteSound(sfx_baboCreve[2]);
    dksDeleteSound(sfx_explosion[0]);
    dksDeleteSound(sfx_grenadeRebond);
    dksDeleteSound(sfx_douille[0]);
    dksDeleteSound(sfx_douille[1]);
    dksDeleteSound(sfx_douille[2]);
    dksDeleteSound(sfx_equip);
    dksDeleteSound(sfx_lifePack);
    dksDeleteSound(sfx_cocktailMolotov);
    dksDeleteSound(sfx_lavaSteam);
    dksDeleteSound(sfx_overHeat);
    dksDeleteSound(sfx_photonStart);
}

// Pour loader les lang_ var
bool ClientGameVar::loadLanguage(char * filename)
{
    int i;
    FILE * fic = fopen(filename, "r");

    if(!fic) return false;

    char varName[256];
    char varValue[512];
    while(true)
    {
        fscanf(fic, "%s", varName);
        if(strcmp(varName, "END") == 0)
        {
            languageLoaded = true;
            return true;
        }
        for(i = 0;;)
        {
            int character = fgetc(fic);
            if(character == 199) { character = 128; }
            if(character == 252) { character = 129; }
            if(character == 233) { character = 130; }
            if(character == 226) { character = 131; }
            if(character == 228) { character = 132; }
            if(character == 224) { character = 133; }
            if(character == 229) { character = 134; }
            if(character == 231) { character = 135; }
            if(character == 234) { character = 136; }
            if(character == 235) { character = 137; }
            if(character == 232) { character = 138; }
            if(character == 239) { character = 139; }
            if(character == 238) { character = 140; }
            if(character == 236) { character = 141; }
            if(character == 196) { character = 142; }
            if(character == 197) { character = 143; }
            if(character == 201) { character = 144; }
            if(character == 230) { character = 145; }
            if(character == 198) { character = 146; }
            if(character == 244) { character = 147; }
            if(character == 246) { character = 148; }
            if(character == 242) { character = 149; }
            if(character == 251) { character = 150; }
            if(character == 249) { character = 151; }
            if(character == 255) { character = 152; }
            if(character == 214) { character = 153; }
            if(character == 220) { character = 154; }
            if(character == 248) { character = 155; }
            if(character == 163) { character = 156; }
            if(character == 216) { character = 157; }
            if(character == 215) { character = 158; }
            if(character == 170) { character = 159; }
            if(character == '\t')
            {
                continue;
            }
            if(character == '\n')
            {
                varValue[i] = '\0';
                break;
            }
            if(character == '%')
            {
                varValue[i] = character;
                ++i;
            }
            if(character == '\\')
            {
                character = fgetc(fic);
                switch(character)
                {
                case 'x':
                    character = fgetc(fic);
                    switch(character)
                    {
                    case '1': character = '\x1'; break;
                    case '2': character = '\x2'; break;
                    case '3': character = '\x3'; break;
                    case '4': character = '\x4'; break;
                    case '5': character = '\x5'; break;
                    case '6': character = '\x6'; break;
                    case '7': character = '\x7'; break;
                    case '8': character = '\x8'; break;
                    case '9': character = '\x9'; break;
                    }
                    break;
                case 'n':
                    character = '\n';
                    break;
                case '\\':
                    character = '\\';
                    break;
                }
            }
            varValue[i] = character;
            ++i;
        }
        CString value = varValue;
        for(i = 0; i < (int)langs.size(); ++i)
        {
            if(langs[i].varName == varName)
            {
                *(langs[i].varPtr) = value;
            }
        }
    }
    languageLoaded = true;
    return true;
}

bool ClientGameVar::isLanguageLoaded()
{
    return languageLoaded;
}
