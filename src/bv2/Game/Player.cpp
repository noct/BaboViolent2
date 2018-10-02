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

#include "Player.h"
#include "GameVar.h"
#include "Console.h"
#include "Map.h"
#include "Game.h"
#include "Scene.h"
#include <limits>
#include "Server.h"

#ifndef DEDICATED_SERVER
#include "ClientGame.h"
#include "ClientScene.h"
#include "ClientHelper.h"
#include "ClientMap.h"
#include "ClientConsole.h"
#endif

extern Scene * scene;


PlayerStats::PlayerStats(const Player* player)
{
    name = player->name;//textColorLess(player->name);
    userID = player->userID;
    teamID = player->teamID;
    kills = player->kills;
    deaths = player->deaths;
    dmg = player->dmg;
    score = player->score;
    returns = player->returns;
    flagAttempts = player->flagAttempts;
    timePlayedCurGame = player->timePlayedCurGame;
}

void PlayerStats::MergeStats(PlayerStats* mergeWith)
{
    if(userID == 0 || userID != mergeWith->userID)
        return;
    name = textColorLess(mergeWith->name);
    kills += mergeWith->kills;
    deaths += mergeWith->deaths;
    dmg += mergeWith->dmg;
    score += mergeWith->score;
    returns += mergeWith->returns;
    flagAttempts += mergeWith->flagAttempts;
    timePlayedCurGame += mergeWith->timePlayedCurGame;
}

//
// Constructeur
//
Player::Player(char pPlayerID, Map * pMap, Game * pGame) : pingLogInterval(0.05f),
pingLogID(0)
{
    nextPingLogTime = pingLogInterval = 1.0f / 30;
    rocketInAir = false;
    detonateRocket = false;

    for(int i = 0; i < PING_LOG_SIZE; i++)
        pingLog[i] = 0;
    // enough time has elapsed for any good to be able to shoot when creating the Player
    mfElapsedSinceLastShot = 9999.0f;

    miNbCoord = 0;
    mfCFTimer = 0.0f;
    mfCumulativeVel = 0.0f;
    shootShakeDis.set(0, 0, 0);

    spawnSlot = -1;

    timeDead = 0.0f;
    timeAlive = 0.0f;
    timeIdle = 0.0f;
    timeInServer = 0.0f;

    userID = 0;

    incShot = 0;
    voted = false;
    protection = 0;
    isAdmin = false;
    playerIP[0] = '\0';
    frameSinceLast = 0;
    lastFrame = 0;
    currentFrame = 0;
    speedHackCount = 0;
    shotsPerSecond = 0;
    shotCount = 0;
    secondPassed = 0;
    meleeDelay = 0;
    fireFrameDelay = 0;
    game = pGame;
    map = pMap;
    name = "Unnamed Babo";
    babonetID = 0; // Retreive this from gknet_
    playerID = pPlayerID;
    teamID = PLAYER_TEAM_SPECTATOR; // -1 = spectator, 0 = bleu, 1 = rouge
    status = PLAYER_STATUS_LOADING;
    ping = -1;
    pingSum = 0;
    avgPing = 0;
    babySitTime = 5.0f;
    pingOverMax = 0.0f;
    memset(pingLog, 0, sizeof(int)*PING_LOG_SIZE);

    dmg = 0;
    kills = 0;
    deaths = 0;
    score = 0;
    returns = 0;
    damage = 0;
    flagAttempts = 0;

    timePlayedCurGame = 0.0f;
    waitForPong = false;
    sendPosFrame = 0;
#ifndef DEDICATED_SERVER
    isThisPlayer = false;
#endif
    timeToSpawn = gameVar.sv_timeToSpawn;
    remoteEntity = true;
    cFProgression = 0;
    weapon = 0;
    meleeWeapon = 0;
#ifndef DEDICATED_SERVER
    tex_baboShadow = dktCreateTextureFromFile("main/textures/BaboShadow.tga", DKT_FILTER_BILINEAR);
    tex_baboHalo = dktCreateTextureFromFile("main/textures/BaboHalo.tga", DKT_FILTER_BILINEAR);
#endif
    nextSpawnWeapon = gameVar.cl_primaryWeapon;//WEAPON_SMG;
    nextMeleeWeapon = gameVar.cl_secondaryWeapon + WEAPON_KNIVES;//WEAPON_KNIVES;
#ifndef DEDICATED_SERVER
    initedMouseClic = false;
#endif
    firedShowDelay = 0;
    deadSince = 0;
#ifndef DEDICATED_SERVER
    followingPlayer = 0;
#endif
    screenHit = 0;
    grenadeDelay = 0;
    nbGrenadeLeft = 2; // On commence toujours avec 2 grenade
    nbMolotovLeft = 1; // On commence toujours avec 1 molotov
    currentPingFrame = 0;
    connectionInterrupted = false;
    spawnRequested = false;
#ifndef DEDICATED_SERVER
    scopeMode = false;
#endif

    //--- on load un skin par default
    skin = "skin10";
#ifndef DEDICATED_SERVER
    tex_skinOriginal = dktCreateTextureFromFile((CString("main/skins/") + skin + ".tga").s, DKT_FILTER_BILINEAR);
    tex_skin = dktCreateEmptyTexture(64, 32, 3, DKT_FILTER_BILINEAR);
#endif
}



//
// Destructeur
//
Player::~Player()
{
#ifndef DEDICATED_SERVER
    ZEVEN_SAFE_DELETE(weapon);
    dktDeleteTexture(&tex_baboShadow);
    dktDeleteTexture(&tex_baboHalo);
    dktDeleteTexture(&tex_skinOriginal);
    dktDeleteTexture(&tex_skin);
#endif
    //--- Est-ce qu'on est server et que ce player poc�e le flag???
    if(scene->server)
    {
        if(scene->server->game && !scene->server->needToShutDown)
        {
            if(scene->server->game->map)
            {
                for(int i = 0; i < 2; ++i)
                {
                    if(scene->server->game->map->flagState[i] == playerID)
                    {
                        scene->server->game->map->flagState[i] = -1; // Le server va nous communiquer la position du flag exacte
                        scene->server->game->map->flagPos[i] = currentCF.position;
                        scene->server->game->map->flagPos[i][2] = 0;
                        if(scene->server->game->isServerGame)
                        {
                            // On envoit la new pos du flag aux autres
                            net_svcl_drop_flag dropFlag;
                            dropFlag.flagID = (char)i;
                            dropFlag.position[0] = scene->server->game->map->flagPos[i][0];
                            dropFlag.position[1] = scene->server->game->map->flagPos[i][1];
                            dropFlag.position[2] = scene->server->game->map->flagPos[i][2];
                            bb_serverSend((char*)&dropFlag, sizeof(net_svcl_drop_flag), NET_SVCL_DROP_FLAG, 0);
                        }
                    }
                }
            }
        }
    }
}



//
// Pour le forcer �crever (suposons quil change de team)
//
void Player::kill(bool silenceDeath)
{
#ifndef DEDICATED_SERVER
    if(silenceDeath)
    {
#endif
        status = PLAYER_STATUS_DEAD;
        deadSince = 0;
#ifndef DEDICATED_SERVER
    }
    else
    {
        status = PLAYER_STATUS_DEAD;
        dksPlay3DSound(clientVar.sfx_baboCreve[rand() % 3], -1, 5, currentCF.position, 255);
        auto cgame = static_cast<ClientGame*>(game);
        cgame->spawnBlood(currentCF.position, 1);
        deadSince = 0;

        //--- Spawn some gibs :D
    /*  for (int i=0;i<10;++i)
        {
            if (game) game->douilles.push_back(new Douille(currentCF.position,
                rand(CVector3f(-2.5,-2.5,1),CVector3f(2.5,2.5,2.5)),
                CVector3f(1,0,0), DOUILLE_TYPE_GIB));
        }*/
    }
#endif

    // Si il avait le flag, on le laisse tomber
    for(int i = 0; i < 2; ++i)
    {
        if(game->map->flagState[i] == playerID)
        {
            game->map->flagState[i] = -1; // Le server va nous communiquer la position du flag exacte
            game->map->flagPos[i] = currentCF.position;
            game->map->flagPos[i][2] = 0;
#ifndef DEDICATED_SERVER
            auto cmap = static_cast<ClientMap*>(game->map);
            cmap->flagAngle[i] = 0;
#endif
            if(game->isServerGame)
            {
                // On envoit la new pos du flag aux autres
                net_svcl_drop_flag dropFlag;
                dropFlag.flagID = (char)i;
                dropFlag.position[0] = game->map->flagPos[i][0];
                dropFlag.position[1] = game->map->flagPos[i][1];
                dropFlag.position[2] = game->map->flagPos[i][2];
                bb_serverSend((char*)&dropFlag, sizeof(net_svcl_drop_flag), NET_SVCL_DROP_FLAG, 0);
            }
        }
    }
    currentCF.position.set(-999, -999, 0);
}


