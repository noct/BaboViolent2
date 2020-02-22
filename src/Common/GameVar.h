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

#ifndef GAMEVAR_H
#define GAMEVAR_H

#include <Zeven/Core.h>
#include "Weapon.h"

#define WEAPON_SMG 0
#define WEAPON_SHOTGUN 1
#define WEAPON_SNIPER 2
#define WEAPON_DUAL_MACHINE_GUN 3
#define WEAPON_CHAIN_GUN 4
#define WEAPON_BAZOOKA 5
#define WEAPON_PHOTON_RIFLE 6
#define WEAPON_FLAME_THROWER 7
#define WEAPON_GRENADE 8
#define WEAPON_COCKTAIL_MOLOTOV 9
#define WEAPON_KNIVES 10
#define WEAPON_SHIELD 11

#define SOUND_GRENADE_REBOUND 1
#define SOUND_MOLOTOV 2
#define SOUND_OVERHEAT 3
#define SOUND_PHOTON_START 4

#define SUBGAMETYPE_NORMAL 0
#define SUBGAMETYPE_INSTAGIB 1
#define SUBGAMETYPE_RANDOMWEAPON 2

struct SLangText
{
    CString varName;
    CString * varPtr;
    SLangText(CString in_varName, CString * in_varPtr) :
        varName(in_varName), varPtr(in_varPtr) {}
};

struct GameVar
{
    bool sv_friendlyFire;
    bool sv_reflectedDamage;
    float sv_timeToSpawn;
    bool sv_topView;
    int sv_minSendInterval;
    bool sv_forceRespawn;
    bool sv_baboStats;
    float sv_roundTimeLimit;
    float sv_gameTimeLimit;
    int sv_scoreLimit;
    int sv_winLimit;
    int sv_gameType;
    int sv_serverType;
    int sv_spawnType;
    int sv_subGameType;
    float sv_bombTime;
    CString sv_gameName;
    int sv_port;
    int sv_maxPlayer;
    int sv_maxPlayerInGame;
    float sv_minTilesPerBabo;
    float sv_maxTilesPerBabo;
    CString sv_password;
    bool sv_enableSMG;
    bool sv_enableShotgun;
    bool sv_enableSniper;
    bool sv_enableDualMachineGun;
    bool sv_enableChainGun;
    bool sv_enableBazooka;
    bool sv_enableShotgunReload;
    bool sv_slideOnIce;
    bool sv_showEnemyTag;
    bool sv_enableSecondary;
    bool sv_enableKnives;
    bool sv_enableNuclear;
    bool sv_enableShield;
    float sv_shottyDropRadius;
    float sv_shottyRange;
    float sv_ftMaxRange;
    float sv_ftMinRange;
    float sv_photonDamageCoefficient;
    float sv_ftDamage;
    float sv_smgDamage;
    float sv_dmgDamage;
    float sv_cgDamage;
    float sv_sniperDamage;
    float sv_shottyDamage;
    float sv_zookaRadius;
    float sv_nukeRadius;
    float sv_nukeTimer;
    float sv_nukeReload;
    float sv_zookaDamage;
    float sv_ftExpirationTimer;
    int sv_photonType;
    bool sv_zookaRemoteDet;
    bool sv_enableMolotov;
    float sv_photonDistMult;
    float sv_photonHorizontalShift;
    float sv_photonVerticalShift;
    bool sv_autoBalance;
    int sv_autoBalanceTime;
    bool sv_gamePublic;
    bool sv_enableVote;
    bool sv_enablePhotonRifle;
    bool sv_enableFlameThrower;
    float sv_maxUploadRate;
    bool sv_showKills;
    CString sv_matchcode;
    int sv_matchmode;
    bool sv_report;
    int sv_maxPing;
    bool sv_autoSpectateWhenIdle;
    int sv_autoSpectateIdleMaxTime;
    bool sv_beGoodServer;
    bool sv_validateWeapons;
    CString sv_joinMessage;
    bool sv_sendJoinMessage;

    //spawn immunity
    float sv_spawnImmunityTime;

    //--- For remote control
    CString zsv_adminUser;
    CString zsv_adminPass;

    int db_version;

    CString cl_playerName;
    CString cl_mapAuthorName;
    bool cl_cubicMotion;
    CString cl_lastUsedIP;
    int cl_port;
    CString cl_password;
    CColor3f cl_redDecal;
    CColor3f cl_greenDecal;
    CColor3f cl_blueDecal;
    CString cl_skin;
    int cl_teamIndicatorType;
    float cl_glowSize;
    bool cl_grassTextureForAllMaps;
    bool cl_preciseCursor;
    bool cl_enableXBox360Controller;
    bool cl_enableVSync;
    int cl_affinityMode;
    int cl_primaryWeapon;
    //bool cl_weaponSideRight;
    int cl_secondaryWeapon;

    // quick messages
    CString cl_qMsg01;
    CString cl_qMsg02;
    CString cl_qMsg03;
    CString cl_qMsg04;
    CString cl_qMsg05;
    CString cl_qMsg06;
    CString cl_qMsg07;
    CString cl_qMsg08;
    CString cl_qMsg09;
    CString cl_qMsg10;

    //--- Those are not included into the cfg file
    long scl_honor;
    long scl_xp;
    long scl_leftToNextLevel;
    long scl_totalKill;
    long scl_totalDeath;
    float scl_ratio;
    long scl_weaponOfChoice;
    long scl_killWeapon[20];

    bool r_showStats;
    bool r_fullScreen;
    CVector2i r_resolution;
    int r_refreshRate;
    bool r_weatherEffects;
    int r_shadowQuality;
    bool r_playerShadow;
    bool r_projectileShadow;
    bool r_showCasing;
    bool r_showGroundMark;
    bool r_showLatency;
    bool r_simpleText;
    int r_maxNameLenOverBabo;

    int r_chatTextSize;
    int r_eventTextSize;
    bool r_showEventText;


    int s_mixRate;
    int s_maxSoftwareChannels;
    float s_masterVolume;
    bool s_inGameMusic;

    // Si on est en debug !!! trclientVar.dkpp_s important
    bool c_debug;
    bool c_huge;
    bool d_showPath;
    bool d_showNodes;

    CString languageFile;

    Weapon * weapons[20];

    // Constructeur
    dkContext* dk;
    GameVar();

    // Destructeur
    virtual ~GameVar();

    void init(dkContext* ctx);

    // Pour envoyer les variables servers au autres tayouin
    void sendSVVar(uint32_t babonetID);
    // send server var to a peer(remote admin)
    void sendSVVar(int32_t peerId);
    void sendOne(char * varName, uint32_t babonetID);
    // send one var to a peer( remote admin )
    void sendOne(char * varName, int32_t peerId);
};

extern GameVar gameVar;
#endif
