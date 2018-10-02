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
    timeToSpawn = gameVar.sv_timeToSpawn;
    remoteEntity = true;
    cFProgression = 0;
    weapon = 0;
    meleeWeapon = 0;
    nextSpawnWeapon = gameVar.cl_primaryWeapon;//WEAPON_SMG;
    nextMeleeWeapon = gameVar.cl_secondaryWeapon + WEAPON_KNIVES;//WEAPON_KNIVES;

    firedShowDelay = 0;
    deadSince = 0;

    screenHit = 0;
    grenadeDelay = 0;
    nbGrenadeLeft = 2; // On commence toujours avec 2 grenade
    nbMolotovLeft = 1; // On commence toujours avec 1 molotov
    currentPingFrame = 0;
    connectionInterrupted = false;
    spawnRequested = false;

    //--- on load un skin par default
    skin = "skin10";
}



//
// Destructeur
//
Player::~Player()
{
    //--- Est-ce qu'on est server et que ce player pocclientVar.dkpp_e le flag???
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
// Pour le forcer clientVar.dkpp_crever (suposons quil change de team)
//
void Player::kill(bool silenceDeath)
{
    status = PLAYER_STATUS_DEAD;
    deadSince = 0;

    // Si il avait le flag, on le laisse tomber
    for(int i = 0; i < 2; ++i)
    {
        if(game->map->flagState[i] == playerID)
        {
            game->map->flagState[i] = -1; // Le server va nous communiquer la position du flag exacte
            game->map->flagPos[i] = currentCF.position;
            game->map->flagPos[i][2] = 0;
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
// Pour spawner un joueur yclientVar.dkpp_
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
}



//
// Pour remettre ses stats clientVar.dkpp_0
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

void Player::switchWeapon(int newWeaponID, bool forceSwitch)
{
    if(weapon && forceSwitch)
    {
        if(weapon->weaponID == newWeaponID) return;
    }
    // Bon testing, on va lui refiler un gun
    ZEVEN_SAFE_DELETE(weapon);
    weapon = new Weapon(gameVar.weapons[newWeaponID]);
    weapon->currentFireDelay = 1; // On a une 1sec de delait quand on switch de gun
    weapon->m_owner = this;

    gameVar.cl_primaryWeapon = newWeaponID;

    // Reset rapid-fire hack check
    shotCount = 0;
    shotsPerSecond = 0;
}

void Player::switchMeleeWeapon(int newWeaponID, bool forceSwitch)
{
    if(meleeWeapon && forceSwitch)
    {
        if(meleeWeapon->weaponID == newWeaponID) return;
    }
    // Bon testing, on va lui refiler un gun
    ZEVEN_SAFE_DELETE(meleeWeapon);

    meleeWeapon = new Weapon(gameVar.weapons[newWeaponID]);
    meleeWeapon->currentFireDelay = 0;
    meleeWeapon->m_owner = this;

    gameVar.cl_secondaryWeapon = newWeaponID - WEAPON_KNIVES;
}

void Player::onDeath(Player* from, Weapon * fromWeapon, bool friendlyFire)
{
    if(scene->server && gameVar.sv_showKills)
    {
        CString message("Player id:%d killed player id:%d with weapon id:%d", this->playerID, from->playerID, fromWeapon->weaponID);
        console->add(message);
    }
}

//
// Ici c'est pour clientVar.dkpp_iter de faire des sons pis toute, vu qu'on est le server
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

                //--- On doit shooter clientVar.dkpp_ au clients
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

            if(life <= std::numeric_limits<float>::epsilon())
            {
                onDeath(from, fromWeapon, true);
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

            //--- On doit shooter clientVar.dkpp_ au clients
            net_svcl_player_hit playerHit;
            playerHit.damage = life;
            playerHit.playerID = playerID;
            playerHit.fromID = (char)from->playerID;
            playerHit.weaponID = fromWeapon->weaponID;
            playerHit.vel[0] = 0;
            playerHit.vel[1] = 0;
            playerHit.vel[2] = 1;
            bb_serverSend((char*)&playerHit, sizeof(net_svcl_player_hit), NET_SVCL_PLAYER_HIT, 0);

            if(life <= std::numeric_limits<float>::epsilon())
            {
                onDeath(from, fromWeapon, false);
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
// clientVar.dkpp_ c'est pour lui donner mon nom ex : Daivuk, etc
//
void Player::setThisPlayerInfo()
{
    remoteEntity = false;
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
    cFProgression = 0; // On commence au dclientVar.dkpp_ut de la courbe ;)

    // On donne la nouvelle velocity clientVar.dkpp_notre entity
    currentCF.vel[0] = (float)playerCoordFrame.vel[0] / 10.0f;
    currentCF.vel[1] = (float)playerCoordFrame.vel[1] / 10.0f;
    currentCF.vel[2] = (float)playerCoordFrame.vel[2] / 10.0f;

    currentCF.camPosZ = (float)playerCoordFrame.camPosZ;

    // Son frame ID
    netCF1.frameID = playerCoordFrame.frameID;

    // Va faloir interpoler ici et prclientVar.dkpp_ire (job's done!)
    netCF1.position[0] = (float)playerCoordFrame.position[0] / 100.0f;
    netCF1.position[1] = (float)playerCoordFrame.position[1] / 100.0f;
    netCF1.position[2] = (float)playerCoordFrame.position[2] / 100.0f;

    // Sa velocity (clientVar.dkpp_ aussi va faloir l'interpoler jcrclientVar.dkpp_ben
    netCF1.vel[0] = (char)playerCoordFrame.vel[0] / 10.0f;
    netCF1.vel[1] = (char)playerCoordFrame.vel[1] / 10.0f;
    netCF1.vel[2] = (char)playerCoordFrame.vel[2] / 10.0f;

    // La position de la mouse
    netCF1.mousePosOnMap[0] = (short)playerCoordFrame.mousePos[0] / 100.0f;
    netCF1.mousePosOnMap[1] = (short)playerCoordFrame.mousePos[1] / 100.0f;
    netCF1.mousePosOnMap[2] = (short)playerCoordFrame.mousePos[2] / 100.0f;

    // Si notre frameID clientVar.dkpp_ait clientVar.dkpp_0, on le copie direct
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
                    onShotgunReload();
                }
            }
            else
            {
                weapon->fullReload = false;
            }
        }
    }

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

        // On l'oriente
        {
            CVector3f dirVect = currentCF.mousePosOnMap - currentCF.position;
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
            onSpawnRequest();
        }
    }
}