//
// Pour spawner un joueur y�
//
void Player::spawn(const CVector3f & spawnPoint)
{
    status = PLAYER_STATUS_ALIVE;
    life = 1; // Full of life
    timeToSpawn = gameVar.sv_timeToSpawn;
    immuneTime = gameVar.sv_spawnImmunityTime;

    timeDead = 0.0f;
    timeAlive = 0.0f;
    timeIdle = 0.0f;

    matrix.LoadIdentity();

    spawnRequested = false;

    currentCF.position = spawnPoint;
    currentCF.vel.set(0, 0, 0);
    currentCF.angle = 0;

    lastCF = currentCF;
    netCF0 = currentCF;
    netCF1 = currentCF;
    netCF0.reset();
    netCF1.reset();
    cFProgression = 0;

    grenadeDelay = 0;
    meleeDelay = 0;
    nbGrenadeLeft = 2;
    nbMolotovLeft = 1;


    switchWeapon(nextSpawnWeapon);
    switchMeleeWeapon(nextMeleeWeapon);

    //--- Si c'est nous on force la camera dessus
#ifndef DEDICATED_SERVER
    auto cmap = static_cast<ClientMap*>(game->map);
    if(isThisPlayer) cmap->setCameraPos(spawnPoint);

    if(!game->isServerGame)
    {
        updateSkin();
    }
#endif
}



//
// Pour remettre ses stats �0
//
void Player::reinit()
{
    timeIdle = 0.0f;
    dmg = 0;
    kills = 0;
    deaths = 0;
    score = 0;
    returns = 0;
    damage = 0;
    ping = 0;
    pingSum = 0;
    avgPing = 0;
    babySitTime = 5.0f;
    pingOverMax = 0.0f;
    flagAttempts = 0;
    timePlayedCurGame = 0.0f;
    spawnSlot = -1;
}



//
// Pour changer son gun!
//
void Player::switchWeapon(int newWeaponID, bool forceSwitch)
{
    if(weapon && forceSwitch)
    {
        if(weapon->weaponID == newWeaponID) return;
    }
    // Bon testing, on va lui refiler un gun
    ZEVEN_SAFE_DELETE(weapon);
#ifndef DEDICATED_SERVER
    weapon = new ClientWeapon(clientVar.weapons[newWeaponID]);
#else
    weapon = new Weapon(gameVar.weapons[newWeaponID]);
#endif
    weapon->currentFireDelay = 1; // On a une 1sec de delait quand on switch de gun
    weapon->m_owner = this;

    gameVar.cl_primaryWeapon = newWeaponID;

    // On entends �
#ifndef DEDICATED_SERVER
    auto cscene = static_cast<ClientScene*>(scene);
    if(cscene->client) dksPlay3DSound(clientVar.sfx_equip, -1, 1, currentCF.position, 255);
#endif

    // Reset rapid-fire hack check
    shotCount = 0;
    shotsPerSecond = 0;
}



//
// Pour changer son melee gun!
//
void Player::switchMeleeWeapon(int newWeaponID, bool forceSwitch)
{
    if(meleeWeapon && forceSwitch)
    {
        if(meleeWeapon->weaponID == newWeaponID) return;
    }
    // Bon testing, on va lui refiler un gun
    ZEVEN_SAFE_DELETE(meleeWeapon);

#ifndef DEDICATED_SERVER
    meleeWeapon = new ClientWeapon(clientVar.weapons[newWeaponID]);
#else
    meleeWeapon = new Weapon(gameVar.weapons[newWeaponID]);
#endif
    meleeWeapon->currentFireDelay = 0;
    meleeWeapon->m_owner = this;

    gameVar.cl_secondaryWeapon = newWeaponID - WEAPON_KNIVES;
}


