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
#ifndef CLIENT_GAMEVAR_H
#define CLIENT_GAMEVAR_H

#include "GameVar.h"
#include "ClientWeapon.h"

struct ClientGameVar
{
    unsigned int dko_rocket;
    unsigned int dko_grenade;
    unsigned int dko_douille;
    unsigned int dko_lifePack;
    unsigned int dko_cocktailMolotov;
    unsigned int dko_gib;

    // Les gun
    ClientWeapon * weapons[20];

    // Textures
    unsigned int tex_nuzzleFlash;
    unsigned int tex_smoke1;
    unsigned int tex_shotGlow;
    unsigned int tex_smokeTrail;
    unsigned int tex_blood[10];
    unsigned int tex_explosionMark;
    unsigned int tex_sniperScope;
    unsigned int tex_medals[32];
    unsigned int tex_drip;
    unsigned int tex_sky;
    unsigned int tex_glowTrail;

    // Particles presets
    dkp_preset dkpp_firingSmoke;
    dkp_preset dkpp_bloodHit;

    // Les sons
    FSOUND_SAMPLE * sfx_ric[5];
    FSOUND_SAMPLE * sfx_hit[2];
    FSOUND_SAMPLE * sfx_baboCreve[3];
    FSOUND_SAMPLE * sfx_explosion[1];
    FSOUND_SAMPLE * sfx_grenadeRebond;
    FSOUND_SAMPLE * sfx_douille[3];
    FSOUND_SAMPLE * sfx_equip;
    FSOUND_SAMPLE * sfx_lifePack;
    FSOUND_SAMPLE * sfx_cocktailMolotov;
    FSOUND_SAMPLE * sfx_lavaSteam;
    FSOUND_SAMPLE * sfx_overHeat;
    FSOUND_SAMPLE * sfx_photonStart;

    // LES STRING DIFFêRENT POUR CHAQUE LANGUE
    CString lang_gameName;

    CString lang_dualMachineGun;
    CString lang_subMachineGun;
    CString lang_changGun;
    CString lang_shotgun;
    CString lang_sniper;
    CString lang_bazooka;
    CString lang_grenade;
    CString lang_flame;

    CString lang_autoAssignTeam;
    CString lang_joinBlueTeam;
    CString lang_joinRedTeam;
    CString lang_spectator;
    CString lang_mainMenu;
    CString lang_disconnect;

    CString lang_serverDisconnected;

    CString lang_dead;
    CString lang_team;

    CString lang_joinedTheGame;
    CString lang_playerDisconnected;
    CString lang_player;
    CString lang_playerChangedHisNameFor;
    CString lang_returnBlueFlag;
    CString lang_returnRedFlag;
    CString lang_tookBlueFlag;
    CString lang_tookRedFlag;
    CString lang_redScore;
    CString lang_blueScore;

    CString lang_reloading;

    CString lang_spawnIn;
    CString lang_pressToRespawn;

    CString lang_blueTeamWin;
    CString lang_redTeamWin;
    CString lang_roundDraw;
    CString lang_changingMap;

    CString lang_deathmatchC;
    CString lang_deathmatchD;
    CString lang_teamDeathmatchC;
    CString lang_teamDeathmatchD;
    CString lang_captureTheFlagC;
    CString lang_captureTheFlagD;
    CString lang_counterBaboristC;
    CString lang_counterBaboristD;

    CString lang_connectingC;
    CString lang_pressF10ToCancel;

    CString lang_goesSpectator;
    CString lang_joinBlueTeamP;
    CString lang_joinRedTeamP;

    CString lang_blueTeamC;
    CString lang_redTeamC;
    CString lang_freeForAllC;
    CString lang_spectatorC;

    CString lang_playerNameC;
    CString lang_scoreC;
    CString lang_pingC;

    CString lang_laggerC;

    CString lang_friendlyFire;

    CString lang_serverVersion;
    CString lang_clientVersion;

    CString lang_connectionFailed;
    CString lang_back;

    CString lang_launch;
    CString lang_dedicate;

    CString lang_generalGameOption;
    CString lang_gameNameS;
    CString lang_gameType;
    CString lang_serverType;
    CString lang_spawnType;
    CString lang_subGameType;