//
// Ici c'est pour �iter de faire des sons pis toute, vu qu'on est le server
//
void Player::hitSV(Weapon * fromWeapon, Player * from, float damage)
{
    float cdamage = damage;
    if(damage == -1)
    {
        if(gameVar.sv_serverType == 1)
        {
            switch(fromWeapon->weaponID)
            {
            case WEAPON_SMG:
                cdamage = gameVar.sv_smgDamage;
                break;
            case WEAPON_SNIPER:
                cdamage = gameVar.sv_sniperDamage;
                break;
            case WEAPON_SHOTGUN:
                cdamage = gameVar.sv_shottyDamage;
                break;
            case WEAPON_DUAL_MACHINE_GUN:
                cdamage = gameVar.sv_dmgDamage;
                break;
            case WEAPON_CHAIN_GUN:
                cdamage = gameVar.sv_cgDamage;
                break;
            default:
                cdamage = fromWeapon->damage;
            };
        }
        else
        {
            cdamage = fromWeapon->damage;
        }
    }
    if(fromWeapon->weaponID == WEAPON_PHOTON_RIFLE && gameVar.sv_serverType == 1 && from != this)
    {
        CVector3f pos1 = from->weapon->shotFrom;
        CVector3f pos2 = currentCF.position;
        CVector3f dist = pos2 - pos1;
        float distance = dist.length();
        switch(gameVar.sv_photonType)
        {
        case 1://a+(pi/2-arctan(x-b)c)d
            cdamage = cdamage * (gameVar.sv_photonVerticalShift + gameVar.sv_photonDamageCoefficient*(PI / 2 - atanf((distance - gameVar.sv_photonHorizontalShift)*gameVar.sv_photonDistMult)));
            break;
        case 2://(a-d/(x-b)c)
            if(distance == 0)
                distance = 0.000001f;
            cdamage = cdamage * (gameVar.sv_photonVerticalShift + gameVar.sv_photonDamageCoefficient / ((distance - gameVar.sv_photonHorizontalShift)*gameVar.sv_photonDistMult));
            break;
        case 3:
            cdamage = cdamage * (gameVar.sv_photonVerticalShift + gameVar.sv_photonDamageCoefficient / (1 + pow((distance - gameVar.sv_photonHorizontalShift)*gameVar.sv_photonDistMult, 2)));
            break;
        default://constant damage
            cdamage = cdamage * gameVar.sv_photonDamageCoefficient;
            break;
        };
    }
    if(fromWeapon->weaponID == WEAPON_FLAME_THROWER && gameVar.sv_serverType == 1 && from != this)
    {
        cdamage = gameVar.sv_ftDamage;
        CVector3f pos1 = from->weapon->shotFrom;
        CVector3f pos2 = currentCF.position;
        CVector3f dist = pos2 - pos1;
        float distance = dist.length();
        cdamage = (1 - distance / gameVar.sv_ftMaxRange)*cdamage;
    }

    //--- Shield protection!!!
    if(protection > .6f)
    {
        cdamage *= .50f; // Shield power !
    }

    if(immuneTime > 0.3f)
    {
        cdamage = 0.0f;
    }

    if((gameVar.sv_subGameType == SUBGAMETYPE_INSTAGIB) && (fromWeapon->weaponID != WEAPON_GRENADE) && (fromWeapon->weaponID != WEAPON_KNIVES) && (fromWeapon->weaponID != WEAPON_COCKTAIL_MOLOTOV))
    {
        cdamage = life;
    }

    if(status == PLAYER_STATUS_ALIVE)
    {
        // On check si c'est ff, ou reflect, etc
        if(from->teamID == teamID && game->gameType != GAME_TYPE_DM)
        {
            if(gameVar.sv_friendlyFire || from->playerID == playerID || game->gameType == GAME_TYPE_DM)
            {
                if(from != this)
                    from->dmg += (cdamage < life) ? cdamage : life;
                life -= cdamage;
                screenHit += cdamage;

                //--- On doit shooter � au clients
                net_svcl_player_hit playerHit;
                playerHit.damage = life;
                playerHit.playerID = playerID;
                playerHit.fromID = (char)from->playerID;
                playerHit.weaponID = fromWeapon->weaponID;
                playerHit.vel[0] = 0;
                playerHit.vel[1] = 0;
                playerHit.vel[2] = 1;
                bb_serverSend((char*)&playerHit, sizeof(net_svcl_player_hit), NET_SVCL_PLAYER_HIT, 0);
            }
            if(gameVar.sv_reflectedDamage && from->playerID != playerID)
            {
                from->hitSV(fromWeapon, from, cdamage);
            }

            // Oups, on cr�e?
            if(life <= std::numeric_limits<float>::epsilon())
            {
#ifndef DEDICATED_SERVER
                auto cscene = static_cast<ClientScene*>(scene);
                if(cscene->client || (scene->server && gameVar.sv_showKills))
#else
                if(scene->server && gameVar.sv_showKills)
#endif
                {
#ifndef DEDICATED_SERVER
                    CString message = name;
                    switch(teamID)
                    {
                    case PLAYER_TEAM_BLUE:message.insert("\x1", 0); break;
                    case PLAYER_TEAM_RED:message.insert("\x4", 0); break;
                    }
                    message.insert("-----> ", 0);
                    message.insert(clientVar.lang_friendlyFire.s, 0);
                    message.insert("\x8 -----", 0);
                    message.insert(from->name.s, 0);
                    switch(from->teamID)
                    {
                    case PLAYER_TEAM_BLUE:message.insert("\x1", 0); break;
                    case PLAYER_TEAM_RED:message.insert("\x4", 0); break;
                    }
#else
                    CString message("Player id:%d killed player id:%d with weapon id:%d", this->playerID, from->playerID, fromWeapon->weaponID);
#endif
                    console->add(message);
                }
                if(game->isServerGame)
                {
                    // On spawn un pack de vie
                    net_clsv_svcl_player_projectile playerProjectile;
                    playerProjectile.nuzzleID = 0;
                    playerProjectile.playerID = playerID;
                    playerProjectile.position[0] = (short)(currentCF.position[0] * 100);
                    playerProjectile.position[1] = (short)(currentCF.position[1] * 100);
                    playerProjectile.position[2] = (short)(currentCF.position[2] * 100);
                    CVector3f pVel(0, 0, 1);
                    pVel = rotateAboutAxis(pVel, rand(-45.0f, 45.0f), CVector3f(1, 0, 0));
                    pVel = rotateAboutAxis(pVel, rand(-0.0f, 360.0f), CVector3f(0, 0, 1)) * 3;
                    pVel += currentCF.vel * .25f;
                    playerProjectile.vel[0] = (char)(pVel[0] * 10);
                    playerProjectile.vel[1] = (char)(pVel[1] * 10);
                    playerProjectile.vel[2] = (char)(pVel[2] * 10);
                    playerProjectile.weaponID = 0;
                    playerProjectile.projectileType = PROJECTILE_LIFE_PACK;
                    //      playerProjectile.uniqueProjectileID = ++(game->uniqueProjectileID);
                    game->spawnProjectile(playerProjectile, true);
                    bb_serverSend((char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, 0);
                    playerProjectile.nuzzleID = 0;
                    playerProjectile.playerID = playerID;
                    playerProjectile.position[0] = (short)(currentCF.position[0] * 100);
                    playerProjectile.position[1] = (short)(currentCF.position[1] * 100);
                    playerProjectile.position[2] = (short)(currentCF.position[2] * 100);
                    pVel.set(0, 0, 1);
                    pVel = rotateAboutAxis(pVel, rand(-45.0f, 45.0f), CVector3f(1, 0, 0));
                    pVel = rotateAboutAxis(pVel, rand(-0.0f, 360.0f), CVector3f(0, 0, 1)) * 3;
                    pVel += currentCF.vel * .25f;
                    playerProjectile.vel[0] = (char)(pVel[0] * 10);
                    playerProjectile.vel[1] = (char)(pVel[1] * 10);
                    playerProjectile.vel[2] = (char)(pVel[2] * 10);
                    playerProjectile.weaponID = weapon->weaponID;
                    playerProjectile.projectileType = PROJECTILE_DROPED_WEAPON;
                    //  playerProjectile.uniqueProjectileID = ++(game->uniqueProjectileID);
                    game->spawnProjectile(playerProjectile, true);
                    bb_serverSend((char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, 0);
                    for(int i = 0; i < this->nbGrenadeLeft; ++i)
                    {
                        playerProjectile.nuzzleID = 0;
                        playerProjectile.playerID = playerID;
                        playerProjectile.position[0] = (short)(currentCF.position[0] * 100);
                        playerProjectile.position[1] = (short)(currentCF.position[1] * 100);
                        playerProjectile.position[2] = (short)(currentCF.position[2] * 100);
                        pVel.set(0, 0, 1);
                        pVel = rotateAboutAxis(pVel, rand(-45.0f, 45.0f), CVector3f(1, 0, 0));
                        pVel = rotateAboutAxis(pVel, rand(-0.0f, 360.0f), CVector3f(0, 0, 1)) * 3;
                        pVel += currentCF.vel * .25f;
                        playerProjectile.vel[0] = (char)(pVel[0] * 10);
                        playerProjectile.vel[1] = (char)(pVel[1] * 10);
                        playerProjectile.vel[2] = (char)(pVel[2] * 10);
                        playerProjectile.weaponID = 0;
                        playerProjectile.projectileType = PROJECTILE_DROPED_GRENADE;
                        //      playerProjectile.uniqueProjectileID = ++(game->uniqueProjectileID);
                        game->spawnProjectile(playerProjectile, true);
                        bb_serverSend((char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, 0);
                    }
                }
                kill(true);
                if(game->gameType == GAME_TYPE_DM && from != this)
                {
                    if(from->teamID == PLAYER_TEAM_BLUE)
                    {
                        game->blueScore++;
                    }
                    else if(from->teamID == PLAYER_TEAM_RED)
                    {
                        game->redScore++;
                    }
                    if(game->gameType != GAME_TYPE_CTF)
                        from->score++;
                    from->kills++;
                    deaths++;
                }
                else
                {
                    if(game->gameType == GAME_TYPE_TDM)
                    {
                        if(from->teamID == PLAYER_TEAM_BLUE)
                        {
                            game->blueScore--;
                        }
                        else if(from->teamID == PLAYER_TEAM_RED)
                        {
                            game->redScore--;
                        }
                    }
                    from->deaths++;
                    if(game->gameType != GAME_TYPE_CTF)
                        from->score--;
                }
            }
        }
        else
        {
            if(from != this)
                from->dmg += (cdamage < life) ? cdamage : life;
            life -= cdamage;
            screenHit += cdamage;

            //--- On doit shooter � au clients
            net_svcl_player_hit playerHit;
            playerHit.damage = life;
            playerHit.playerID = playerID;
            playerHit.fromID = (char)from->playerID;
            playerHit.weaponID = fromWeapon->weaponID;
            playerHit.vel[0] = 0;
            playerHit.vel[1] = 0;
            playerHit.vel[2] = 1;
            bb_serverSend((char*)&playerHit, sizeof(net_svcl_player_hit), NET_SVCL_PLAYER_HIT, 0);

            // Oups, on cr�e?
            if(life <= std::numeric_limits<float>::epsilon())
            {
#ifndef DEDICATED_SERVER
                auto cscene = static_cast<ClientScene*>(scene);
                if(cscene->client || (scene->server && gameVar.sv_showKills))
#else
                if(scene->server && gameVar.sv_showKills)
#endif
                {
#ifndef DEDICATED_SERVER
                    CString message = name;
                    switch(teamID)
                    {
                    case PLAYER_TEAM_BLUE:message.insert("\x1", 0); break;
                    case PLAYER_TEAM_RED:message.insert("\x4", 0); break;
                    }
                    message.insert("-----> ", 0);
                    message.insert(fromWeapon->weaponName.s, 0);
                    message.insert("\x8 -----", 0);
                    message.insert(from->name.s, 0);
                    switch(from->teamID)
                    {
                    case PLAYER_TEAM_BLUE:message.insert("\x1", 0); break;
                    case PLAYER_TEAM_RED:message.insert("\x4", 0); break;
                    }
#else
                    CString message("Player id:%d killed player id:%d with weapon id:%d", this->playerID, from->playerID, fromWeapon->weaponID);
#endif
                    console->add(message);
                }
                if(game->isServerGame)
                {
                    // On spawn un pack de vie
                    net_clsv_svcl_player_projectile playerProjectile;
                    playerProjectile.nuzzleID = 0;
                    playerProjectile.playerID = playerID;
                    playerProjectile.position[0] = (short)(currentCF.position[0] * 100);
                    playerProjectile.position[1] = (short)(currentCF.position[1] * 100);
                    playerProjectile.position[2] = (short)(currentCF.position[2] * 100);
                    CVector3f pVel(0, 0, 1);
                    pVel = rotateAboutAxis(pVel, rand(-45.0f, 45.0f), CVector3f(1, 0, 0));
                    pVel = rotateAboutAxis(pVel, rand(-0.0f, 360.0f), CVector3f(0, 0, 1)) * 3;
                    pVel += currentCF.vel * .25f;
                    playerProjectile.vel[0] = (char)(pVel[0] * 10);
                    playerProjectile.vel[1] = (char)(pVel[1] * 10);
                    playerProjectile.vel[2] = (char)(pVel[2] * 10);
                    playerProjectile.weaponID = 0;
                    playerProjectile.projectileType = PROJECTILE_LIFE_PACK;
                    //      playerProjectile.uniqueProjectileID = ++(game->uniqueProjectileID);
                    game->spawnProjectile(playerProjectile, true);
                    bb_serverSend((char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, 0);
                    playerProjectile.nuzzleID = 0;
                    playerProjectile.playerID = playerID;
                    playerProjectile.position[0] = (short)(currentCF.position[0] * 100);
                    playerProjectile.position[1] = (short)(currentCF.position[1] * 100);
                    playerProjectile.position[2] = (short)(currentCF.position[2] * 100);
                    pVel.set(0, 0, 1);
                    pVel = rotateAboutAxis(pVel, rand(-45.0f, 45.0f), CVector3f(1, 0, 0));
                    pVel = rotateAboutAxis(pVel, rand(-0.0f, 360.0f), CVector3f(0, 0, 1)) * 3;
                    pVel += currentCF.vel * .25f;
                    playerProjectile.vel[0] = (char)(pVel[0] * 10);
                    playerProjectile.vel[1] = (char)(pVel[1] * 10);
                    playerProjectile.vel[2] = (char)(pVel[2] * 10);
                    playerProjectile.weaponID = weapon->weaponID;
                    playerProjectile.projectileType = PROJECTILE_DROPED_WEAPON;
                    //  playerProjectile.uniqueProjectileID = ++(game->uniqueProjectileID);
                    game->spawnProjectile(playerProjectile, true);
                    bb_serverSend((char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, 0);
                    for(int i = 0; i < this->nbGrenadeLeft; ++i)
                    {
                        playerProjectile.nuzzleID = 0;
                        playerProjectile.playerID = playerID;
                        playerProjectile.position[0] = (short)(currentCF.position[0] * 100);
                        playerProjectile.position[1] = (short)(currentCF.position[1] * 100);
                        playerProjectile.position[2] = (short)(currentCF.position[2] * 100);
                        pVel.set(0, 0, 1);
                        pVel = rotateAboutAxis(pVel, rand(-45.0f, 45.0f), CVector3f(1, 0, 0));
                        pVel = rotateAboutAxis(pVel, rand(-0.0f, 360.0f), CVector3f(0, 0, 1)) * 3;
                        pVel += currentCF.vel * .25f;
                        playerProjectile.vel[0] = (char)(pVel[0] * 10);
                        playerProjectile.vel[1] = (char)(pVel[1] * 10);
                        playerProjectile.vel[2] = (char)(pVel[2] * 10);
                        playerProjectile.weaponID = 0;
                        playerProjectile.projectileType = PROJECTILE_DROPED_GRENADE;
                        //      playerProjectile.uniqueProjectileID = ++(game->uniqueProjectileID);
                        game->spawnProjectile(playerProjectile, true);
                        bb_serverSend((char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, 0);
                    }
                }
                kill(true);
                if(from != this)
                {
                    if(from->teamID == PLAYER_TEAM_BLUE && game->gameType != GAME_TYPE_CTF)
                    {
                        game->blueScore++;
                    }
                    else if(from->teamID == PLAYER_TEAM_RED && game->gameType != GAME_TYPE_CTF)
                    {
                        game->redScore++;
                    }
                    if(game->gameType != GAME_TYPE_CTF)
                        from->score++;
                    from->kills++;
                    deaths++;
                }
                else
                {
                    from->deaths++;
                    if(game->gameType != GAME_TYPE_CTF)
                    {
                        from->kills--;
                        from->score--;
                    }
                }
            }
        }
    }
}



//
// Pour lui donner les info de notre joueur
// � c'est pour lui donner mon nom ex : Daivuk, etc
//
void Player::setThisPlayerInfo()
{
    remoteEntity = false;
#ifndef DEDICATED_SERVER
    isThisPlayer = true;
#endif
    name = gameVar.cl_playerName;
}



//
// Pour setter le coordframe du player
//
void Player::setCoordFrame(net_clsv_svcl_player_coord_frame & playerCoordFrame)
{
    if(playerID != playerCoordFrame.playerID) return; // Wtf c pas le bon player!? (Pas suposer arriver)

    // On check si ce n'est pas un out of order data
    if(netCF1.frameID > playerCoordFrame.frameID) return;

    // Notre dernier keyframe change pour celui qu'on est rendu
    netCF0 = currentCF;
    netCF0.frameID = netCF1.frameID; // On pogne le frameID de l'ancien packet par contre
    cFProgression = 0; // On commence au d�ut de la courbe ;)

    // On donne la nouvelle velocity �notre entity
    currentCF.vel[0] = (float)playerCoordFrame.vel[0] / 10.0f;
    currentCF.vel[1] = (float)playerCoordFrame.vel[1] / 10.0f;
    currentCF.vel[2] = (float)playerCoordFrame.vel[2] / 10.0f;

    currentCF.camPosZ = (float)playerCoordFrame.camPosZ;

    // Son frame ID
    netCF1.frameID = playerCoordFrame.frameID;

    // Va faloir interpoler ici et pr�ire (job's done!)
    netCF1.position[0] = (float)playerCoordFrame.position[0] / 100.0f;
    netCF1.position[1] = (float)playerCoordFrame.position[1] / 100.0f;
    netCF1.position[2] = (float)playerCoordFrame.position[2] / 100.0f;

    // Sa velocity (� aussi va faloir l'interpoler jcr�ben
    netCF1.vel[0] = (char)playerCoordFrame.vel[0] / 10.0f;
    netCF1.vel[1] = (char)playerCoordFrame.vel[1] / 10.0f;
    netCF1.vel[2] = (char)playerCoordFrame.vel[2] / 10.0f;

    // La position de la mouse
    netCF1.mousePosOnMap[0] = (short)playerCoordFrame.mousePos[0] / 100.0f;
    netCF1.mousePosOnMap[1] = (short)playerCoordFrame.mousePos[1] / 100.0f;
    netCF1.mousePosOnMap[2] = (short)playerCoordFrame.mousePos[2] / 100.0f;

    // Si notre frameID �ait �0, on le copie direct
    if(netCF0.frameID == 0)
    {
        netCF0 = netCF1;
    }
}



void Player::updatePing(float delay)
{
    nextPingLogTime -= delay;
    if(nextPingLogTime < 0.0f)
    {
        if(pingLogID >= PING_LOG_SIZE)
            pingLogID = 0;
        pingLog[pingLogID] = ping;
        pingSum += pingLog[pingLogID];
        if(pingLogID + 1 < PING_LOG_SIZE)
            pingSum -= pingLog[pingLogID + 1];
        else
            pingSum -= pingLog[0];
        avgPing = pingSum / PING_LOG_SIZE;
        if(avgPing < 1)
            avgPing = 1;
        nextPingLogTime = pingLogInterval;
        pingLogID++;
    }
}

//
// Update
//
void Player::update(float delay)
{
    updatePing(delay);

    if(teamID != PLAYER_TEAM_SPECTATOR)
        timeIdle += delay;
    else
        timeIdle = 0.0f;

    if(babySitTime >= 0.0f)
        babySitTime -= delay;

    float lenShootShakeDis = shootShakeDis.length();
    if(lenShootShakeDis > 0)
    {
        normalize(shootShakeDis);
        lenShootShakeDis -= delay;
        if(lenShootShakeDis < 0) lenShootShakeDis = 0;
        shootShakeDis *= lenShootShakeDis;
    }

    mfElapsedSinceLastShot += delay;
    mfCFTimer += delay;

    if(protection > 0)
    {
        protection -= delay;
        if(protection < 0) protection = 0;
    }

    if(immuneTime > 0)
    {
        immuneTime -= delay;
        if(immuneTime < 0) immuneTime = 0;
    }

    frameSinceLast++; // For server hacking prevention

    if(fireFrameDelay > 0) fireFrameDelay--;

    lastCF = currentCF; // On garde une copie du dernier coordFrame
    currentCF.frameID++; // Ça ça reste inchangé

    // Rapid-fire hack detection
    secondPassed += delay;
    if(secondPassed > 3.0f && shotCount > 1) {
        // Calculate shots per second, subtracting one shot
        // to allow for lag
        shotsPerSecond = (shotCount - 1) / secondPassed;
        secondPassed = 0;
    }

    if(screenHit > 0)
    {
        screenHit -= delay * .25f;
        if(screenHit < 0) screenHit = 0;
    }

    if(firedShowDelay > 0)
    {
        firedShowDelay -= delay;
        if(firedShowDelay < 0) firedShowDelay = 0;
    }

    if(grenadeDelay > 0)
    {
        grenadeDelay -= delay;
        if(grenadeDelay < 0) grenadeDelay = 0;
    }

    if(meleeDelay > 0)
    {
        meleeDelay -= delay;
        if(meleeDelay < 0) meleeDelay = 0;
    }


    // ca c pour le reload complet du shotty...jme demande ben ou a part ici ca devrait aller......dans shoot c appelé seulement quand ya un mouse HOLD....faq c la meilleur place que j'ai trouver la
    if(weapon)
    {
        if(weapon->fullReload && weapon->weaponID == WEAPON_SHOTGUN)
        {
            if(weapon->shotInc > 0)
            {
                // si ca fait un sizieme du temps alloué pour reloader
                if((int)(weapon->currentFireDelay / 3 * 100) % 17 == 0)
                {
                    weapon->shotInc--;
#ifndef DEDICATED_SERVER
                    auto cscene = static_cast<ClientScene*>(scene);
                    dksPlay3DSound(cscene->client->sfxShotyReload, -1, 5, currentCF.position, 230);
#endif
                }
            }
            else
            {
                weapon->fullReload = false;
            }
        }
    }

#ifndef DEDICATED_SERVER
    // On check si on change de nom (on fait ça uniquement si on est à la fin d'un round (tout le temps en fin de compte))
    if(isThisPlayer)
    {
        if(name != gameVar.cl_playerName)
        {
            // On le clamp à 31 caracter
            if(gameVar.cl_playerName.len() > 31) gameVar.cl_playerName.resize(31);
            net_clsv_svcl_player_change_name playerChangeName;
            memcpy(playerChangeName.playerName, gameVar.cl_playerName.s, gameVar.cl_playerName.len() + 1);
            playerChangeName.playerID = playerID;
            auto cscene = static_cast<ClientScene*>(scene);
            bb_clientSend(cscene->client->uniqueClientID, (char*)&playerChangeName, sizeof(net_clsv_svcl_player_change_name), NET_CLSV_SVCL_PLAYER_CHANGE_NAME);
            name = gameVar.cl_playerName;
        }
    }
#endif

    // On update son gun
    if(weapon) weapon->update(delay);
    if(meleeWeapon) meleeWeapon->update(delay);

    if(status == PLAYER_STATUS_DEAD)
    {
        timeDead += delay;
    }

    timeInServer += delay;

    if(status == PLAYER_STATUS_ALIVE)
    {
        timeAlive += delay;
        timePlayedCurGame += delay;

        if(remoteEntity)
        {
            // Là on va créer une genre d'interpolation
            currentCF.interpolate(cFProgression, netCF0, netCF1, delay);

            // Un ajustement obligatoire (sa hauteur)
            currentCF.position[2] = .25f;
        }
#ifndef DEDICATED_SERVER
        else
        {
            // On déplace avec la velocity
            currentCF.position += currentCF.vel * delay;

            // On ralenti sa vel
            float size = currentCF.vel.length();
            if(size > 0)
            {
                auto cmap = static_cast<ClientMap*>(game->map);
                if(cmap->theme == THEME_SNOW && gameVar.sv_slideOnIce)
                {
                    if(game->map->cells[(int)(currentCF.position[1] - .5f) * game->map->size[0] + (int)(currentCF.position[0] - .5f)].splater[0] > .5f)
                    {
                        size -= delay * 1;
                    }
                    else
                    {
                        size -= delay * 4;
                    }
                }
                else
                {
                    size -= delay * 4;
                }
                if(size < 0) size = 0;
                normalize(currentCF.vel);
                currentCF.vel = currentCF.vel * size;
            }

            // Un ajustement obligatoire
            currentCF.position[2] = .25f;

            // On gère les inputs
            auto cconsole = static_cast<ClientConsole*>(console);
            auto cgame = static_cast<ClientGame*>(game);
            if(isThisPlayer && !cconsole->isActive() && !writting && !cgame->showMenu && !menuManager.root->visible)
            {
                controlIt(delay);
            }

            // On envoit aux autres
            if(isThisPlayer)
            {
                // Bon, on envoit cette position aux autres joueurs, la vitesse d'envoit c'est dépendant de son ping
                sendPosFrame++;
                if(sendPosFrame >= avgPing && sendPosFrame >= gameVar.sv_minSendInterval && status == PLAYER_STATUS_ALIVE)
                {
                    auto cmap = static_cast<ClientMap*>(game->map);
                    // On essait de rester constant
                    net_clsv_svcl_player_coord_frame playerCoordFrame;
                    playerCoordFrame.frameID = currentCF.frameID;
                    //  playerCoordFrame.angle = currentCF.angle;
                    playerCoordFrame.playerID = playerID;
                    playerCoordFrame.position[0] = (short)(currentCF.position[0] * 100);
                    playerCoordFrame.position[1] = (short)(currentCF.position[1] * 100);
                    playerCoordFrame.position[2] = (short)(currentCF.position[2] * 100);
                    playerCoordFrame.vel[0] = (char)(currentCF.vel[0] * 10);
                    playerCoordFrame.vel[1] = (char)(currentCF.vel[1] * 10);
                    playerCoordFrame.vel[2] = (char)(currentCF.vel[2] * 10);
                    playerCoordFrame.mousePos[0] = (short)(currentCF.mousePosOnMap[0] * 100);
                    playerCoordFrame.mousePos[1] = (short)(currentCF.mousePosOnMap[1] * 100);
                    playerCoordFrame.mousePos[2] = (short)(currentCF.mousePosOnMap[2] * 100);
                    playerCoordFrame.babonetID = babonetID;
                    playerCoordFrame.camPosZ = (int)cmap->camPos[2];
                    auto cscene = static_cast<ClientScene*>(scene);
                    bb_clientSend(cscene->client->uniqueClientID, (char*)&playerCoordFrame, sizeof(net_clsv_svcl_player_coord_frame), NET_CLSV_SVCL_PLAYER_COORD_FRAME, NET_UDP);

                    sendPosFrame = 0;
                }
            }

            // On est vivant, donc on a 10sec avant de pouvoir respawner
            timeToSpawn = gameVar.sv_timeToSpawn;
        }
#endif

        // On l'oriente
        {
#ifndef DEDICATED_SERVER
            auto cweapon = static_cast<ClientWeapon*>(weapon);
            CVector3f shotOrigin;
            if(cweapon->nuzzleFlashes.size() > 0)
                shotOrigin = cweapon->nuzzleFlashes[cweapon->firingNuzzle]->position * .005f;
            else
                shotOrigin = currentCF.position;
            shotOrigin = rotateAboutAxis(shotOrigin, currentCF.angle, CVector3f(0, 0, 1)) + currentCF.position;
            CVector3f dirVect = currentCF.mousePosOnMap - shotOrigin;
            CVector3f dirVectAlt = currentCF.mousePosOnMap - currentCF.position;
            if(!gameVar.cl_preciseCursor || weapon->weaponID == WEAPON_DUAL_MACHINE_GUN || dirVectAlt.length() <= 1.5f)
                dirVect = dirVectAlt;
#else
            CVector3f dirVect = currentCF.mousePosOnMap - currentCF.position;
#endif
            dirVect[2] = 0;

            normalize(dirVect);
            float dotWithY = dirVect[1];//dotting with (0,1,0) just gives the y coord
            float dotWithX = dirVect[0];//dotting with (1,0,0) just gives the x coord
            currentCF.angle = acosf(dotWithY)*TO_DEGREE;
            if(dotWithX > 0) currentCF.angle = -currentCF.angle;

            // Bon, on fait rouler la bouboule
            if(lastCF.position != currentCF.position)
            {
                CVector3f mouvement = currentCF.position - lastCF.position;
                float angle = PI * mouvement.length();
                CVector3f right = cross(mouvement, CVector3f(0, 0, 1));
                normalize(right);
                matrix.RotateArbitrary(-angle * TO_DEGREE, right);

                // On la normalize (parce que la boule à rapetisse :|)
                matrix.normalize();
            }
        }
    }
    else if((
        teamID == PLAYER_TEAM_BLUE ||
        teamID == PLAYER_TEAM_RED) &&
        status == PLAYER_STATUS_DEAD && timeToSpawn >= 0)
    {
        timeToSpawn -= delay;
        if(timeToSpawn <= 0)
        {
            // On ne veut pas afficher des négatif ;)
            timeToSpawn = 0;

#ifndef DEDICATED_SERVER
            // Seuleument si c'est notre joueur, sinon on s'en caliss
            if(isThisPlayer)
            {
                // On check si on peut requester le spawn, sauf si on est en s&d (là c le server qui choisi ;))
                auto cscene = static_cast<ClientScene*>(scene);
                if((gameVar.sv_forceRespawn || ((dkiGetState(clientVar.k_shoot) == DKI_DOWN && !cscene->client->showMenu) && !cscene->client->chatting.haveFocus())) && !spawnRequested)
                {
                    if(gameVar.sv_subGameType == SUBGAMETYPE_RANDOMWEAPON)
                    {
                        nextSpawnWeapon = rand(0, WEAPON_FLAME_THROWER + 1);
                        if(rand(-1, 1))
                            nextMeleeWeapon = WEAPON_KNIVES;
                        else
                            nextMeleeWeapon = WEAPON_SHIELD;
                    }

                    // Ici on le call juste une fois, isshh sinon ça sera pas trop bon...
                    // On request to spawn
                    spawnRequested = true;
                    net_clsv_spawn_request spawnRequest;
                    spawnRequest.playerID = playerID;
                    spawnRequest.weaponID = nextSpawnWeapon;
                    spawnRequest.meleeID = nextMeleeWeapon;
                    skin = gameVar.cl_skin;
                    redDecal = gameVar.cl_redDecal;
                    greenDecal = gameVar.cl_greenDecal;
                    blueDecal = gameVar.cl_blueDecal;
                    memcpy(spawnRequest.skin, skin.s, (skin.len() <= 6) ? skin.len() + 1 : 7);
                    spawnRequest.blueDecal[0] = (unsigned char)(blueDecal[0] * 255.0f);
                    spawnRequest.blueDecal[1] = (unsigned char)(blueDecal[1] * 255.0f);
                    spawnRequest.blueDecal[2] = (unsigned char)(blueDecal[2] * 255.0f);
                    spawnRequest.greenDecal[0] = (unsigned char)(greenDecal[0] * 255.0f);
                    spawnRequest.greenDecal[1] = (unsigned char)(greenDecal[1] * 255.0f);
                    spawnRequest.greenDecal[2] = (unsigned char)(greenDecal[2] * 255.0f);
                    spawnRequest.redDecal[0] = (unsigned char)(redDecal[0] * 255.0f);
                    spawnRequest.redDecal[1] = (unsigned char)(redDecal[1] * 255.0f);
                    spawnRequest.redDecal[2] = (unsigned char)(redDecal[2] * 255.0f);
                    auto cscene = static_cast<ClientScene*>(scene);
                    bb_clientSend(cscene->client->uniqueClientID, (char*)&spawnRequest, sizeof(net_clsv_spawn_request), NET_CLSV_SPAWN_REQUEST);
                }
            }
#endif
        }
    }
}


#ifndef DEDICATED_SERVER

void MultOglMatrix(CMatrix3x3f m)
{
    float Matrix[16] = {
        m.s[0], m.s[1], m.s[2], 0,
        m.s[3], m.s[4], m.s[5], 0,
        m.s[6], m.s[7], m.s[8], 0,
        0,    0,    0,    1};

    glMultMatrixf(Matrix);
}


//
// Render
//
void Player::render()
{
    auto cgame = static_cast<ClientGame*>(game);
    if(status == PLAYER_STATUS_ALIVE)
    {
        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT);
        //--- TEMP render path with his bot

            // On render son shadow :)
        if(gameVar.r_playerShadow)
        {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, tex_baboShadow);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_LIGHTING);
            glColor4f(1, 1, 1, .75f);
            glDepthMask(GL_FALSE);
            glPushMatrix();
            glTranslatef(currentCF.position[0] + .1f, currentCF.position[1] - .1f, .025f);
            glBegin(GL_QUADS);
            glTexCoord2f(0, 1);
            glVertex2f(-.5f, .5f);
            glTexCoord2f(0, 0);
            glVertex2f(-.5f, -.5f);
            glTexCoord2f(1, 0);
            glVertex2f(.5f, -.5f);
            glTexCoord2f(1, 1);
            glVertex2f(.5f, .5f);
            glEnd();
            glPopMatrix();
        }
        if((game->gameType != GAME_TYPE_DM) && (gameVar.cl_teamIndicatorType == 1 || (gameVar.cl_teamIndicatorType == 2 && teamID == cgame->thisPlayer->teamID) || (gameVar.cl_teamIndicatorType > 0 && cgame->thisPlayer->teamID == PLAYER_TEAM_SPECTATOR)))
        {
            //--- Get up & right vectors
            float modelview[16];
            glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
            CVector3f up(modelview[1], modelview[5], modelview[9]);
            CVector3f right(modelview[0], modelview[4], modelview[8]);

            float size = gameVar.cl_glowSize;
            CVector3f a, b, c, d;
            a = (right + up) * -size;
            b = (right - up) * size;
            c = (right + up) * size;
            d = (right - up) * -size;
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            if(game->gameType != GAME_TYPE_DM)
            {
                if(teamID == PLAYER_TEAM_RED)
                {
                    glColor3f(1, 0, 0);
                }
                else if(teamID == PLAYER_TEAM_BLUE)
                {
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glColor3f(0, 0, 1);
                }
                else
                {
                    glColor3f(1, 1, 1);
                }
            }
            else
            {
                glColor3f(1, 1, 1);
            }


            glPushMatrix();
            glTranslatef(currentCF.position[0], currentCF.position[1], currentCF.position[2]);
            glBindTexture(GL_TEXTURE_2D, tex_baboHalo);
            glBegin(GL_QUADS);
            glTexCoord2f(0, 1);
            glVertex3f(a[0], a[1], a[2]);
            glTexCoord2f(0, 0);
            glVertex3f(b[0], b[1], b[2]);
            glTexCoord2f(1, 0);
            glVertex3f(c[0], c[1], c[2]);
            glTexCoord2f(1, 1);
            glVertex3f(d[0], d[1], d[2]);
            glEnd();
            glPopMatrix();
        }

        // La boule
        glDepthMask(GL_TRUE);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glEnable(GL_LIGHTING);
        glDepthFunc(GL_LEQUAL);
        glPushMatrix();
        glTranslatef(currentCF.position[0], currentCF.position[1], currentCF.position[2]);
        MultOglMatrix(matrix);
        glEnable(GL_COLOR_MATERIAL);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex_skin);
        glColor3f(1, 1, 1);
        glPolygonMode(GL_FRONT, GL_FILL);
        //gluQuadricTexture(qObj, true);
        //gluSphere(qObj, .25f, 16, 16);
        dkglDrawSphere(0.25f, 16, 16, GL_TRIANGLES);

        //--- On pogne la position sur l'�ran
        CVector3f screenPos = dkglProject(CVector3f(0, 0, 0));
        CVector2i res = dkwGetResolution();
        onScreenPos[0] = (int)screenPos[0];
        onScreenPos[1] = res[1] - (int)screenPos[1];
        glPopMatrix();

        glPushMatrix();
        // Draw the gun
        glPolygonMode(GL_FRONT, GL_FILL);
        glTranslatef(currentCF.position[0], currentCF.position[1], 0);
        glRotatef(currentCF.angle, 0, 0, 1);
        glScalef(0.005f, 0.005f, 0.005f);
        auto cweapon = static_cast<ClientWeapon*>(weapon);
        auto cmeleeWeapon = static_cast<ClientWeapon*>(meleeWeapon);
        if(cweapon) cweapon->render();
        if(cmeleeWeapon)
        {
            cmeleeWeapon->render();
        }
        glRotatef(-90, 0, 0, 1);

        glPopMatrix();

        // Le flag si c'est le cas
        if(game->gameType == GAME_TYPE_CTF)
        {
            if(game->map)
            {
                auto cmap = static_cast<ClientMap*>(game->map);
                if(game->map->flagState[0] == playerID)
                {
                    cmap->flagAngle[0] = currentCF.angle - 90;
                    //game->map->renderFlag(0);
                    //dkoRender(game->map->dko_flag[0], game->map->flagAnim);
                }
                if(game->map->flagState[1] == playerID)
                {
                    cmap->flagAngle[1] = currentCF.angle - 90;
                    //game->map->renderFlag(1);
                    //dkoRender(game->map->dko_flag[1], game->map->flagAnim);
                }
            }
        }
        glPopAttrib();
    }
}

void Player::renderName()
{
    auto cgame = static_cast<ClientGame*>(game);
    auto cscene = static_cast<ClientScene*>(scene);
    if(gameVar.sv_showEnemyTag && cgame->thisPlayer)
    {
        if(!isThisPlayer && teamID != PLAYER_TEAM_SPECTATOR && teamID != cgame->thisPlayer->teamID && game->gameType != GAME_TYPE_DM)
        {
            //--- We don't print it !!!!!!
            return;
        }
    }
    if(status == PLAYER_STATUS_ALIVE)
    {
        CVector2i res = dkwGetResolution();
        if(onScreenPos[0] > 0 && onScreenPos[0] < res[0] &&
            onScreenPos[1] > 0 && onScreenPos[1] < res[1])
        {
            CString showName;
            if(ping > 12 && cscene->client->blink < .25f) showName.insert(CString(" \x5%s", clientVar.lang_laggerC.s).s, 0);
            if(gameVar.r_maxNameLenOverBabo > 0 && name.len() > gameVar.r_maxNameLenOverBabo)
            {
                CString sname(CString("%%.%is[...]", gameVar.r_maxNameLenOverBabo).s, name.s);
                showName.insert(sname.s, 0);
            }
            else
                showName.insert(name.s, 0);
            showName.insert("\x8", 0);
            if(ping > 12 && cscene->client->blink < .25f) showName.insert(CString("\x5%s ", clientVar.lang_laggerC.s).s, 0);
            printCenterText((float)onScreenPos[0], (float)onScreenPos[1] - 28, 28, showName);
        }
    }

    //--- The life of this player
    if(cgame->thisPlayer && status == PLAYER_STATUS_ALIVE)
    {
        if((!isThisPlayer && teamID == cgame->thisPlayer->teamID && game->gameType != GAME_TYPE_DM) ||
            teamID == PLAYER_TEAM_SPECTATOR)
        {
            glColor3f(1, 1, 1);
            renderTexturedQuad(onScreenPos[0] - 15, onScreenPos[1] - 8, 30, 7, 0);
            glColor3f(0, 0, 0);
            renderTexturedQuad(onScreenPos[0] - 14, onScreenPos[1] - 7, 28, 5, 0);
            if(life > .25f || cscene->client->blink < .25f)
            {
                glColor3f(1 - life, life, 0);
                renderTexturedQuad(onScreenPos[0] - 14, onScreenPos[1] - 7, (int)(life*28.0f), 5, 0);
            }
        }
    }
}
#endif



#ifndef DEDICATED_SERVER
//
// Pour updater la skin texture
//
void Player::updateSkin()
{
    CColor3f redDecalT;
    CColor3f greenDecalT;
    CColor3f blueDecalT;
    CString skinT;

    //--- Ici c'est nowhere on update les couleurs lol
    //--- Si � chang�on update � au autres joueur!
    if(isThisPlayer)
    {
        redDecalT = gameVar.cl_redDecal;
        greenDecalT = gameVar.cl_greenDecal;
        blueDecalT = gameVar.cl_blueDecal;
        skinT = gameVar.cl_skin;
    }
    else
    {
        redDecalT = redDecal;
        greenDecalT = greenDecal;
        blueDecalT = blueDecal;
        skinT = skin;
    }

    //--- On reload le skin si � chang�
    if(lastSkin != skinT)
    {
        skin = skinT;
        dktDeleteTexture(&tex_skinOriginal);
        tex_skinOriginal = dktCreateTextureFromFile(CString("main/skins/%s.tga", skin.s).s, DKT_FILTER_BILINEAR);
    }

    redDecal = redDecalT;
    greenDecal = greenDecalT;
    blueDecal = blueDecalT;
    lastSkin = skin;

    //--- Hey oui, on recr�une texture ogl �chaque fois pour chaque babo qui spawn!!!!
    //--- On est en ogl, faq � kick ass MOUHOUHOUHAHAHA
    unsigned char imgData[64 * 32 * 3];
    dktGetTextureData(tex_skinOriginal, imgData);

    //--- Celon son team, on set la couleur du babo en cons�uence
    if((game->gameType != GAME_TYPE_DM) && gameVar.cl_teamIndicatorType == 0)
    {
        switch(teamID)
        {
        case PLAYER_TEAM_RED:
        {
            redDecalT.set(1, .5f, .5f);
            greenDecalT.set(1, .0f, .0f);
            blueDecalT.set(.5f, 0, 0);
            break;
        }
        case PLAYER_TEAM_BLUE:
        {
            redDecalT.set(.5f, .5f, 1);
            greenDecalT.set(0, 0, 1);
            blueDecalT.set(0, 0, .5f);
            break;
        }
        }
    }
    else
    {
        //--- Sinon on prend les couleurs que le gars a mis
        redDecalT = redDecal;
        greenDecalT = greenDecal;
        blueDecalT = blueDecal;
    }

    int i, j, k;
    float r, g, b;
    CColor3f finalColor;
    for(j = 0; j < 32; ++j)
    {
        for(i = 0; i < 64; ++i)
        {
            k = ((j * 64) + i) * 3;
            r = (float)imgData[k + 0] / 255.0f;
            g = (float)imgData[k + 1] / 255.0f;
            b = (float)imgData[k + 2] / 255.0f;
            finalColor = (redDecalT * r + greenDecalT * g + blueDecalT * b) / (r + g + b);
            imgData[k + 0] = (unsigned char)(finalColor[0] * 255.0f);
            imgData[k + 1] = (unsigned char)(finalColor[1] * 255.0f);
            imgData[k + 2] = (unsigned char)(finalColor[2] * 255.0f);
        }
    }

    // update
    dktCreateTextureFromBuffer(&tex_skin, imgData, 64, 32, 3, DKT_FILTER_BILINEAR);
}
#endif


#ifndef DEDICATED_SERVER
//
// Si on se fait toucher !
//
void Player::hit(Weapon * fromWeapon, Player * from, float damage)
{
    auto cgame = static_cast<ClientGame*>(game);
    auto cscene = static_cast<ClientScene*>(scene);
    float cdamage = life - damage; // La diff�ence :) (boom headshot)
    if(damage == -1) cdamage = fromWeapon->damage; // C'est pus possible �

    if(status == PLAYER_STATUS_ALIVE)
    {
        // On check si c'est ff, ou reflect, etc
        if(from->teamID == teamID && game->gameType != GAME_TYPE_DM)
        {
            if(gameVar.sv_friendlyFire || from->playerID == playerID || game->gameType == GAME_TYPE_DM)
            {
                dksPlay3DSound(clientVar.sfx_hit[rand() % 2], -1, 5, currentCF.position, 255);
                cgame->spawnBlood(currentCF.position, cdamage);
                if(from != this)
                {
                    from->dmg += (cdamage < life) ? cdamage : life;
                }
                life -= cdamage;
                screenHit += cdamage;
                if(screenHit > 1.0) screenHit = 1.0;
                if(cdamage > 1) screenHit = 0;
                if(from->isThisPlayer)
                {
                    cscene->client->hitIndicator = 1;
                    dksPlaySound(cscene->client->sfxHit, -1, 250);
                }
            }
            if(gameVar.sv_reflectedDamage && from->playerID != playerID)
            {
                from->hit(fromWeapon, from, damage);
            }

            // Oups, on cr�e?
            if(life <= std::numeric_limits<float>::epsilon())
            {
                if(cscene->client)
                {
                    CString message = /*textColorLess*/(name);
                    switch(teamID)
                    {
                    case PLAYER_TEAM_BLUE:message.insert("{", 0); break;
                    case PLAYER_TEAM_RED:message.insert("}", 0); break;
                    }
                    message.insert("-----> ", 0);
                    message.insert(clientVar.lang_friendlyFire.s, 0);
                    message.insert("\x8 -----", 0);
                    message.insert(/*textColorLess*/(from->name).s, 0);
                    switch(from->teamID)
                    {
                    case PLAYER_TEAM_BLUE:message.insert("{", 0); break;
                    case PLAYER_TEAM_RED:message.insert("}", 0); break;
                    }
                    console->add(message);
                    cscene->client->eventMessages.push_back(TimedMessage(message));
                }
                kill(false);
                if(game->gameType == GAME_TYPE_TDM)
                {
                    if(from->teamID == PLAYER_TEAM_BLUE)
                        game->blueScore--;
                    else if(from->teamID == PLAYER_TEAM_RED)
                        game->redScore--;
                }//If we do a friendly fire kill, reduce score
                if((game->gameType == GAME_TYPE_DM) && from != this)
                {
                    if(from->teamID == PLAYER_TEAM_BLUE)
                    {
                        game->blueScore++;
                    }
                    else if(from->teamID == PLAYER_TEAM_RED)
                    {
                        game->redScore++;
                    }
                    if(game->gameType != GAME_TYPE_CTF)
                        from->score++;
                    from->kills++;
                    deaths++;
                }
                else
                {
                    from->deaths++;
                    if(game->gameType != GAME_TYPE_CTF)
                        from->score--;
                }
            }
        }
        else
        {
            dksPlay3DSound(clientVar.sfx_hit[rand() % 2], -1, 5, currentCF.position, 255);
            cgame->spawnBlood(currentCF.position, cdamage);
            if(from != this)
                from->dmg += (cdamage < life) ? cdamage : life;
            life -= cdamage;
            screenHit += cdamage;
            if(screenHit > 1.0) screenHit = 1.0;
            if(cdamage > 1) screenHit = 0;
            if(from->isThisPlayer)
            {
                cscene->client->hitIndicator = 1;
                dksPlaySound(cscene->client->sfxHit, -1, 250);
            }

            // Oups, on cr�e?
            if(life <= std::numeric_limits<float>::epsilon())
            {
                if(cscene->client)
                {
                    CString message = /*textColorLess*/(name);
                    switch(teamID)
                    {
                    case PLAYER_TEAM_BLUE:message.insert("{", 0); break;
                    case PLAYER_TEAM_RED:message.insert("}", 0); break;
                    }
                    message.insert("-----> ", 0);
                    message.insert(fromWeapon->weaponName.s, 0);
                    message.insert("\x8 -----", 0);
                    message.insert(/*textColorLess*/(from->name).s, 0);
                    switch(from->teamID)
                    {
                    case PLAYER_TEAM_BLUE:message.insert("{", 0); break;
                    case PLAYER_TEAM_RED:message.insert("}", 0); break;
                    }
                    console->add(message);
                    cscene->client->eventMessages.push_back(TimedMessage(message));
                }
                kill(false);
                if(from != this)
                {
                    if(from->teamID == PLAYER_TEAM_BLUE && game->gameType != GAME_TYPE_CTF)
                    {
                        game->blueScore++;
                    }
                    else if(from->teamID == PLAYER_TEAM_RED && game->gameType != GAME_TYPE_CTF)
                    {
                        game->redScore++;
                    }
                    if(game->gameType != GAME_TYPE_CTF)
                        from->score++;
                    from->kills++;
                    deaths++;
                }
                else
                {
                    from->deaths++;
                    if(game->gameType != GAME_TYPE_CTF)
                    {
                        from->kills--;
                        from->score--;
                    }
                }
            }
        }
    }
}
#endif


#ifndef DEDICATED_SERVER
//
// Pour le controller (ça c'est client side only, on ne gère pas le mouvement des autres players comme ça)
//
void Player::controlIt(float delay)
{
    // On gère les inputs

    // Si on est en mode scope (FPS), on tourne la tête avec la mouse
    if(scopeMode)
    {
        CVector2i mouseVel = dkiGetMouseVel();
        float angle = -(float)mouseVel[0];
        angle *= .2f;

        CVector3f dir = currentCF.mousePosOnMap - currentCF.position;
        normalize(dir);
        dir = rotateAboutAxis(dir, angle, CVector3f(0, 0, 1));
        dir *= 10;
        currentCF.mousePosOnMap = currentCF.position + dir;
    }


    float accel = 12.5f;
    auto cmap = static_cast<ClientMap*>(game->map);
    if(game->map->cells[(int)(currentCF.position[1] - .5f) * game->map->size[0] + (int)(currentCF.position[0] - .5f)].splater[0] > .5f && cmap->theme == THEME_SNOW && gameVar.sv_slideOnIce)
    {
        accel = 4.0f;
    }


    // Si on est en mode scope, on se déplace dapres l'orientation local
    if(scopeMode)
    {
        CVector3f front = currentCF.mousePosOnMap - currentCF.position;
        normalize(front);
        CVector3f right = cross(front, CVector3f(0, 0, 1));
        /*if (!(weapon->weaponID == WEAPON_PHOTON_RIFLE && dkiGetState(clientVar.k_shoot)))
        {*/
        if(dkiGetState(clientVar.k_moveUp))
        {
            currentCF.vel += front * delay * accel;
        }
        if(dkiGetState(clientVar.k_moveDown))
        {
            currentCF.vel -= front * delay * accel;
        }
        if(dkiGetState(clientVar.k_moveRight))
        {
            currentCF.vel += right * delay * accel;
        }
        if(dkiGetState(clientVar.k_moveLeft))
        {
            currentCF.vel -= right * delay * accel;
        }
        //}
    }
    else // Sinon absolu
    {
        /*if (!(weapon->weaponID == WEAPON_PHOTON_RIFLE && dkiGetState(clientVar.k_shoot)))
        {*/
        if(gameVar.cl_enableXBox360Controller)
        {
            CVector3f joyVel = dkiGetJoy();
            joyVel[2] = 0;
            joyVel[1] = -joyVel[1];

            if(fabsf(joyVel[0]) < .1f) joyVel[0] = 0;
            if(fabsf(joyVel[1]) < .1f) joyVel[1] = 0;

            joyVel *= 1.5f;
            if(joyVel[0] < -1) joyVel[0] = -1;
            if(joyVel[1] < -1) joyVel[1] = -1;
            if(joyVel[0] > 1) joyVel[0] = 1;
            if(joyVel[1] > 1) joyVel[1] = 1;

            currentCF.vel[0] += joyVel[0] * delay * accel;
            currentCF.vel[1] += joyVel[1] * delay * accel;
        }
        else
        {
            if(dkiGetState(clientVar.k_moveUp))
            {
                currentCF.vel[1] += delay * accel;
            }
            if(dkiGetState(clientVar.k_moveDown))
            {
                currentCF.vel[1] -= delay * accel;
            }
            if(dkiGetState(clientVar.k_moveRight))
            {
                currentCF.vel[0] += delay * accel;
            }
            if(dkiGetState(clientVar.k_moveLeft))
            {
                currentCF.vel[0] -= delay * accel;
            }
        }
        //}
    }

    // Si on tire
    if(!dkiGetState(clientVar.k_shoot))
    {
        if(weapon)
        {
            weapon->charge = 0;
        }
    }
    if(dkiGetState(clientVar.k_shoot) == DKI_DOWN) initedMouseClic = true;
    if(dkiGetState(clientVar.k_shoot) && initedMouseClic)
    {
        if(weapon && grenadeDelay == 0 && meleeDelay == 0)
        {
            firedShowDelay = 2; // Ça c'est le ping sur la map qu'on voit quand L'autre tire

            //--- Est-ce qu'on est sniper et en scope mode?
            if(weapon->weaponID == WEAPON_SNIPER && cmap->camPos[2] >= 10)
            {
                //--- On shoot une deuxième fois pour faire plus de damage en scope mode
                weapon->nbShot = 3;
            }

            auto cweapon = static_cast<ClientWeapon*>(weapon);
            cweapon->shoot(this);

            if(weapon->weaponID == WEAPON_SNIPER)
            {
                //--- On shoot une deuxième fois pour faire plus de damage en scope mode
                weapon->nbShot = 2;
            }
        }
    }

    // SECONDARY FIRE (Melee weapon)
    if(dkiGetState(clientVar.k_melee) && grenadeDelay == 0 && meleeDelay == 0 && gameVar.sv_enableSecondary)
    {
        if(meleeWeapon && grenadeDelay == 0)
        {
            firedShowDelay = 2; // Ça c'est le ping sur la map qu'on voit quand L'autre tire

            //--- On shoot ça
            net_clsv_svcl_player_shoot_melee playerShootMelee;
            playerShootMelee.playerID = playerID;
            auto cscene = static_cast<ClientScene*>(scene);
            bb_clientSend(cscene->client->uniqueClientID, (char*)&playerShootMelee, sizeof(net_clsv_svcl_player_shoot_melee), NET_CLSV_SVCL_PLAYER_SHOOT_MELEE);

            //      meleeWeapon->shoot(this);

            meleeDelay = meleeWeapon->fireDelay;
        }
    }

    // On lance une nade
    if(dkiGetState(clientVar.k_throwGrenade) == DKI_DOWN && grenadeDelay == 0 && nbGrenadeLeft > 0 && meleeDelay == 0)
    {
        if(weapon)
        {
            if(weapon->currentFireDelay <= 0) // On n'est pas entrein de shoter avec un autre gun?
            {
                lastShootWasNade = true;
                nbGrenadeLeft--;
                grenadeDelay = clientVar.weapons[WEAPON_GRENADE]->fireDelay;
                // On pitch ça
                clientVar.weapons[WEAPON_GRENADE]->shoot(this);
                clientVar.weapons[WEAPON_GRENADE]->currentFireDelay = 0; // Il n'y a pas d'update sur ce gun
            }
        }
    }

    // On lance un cocktail molotov
    if(dkiGetState(clientVar.k_throwMolotov) == DKI_DOWN && grenadeDelay == 0 && nbMolotovLeft > 0 && gameVar.sv_enableMolotov)
    {
        if(weapon)
        {
            if(weapon->currentFireDelay <= 0) // On n'est pas entrein de shoter avec un autre gun?
            {
                lastShootWasNade = false;
                nbMolotovLeft--;
                grenadeDelay = clientVar.weapons[WEAPON_COCKTAIL_MOLOTOV]->fireDelay;
                // On pitch ça
                clientVar.weapons[WEAPON_COCKTAIL_MOLOTOV]->shoot(this);
                clientVar.weapons[WEAPON_COCKTAIL_MOLOTOV]->currentFireDelay = 0; // Il n'y a pas d'update sur ce gun
            }
        }
    }

    // On switch en scope mode (sniper only) (cétait juste un test à chier ça)
/*  if (dkiGetState(DKI_MOUSE_BUTTON2) == DKI_DOWN)
    {
        scopeMode = !scopeMode;
    }*/

    // On pickup un item par terre
    if(dkiGetState(clientVar.k_pickUp) == DKI_DOWN)
    {
        net_clsv_pickup_request pickupRequest;
        pickupRequest.playerID = playerID;
        auto cscene = static_cast<ClientScene*>(scene);
        bb_clientSend(cscene->client->uniqueClientID, (char*)&pickupRequest, sizeof(net_clsv_pickup_request), NET_CLSV_PICKUP_REQUEST);
    }

    // On clamp sa vel /* Upgrade, faster ! haha */
    float size = currentCF.vel.length();
    if(size > 3.25f)
    {
        normalize(currentCF.vel);
        currentCF.vel = currentCF.vel * 3.25f;
    }
}
#endif