    CString lang_freeForAll;
    CString lang_teamDeathmatch;
    CString lang_captureTheFlag;
    CString lang_TCPnUDPport;
    CString lang_gameLimits;
    CString lang_scoreLimit;
    CString lang_teamWinLimit;
    CString lang_timeLimit;
    CString lang_maxPlayer;
    CString lang_spawning;
    CString lang_timeToSpawn;
    CString lang_forceRespawn;
    CString lang_no;
    CString lang_yes;
    CString lang_reflectDamage;
    CString lang_maps;
    CString lang_includedMaps;

    CString lang_RndLabsTeam;
    CString lang_gameDesigner;
    CString lang_programmers;
    CString lang_soundDesigners;
    CString lang_musics;
    CString lang_thanksTo;
    CString lang_betaTesters;
    CString lang_buyAlbum;

    CString lang_internet;
    CString lang_localNetwork;
    CString lang_refreshList;
    CString lang_players;
    CString lang_mapName;
    CString lang_ping;
    CString lang_join;
    CString lang_IPAdress;
    CString lang_canRefreshIn;
    CString lang_nbGamesRefreshed;
    CString lang_noGameFound;
    CString lang_problemConnectingToMaster;

    CString lang_resumeCurrentGame;
    CString lang_joinGame;
    CString lang_createServer;
    CString lang_options;
    CString lang_credits;
    CString lang_quit;

    CString lang_noGameRunning;

    CString lang_selectAtLeastOneMap;

    CString lang_apply;
    CString lang_cancel;
    CString lang_someOptionRequireRestart;
    CString lang_clientOptions;
    CString lang_playerName;
    CString lang_inGameMusic;
    CString lang_renderingOptions;
    CString lang_showStats;
    CString lang_fullscreen;
    CString lang_screenResolution;
    CString lang_colorDepth;
    CString lang_bits;
    CString lang_wallShadowQuality;
    CString lang_noShadows;
    CString lang_hardEdges;
    CString lang_softEdges;
    CString lang_baboShadow;
    CString lang_projectileShadow;
    CString lang_particleLighting;
    CString lang_particleSorting;
    CString lang_terrainSplatter;
    CString lang_showCasing;
    CString lang_groundMarkNBlood;
    CString lang_soundOptions;
    CString lang_mixRate;
    CString lang_hertz;
    CString lang_maxSoftwareChannels;
    CString lang_masterVolume;
    CString lang_keyOptions;
    CString lang_moveUp;
    CString lang_moveDown;
    CString lang_moveRight;
    CString lang_moveLeft;
    CString lang_shoot;
    CString lang_throwGrenade;
    CString lang_throwCocktailMolotov;
    CString lang_use;
    CString lang_chatToAll;
    CString lang_chatToTeam;
    CString lang_showScoreBoard;
    CString lang_toggleMenu;

    CString lang_areYouSureYouWantToQuit;
    CString lang_yesQ;
    CString lang_noQ;

    CString lang_serverHasClosedTheConnection;

    CString lang_language;
    CString lang_incorrectName;

    int k_moveUp;
    int k_moveDown;
    int k_moveRight;
    int k_moveLeft;
    int k_shoot;
    int k_throwGrenade;
    int k_throwMolotov;
    int k_pickUp;
    int k_chatAll;
    int k_chatTeam;
    int k_showScore;
    int k_menuAccess;
    int k_melee;
    int k_screenShot;
    int k_stats;

    // quick messages
    int k_qMsg01;
    int k_qMsg02;
    int k_qMsg03;
    int k_qMsg04;
    int k_qMsg05;
    int k_qMsg06;
    int k_qMsg07;
    int k_qMsg08;
    int k_qMsg09;
    int k_qMsg10;
    int ro_nbParticle;
    CVector3f ro_hitPoint;


    // List des variables lang_ pour loader rapidement d'un fichier .lang
    std::vector<SLangText> langs;

    ClientGameVar();

    // Destructeur
    virtual ~ClientGameVar();

    // on load les models pour le jeu
    void loadModels();

    // pour effacer les models du jeu
    void deleteModels();

    dkContext* dk;
    void init(dkContext* ctx);

    bool languageLoaded;
    // Pour loader les lang_ var
    bool loadLanguage(char * filename);
    bool isLanguageLoaded();
};
extern ClientGameVar clientVar;

#endif
