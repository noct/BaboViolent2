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
#include "Game.h"
#include "netPacket.h"
#include "Console.h"
#include "GameVar.h"
#include "Server.h"
#include "Scene.h"
#include <time.h>
#include <algorithm>

extern Scene * scene;

Game::SVoting::SVoting()
{
    //nbActivePlayers = 0;
    voted = false;
    remaining = 0;
    votingInProgress = false;
    votingResults[0] = 0;
    votingResults[1] = 0;
}

void Game::SVoting::castVote(Game* game, const net_clsv_svcl_vote_request & voteRequest)
{
    //nbActivePlayers = 0;
    activePlayersID.clear();
    for(int i = 0; i < MAX_PLAYER; ++i)
    {
        if(game->players[i])
        {
            if((game->players[i]->teamID != PLAYER_TEAM_AUTO_ASSIGN) &&
                (game->players[i]->teamID != PLAYER_TEAM_SPECTATOR))
            {
                activePlayersID.push_back(game->players[i]->babonetID);
                game->players[i]->voted = false;
            }
            else
                game->players[i]->voted = true;
        }
    }
#ifndef DEDICATED_SERVER
    auto cgame = static_cast<ClientGame*>(game);
    if(cgame->thisPlayer && (cgame->thisPlayer->teamID != PLAYER_TEAM_AUTO_ASSIGN) &&
        (cgame->thisPlayer->teamID != PLAYER_TEAM_SPECTATOR))
        voted = false;
    else
        voted = true;
#else
    voted = true;
#endif
    votingWhat = voteRequest.vote;
    votingResults[0] = 0;
    votingResults[1] = 0;
    remaining = 30; // 30 sec to vote
    votingInProgress = true;
}

bool Game::SVoting::update(float delay)
{
    if(votingInProgress)
    {
        remaining -= delay;
        if(remaining <= 0)
        {
            remaining = 0;
            votingInProgress = false;
            return true;
        }

        if(votingResults[0] > (int)activePlayersID.size() / 2 ||
            votingResults[1] > (int)activePlayersID.size() / 2 ||
            votingResults[0] + votingResults[1] >= (int)activePlayersID.size())
        {
            votingInProgress = false;
            return true;
        }
    }
    return false;
}

//
// Constructeur
//
Game::Game(CString pMapName)
{
    //  uniqueProjectileID = 0;
    mapName = pMapName;

    players = new Player*[MAX_PLAYER];
    for(int i = 0; i < MAX_PLAYER; ++i) players[i] = 0;

    isServerGame = false;
    roundState = GAME_PLAYING;
    bombPlanted = false;
    bombTime = 0;

    map = 0;

    // Le type de jeu
    gameType = gameVar.sv_gameType;
    spawnType = gameVar.sv_spawnType;
    subGameType = gameVar.sv_subGameType;

    blueScore = 0;
    redScore = 0;
    blueWin = 0;
    redWin = 0;
    gameTimeLeft = gameVar.sv_gameTimeLimit;
    roundTimeLeft = gameVar.sv_roundTimeLimit;

    teamApproveAll[PLAYER_TEAM_RED] = true;
    teamApproveAll[PLAYER_TEAM_BLUE] = true;

    // Ça ça va être utilise quand on va équilibrer les teams
    bluePing = 0;
    redPing = 0;
    ffaPing = 0;
    spectatorPing = 0;

    UpdateProSettings();
}

//
// Destructeur
//
Game::~Game()
{
    int i;
    ZEVEN_SAFE_DELETE(map);
    for(i = 0; i < MAX_PLAYER; ++i) ZEVEN_SAFE_DELETE(players[i]);
    ZEVEN_SAFE_DELETE_ARRAY(players);
    ZEVEN_DELETE_VECTOR(projectiles, i);
}

//
// Pour starter un new round
//
void Game::resetGameType(int pGameType)
{
    // Server ony
    gameType = pGameType;

    spawnType = gameVar.sv_spawnType;
    subGameType = gameVar.sv_subGameType;

    blueScore = 0;
    redScore = 0;
    blueWin = 0;
    redWin = 0;
    gameTimeLeft = gameVar.sv_gameTimeLimit;
    roundTimeLeft = gameVar.sv_roundTimeLimit;
    resetRound();

    // On dit aux autres d'en faire autant
    if(isServerGame)
    {
        net_svcl_change_game_type changeGameType;
        changeGameType.newGameType = pGameType;
        bb_serverSend((char*)&changeGameType, sizeof(net_svcl_change_game_type), NET_SVCL_CHANGE_GAME_TYPE, 0);
    }

    // On remet les score des players à 0
    for(int i = 0; i < MAX_PLAYER; ++i)
    {
        if(players[i])
        {
            players[i]->reinit();
        }
    }

}

void Game::UpdateProSettings()
{
    // Adjust weapons depending on game var
    if(gameVar.sv_serverType == SERVER_TYPE_PRO)
    {
        gameVar.weapons[WEAPON_SHIELD]->fireDelay = 2.5;
        gameVar.weapons[WEAPON_CHAIN_GUN]->reculVel = 1.0f;
    }
    else
    {
        gameVar.weapons[WEAPON_SHIELD]->fireDelay = 3.0;
        gameVar.weapons[WEAPON_CHAIN_GUN]->reculVel = 2.0f;
    }
}

//
// Pour starter un new round
//
void Game::resetRound()
{
    roundTimeLeft = gameVar.sv_roundTimeLimit;

    map->flagState[0] = -2;
    map->flagState[1] = -2;

    bombPlanted = false;
    bombTime = gameVar.sv_bombTime; // Seconds before kaboom

    // On clair les trails n Stuff
    int i;
    ZEVEN_DELETE_VECTOR(projectiles, i);

    // On respawn tout les players (le server va décider de tout ça)
    for(i = 0; i < MAX_PLAYER; ++i)
    {
        if(players[i])
        {
            players[i]->kill(true);
            players[i]->timeToSpawn = 0;
            players[i]->timePlayedCurGame = 0;
        }
    }
}

//
// Pour lui dire : ok, tu peux créer la map
//
void Game::createMap()
{
    ZEVEN_SAFE_DELETE(map);
    srand((unsigned int)time(0));//mapSeed); // Fuck le mapSeed, on l'utilise pus

    if(scene->server) {
        //--- Check first is that map exist.
        CString filename("main/maps/%s.bvm", mapName.s);
        FILE* fic = fopen(filename.s, "rb");
        if(!fic)
        {
            console->add(CString("\x9> Warning, map not found %s", mapName.s));
            return;
        }
        else
        {
            fclose(fic);
        }
    }

    map = new Map(mapName, this, 0);

    if(!map->isValid)
    {
        console->add("\x4> Invalid map");
        ZEVEN_SAFE_DELETE(map);
        if(scene->server)
        {
            scene->server->needToShutDown = true;
            scene->server->isRunning = false;
        }
        return;
    }
    dkcJumpToFrame(scene->ctx, 0);
}

//--- He oui, une fonction GLOBAL !!!!!!!!!!!!
int SelectToAvailableWeapon()
{
    if(gameVar.sv_enableSMG) return WEAPON_SMG;
    if(gameVar.sv_enableShotgun) return WEAPON_SHOTGUN;
    if(gameVar.sv_enableSniper) return WEAPON_SNIPER;
    if(gameVar.sv_enableDualMachineGun) return WEAPON_DUAL_MACHINE_GUN;
    if(gameVar.sv_enableChainGun) return WEAPON_CHAIN_GUN;
    if(gameVar.sv_enableBazooka) return WEAPON_BAZOOKA;
    if(gameVar.sv_enablePhotonRifle) return WEAPON_PHOTON_RIFLE;
    if(gameVar.sv_enableFlameThrower) return WEAPON_FLAME_THROWER;

    return WEAPON_SMG;
}

//--- He oui, une fonction GLOBAL !!!!!!!!!!!!
int SelectToAvailableMeleeWeapon()
{
    if(gameVar.sv_enableKnives)    return WEAPON_KNIVES;
    if(gameVar.sv_enableShield)    return WEAPON_SHIELD;
    return WEAPON_KNIVES;
}

//
// Update
//
void Game::update(float delay)
{
    int i;
    if(voting.votingInProgress)
    {
        if(voting.update(delay))
        {
            if(isServerGame)
            {
                //--- Voting complete, check the result
                //    In order to the vote to pass, more than
                //    50% of ALL the players should have voted YES.
                /*voting.nbActivePlayers = 0;
                for (int i=0;i<MAX_PLAYER;++i)
                {
                    if ( (players[i]) && (players[i]->teamID != PLAYER_TEAM_AUTO_ASSIGN) &&
                        (players[i]->teamID != PLAYER_TEAM_AUTO_ASSIGN) )
                        ++voting.nbActivePlayers;
                }*/

                net_svcl_vote_result voteResult;
                voteResult.passed = (voting.votingResults[0] > (int)voting.activePlayersID.size() / 2);
                bb_serverSend((char*)(&voteResult), sizeof(net_svcl_vote_result), NET_SVCL_VOTE_RESULT);
                if(voteResult.passed)
                {
                    //--- Vote passed!!!
                    console->sendCommand(voting.votingWhat);
                }
                else
                {
                    //--- Vote failed...
                }
            }
        }
    }

    if(roundState == GAME_PLAYING)
    {
        // On update les players
        for(int i = 0; i < MAX_PLAYER; ++i)
        {
            if(players[i])
            {
                players[i]->update(delay);

                if(players[i]->incShot > 0)
                {
                    players[i]->incShot--;
                    if(players[i]->incShot % 3 == 0)
                    {
                        // On test premièrement si on touche un autre joueur!
                        Player * hitPlayer = 0;
                        CVector3f p3 = players[i]->p2;
                        for(int j = 0; j < MAX_PLAYER; j++)
                        {
                            if(players[j])
                            {
                                if(j != i)
                                {
                                    if(players[j]->status == PLAYER_STATUS_ALIVE && (players[j]->teamID != players[i]->teamID || gameType == GAME_TYPE_DM || gameVar.sv_friendlyFire || gameVar.sv_reflectedDamage))
                                    {
                                        // Ray to sphere test
                                        if(segmentToSphere(players[i]->p1, p3, players[j]->currentCF.position, .35f))
                                        {
                                            hitPlayer = players[j];
                                            p3 = players[i]->p2; // Full length

                                            // On décrémente sa vie
                                            hitPlayer->hitSV(gameVar.weapons[WEAPON_PHOTON_RIFLE], players[i], gameVar.weapons[WEAPON_PHOTON_RIFLE]->damage / 2.0f);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                //--- Update les guns
                if(players[i]->weapon)
                {
                    if(players[i]->weapon->weaponID == WEAPON_SMG)
                    {
                        if(!gameVar.sv_enableSMG)
                        {
                            players[i]->switchWeapon(SelectToAvailableWeapon(), true);
                        }
                    }
                    if(players[i]->weapon->weaponID == WEAPON_SHOTGUN)
                    {
                        if(!gameVar.sv_enableShotgun)
                        {
                            players[i]->switchWeapon(SelectToAvailableWeapon(), true);
                        }
                    }
                    if(players[i]->weapon->weaponID == WEAPON_SNIPER)
                    {
                        if(!gameVar.sv_enableSniper/* || map->size[0] * map->size[1] <= 512*/)
                        {
                            players[i]->switchWeapon(SelectToAvailableWeapon(), true);
                        }
                    }
                    if(players[i]->weapon->weaponID == WEAPON_DUAL_MACHINE_GUN)
                    {
                        if(!gameVar.sv_enableDualMachineGun)
                        {
                            players[i]->switchWeapon(SelectToAvailableWeapon(), true);
                        }
                    }
                    if(players[i]->weapon->weaponID == WEAPON_CHAIN_GUN)
                    {
                        if(!gameVar.sv_enableChainGun)
                        {
                            players[i]->switchWeapon(SelectToAvailableWeapon(), true);
                        }
                    }
                    if(players[i]->weapon->weaponID == WEAPON_BAZOOKA)
                    {
                        if(!gameVar.sv_enableBazooka)
                        {
                            players[i]->switchWeapon(SelectToAvailableWeapon(), true);
                        }
                    }
                    if(players[i]->weapon->weaponID == WEAPON_PHOTON_RIFLE)
                    {
                        if(!gameVar.sv_enablePhotonRifle)
                        {
                            players[i]->switchWeapon(SelectToAvailableWeapon(), true);
                        }
                    }
                    if(players[i]->weapon->weaponID == WEAPON_FLAME_THROWER)
                    {
                        if(!gameVar.sv_enableFlameThrower)
                        {
                            players[i]->switchWeapon(SelectToAvailableWeapon(), true);
                        }
                    }
                }

                //--- Update les melee
                if(players[i]->meleeWeapon)
                {
                    if(!gameVar.sv_enableSecondary)
                    {
                        players[i]->switchMeleeWeapon(SelectToAvailableMeleeWeapon(), true);
                    }
                    else
                    {
                        if(players[i]->meleeWeapon->weaponID == WEAPON_KNIVES)
                        {
                            if(!gameVar.sv_enableKnives)
                            {
                                players[i]->switchMeleeWeapon(SelectToAvailableMeleeWeapon(), true);
                            }
                        }
                        if(players[i]->meleeWeapon->weaponID == WEAPON_SHIELD)
                        {
                            if(!gameVar.sv_enableShield)
                            {
                                players[i]->switchMeleeWeapon(SelectToAvailableMeleeWeapon(), true);
                            }
                        }
                    }
                }
            }
        }
    }

    // On update les projectiles
    for(int i = 0; i < (int)projectiles.size(); ++i)
    {
        Projectile * projectile = projectiles[i];
        projectile->update(delay, map);
        projectile->projectileID = (short)i; // On l'update toujours
        if(projectile->needToBeDeleted)
        {
            if(!projectile->reallyNeedToBeDeleted)
            {
                projectile->reallyNeedToBeDeleted = true;
                continue;
            }
            projectiles.erase(projectiles.begin() + i);
            net_svcl_delete_projectile deleteProjectile;
            deleteProjectile.projectileID = projectile->uniqueID;
            bb_serverSend((char*)&deleteProjectile, sizeof(net_svcl_delete_projectile), NET_SVCL_DELETE_PROJECTILE, 0);
            i--;
            delete projectile;
        }

    }
}

//
// pour donner un team à un player
//
int Game::assignPlayerTeam(int playerID, char teamRequested, Client * client)
{
    if(players[playerID])
    {
        players[playerID]->spawnSlot = -1;

        if(teamRequested == PLAYER_TEAM_AUTO_ASSIGN)
        {
            // On va équilibrer les team :)
            int blueCount = 0;
            int redCount = 0;
            for(int i = 0; i < MAX_PLAYER; ++i)
            {
                if(players[i] && i != playerID)
                {
                    switch(players[i]->teamID)
                    {
                    case PLAYER_TEAM_BLUE:
                        blueCount++;
                        break;
                    case PLAYER_TEAM_RED:
                        redCount++;
                        break;
                    }
                }
            }

            // On le met dans le team qui a le moins de joueur
            if(redCount > blueCount)
            {
                teamRequested = PLAYER_TEAM_BLUE;
            }
            else if(redCount < blueCount)
            {
                teamRequested = PLAYER_TEAM_RED;
            }
            else
            {
                // choose the losing team
                if(blueScore < redScore)
                {
                    teamRequested = PLAYER_TEAM_BLUE;
                }
                else if(blueScore > redScore)
                {
                    teamRequested = PLAYER_TEAM_RED;
                }
                else
                {
                    // random is bad! but there is nothing else we could do...
                    if(rand() % 2 == 0) teamRequested = PLAYER_TEAM_BLUE;
                    else teamRequested = PLAYER_TEAM_RED;
                }
            }
        }

        if(isApproved(players[playerID]->userID, teamRequested) == false)
            teamRequested = PLAYER_TEAM_SPECTATOR;

        if(players[playerID]->teamID != teamRequested)
        {
            // On le tue seuleument si on change de team
            players[playerID]->kill(true);
            players[playerID]->timeToSpawn = gameVar.sv_timeToSpawn;

            // On lui donne le nouveau team
            players[playerID]->teamID = teamRequested;

            // On print dans console
            switch(players[playerID]->teamID)
            {
            case PLAYER_TEAM_SPECTATOR:
                console->add(CString("%s\x8 goes spectator ID:%i", players[playerID]->name.s, playerID));
#ifndef DEDICATED_SERVER
                if(client) client->eventMessages.push_back(CString(gameVar.lang_goesSpectator.s, players[playerID]->name.s));
#endif
                break;
            case PLAYER_TEAM_BLUE:
                console->add(CString("%s\x1 joins blue team ID:%i", players[playerID]->name.s, playerID));
#ifndef DEDICATED_SERVER
                if(client) client->eventMessages.push_back(CString(gameVar.lang_joinBlueTeamP.s, players[playerID]->name.s));
#endif
                break;
            case PLAYER_TEAM_RED:
                console->add(CString("%s\x4 joins red team ID:%i", players[playerID]->name.s, playerID));
#ifndef DEDICATED_SERVER
                if(client) client->eventMessages.push_back(CString(gameVar.lang_joinRedTeamP.s, players[playerID]->name.s));
#endif
                break;
            }
        }

        // Just in case
        players[playerID]->teamID = teamRequested;

        return teamRequested;
    }
    else
    {
        return PLAYER_TEAM_SPECTATOR; // Pas suposer en arriver là!
    }
}

// let the player join selected team
bool Game::approvePlayer(int userID, char team)
{
    if(userID <= 0 || (team != PLAYER_TEAM_RED && team != PLAYER_TEAM_BLUE))
        return false;

    approvedPlayers[team].push_back(userID);
    return true;
}

void Game::rejectPlayer(int userID)
{
    for(int i = 0; i < (int)approvedPlayers.size(); i++)
    {
        std::vector<int>::iterator it = std::find(approvedPlayers[i].begin(),
            approvedPlayers[i].end(), userID);
        if(it != approvedPlayers[i].end())
        {
            approvedPlayers[i].erase(it);
            continue;
        }
    }
}

void Game::rejectPlayer(int userID, char team)
{
    if(team != PLAYER_TEAM_RED && team != PLAYER_TEAM_BLUE)
        return;
    std::vector<int>::iterator it = std::find(approvedPlayers[team].begin(),
        approvedPlayers[team].end(), userID);
    if(it != approvedPlayers[team].end())
        approvedPlayers[team].erase(it);
}

// approve all players to join selected team
void Game::approveAll(int team)
{
    if(team != PLAYER_TEAM_RED && team != PLAYER_TEAM_BLUE)
        return;
    approvedPlayers[team].clear();
    teamApproveAll[team] = true;
}

// approve all players to join
void Game::approveAll()
{
    ApprovedPlayers::iterator it = approvedPlayers.begin();
    for(; it != approvedPlayers.end(); it++)
    {
        it->second.clear();
        teamApproveAll[it->first] = true;
    }
}

bool Game::isApproved(int userID, int team)
{
    if(team == PLAYER_TEAM_SPECTATOR)
        return true;
    if(team != PLAYER_TEAM_RED && team != PLAYER_TEAM_BLUE)
        return false;

    std::vector<int>::iterator it = std::find(approvedPlayers[team].begin(),
        approvedPlayers[team].end(), userID);
    if(teamApproveAll[team] == true || it != approvedPlayers[team].end())
        return true;
    else
        return false;
}

void Game::rejectAllPlayers()
{
    approveAll();
    for(int i = 0; i < (int)teamApproveAll.size(); i++)
        teamApproveAll[i] = false;
}

//
// Pour savoir s'il y a un joueur dans le radius
//
Player * Game::playerInRadius(CVector3f position, float radius, int ignore)
{
    for(int i = 0; i < MAX_PLAYER; ++i)
    {
        if(players[i])
        {
            if(players[i]->status == PLAYER_STATUS_ALIVE && i != ignore)
            {
                if(distanceSquared(position, players[i]->currentCF.position) <= (radius + .25f)*(radius + .25f)) return players[i];
            }
        }
    }
    return 0;
}

//
// Quand un client shot, mais que le server le vérifie puis le shoot aux autres joueurs
//
void Game::shootSV(net_clsv_player_shoot & playerShoot)
{
    Player* player = players[playerShoot.playerID];
    if(player == 0)
        return;

    CVector3f p1;/*(playerShoot.p1);*/
    CVector3f p2;/*(playerShoot.p2);*/
    p1[0] = (float)playerShoot.p1[0] / 100.0f;
    p1[1] = (float)playerShoot.p1[1] / 100.0f;
    p1[2] = (float)playerShoot.p1[2] / 100.0f;
    p2[0] = (float)playerShoot.p2[0] / 100.0f;
    p2[1] = (float)playerShoot.p2[1] / 100.0f;
    p2[2] = (float)playerShoot.p2[2] / 100.0f;
    player->weapon->shotFrom = p1;

    if(player->weapon->weaponID == WEAPON_SHOTGUN)
    {
        // ves's suggestion
        const float directionAngles[5] = { -10.0f, -5.0f, 0.0f, 5.0f, 10.0f };
        const float deviationAngles[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };//for the moment, these simply identify which shots are which, the actual deviation is fixed in the next call to shootSV
        // adder's suggestion
        //const float directionAngles[7] = {0.0f, 7.0f, -7.0f, 3.5f, -3.5f, 10.5f, -10.5f};
        //const float deviationAngles[7] = {3.5f, 3.5f, 3.5f, 3.5f, 3.5f, 3.5f, 3.5f};
        CVector3f newP2;
        for(int i = 0; i < player->weapon->nbShot; ++i)
        {
            newP2 = rotateAboutAxis(p2, directionAngles[i], CVector3f(0.0f, 0.0f, 1.0f));
            shootSV(playerShoot.playerID, playerShoot.nuzzleID, deviationAngles[i], p1, newP2);
        }
    }
    else if(player->weapon->weaponID == WEAPON_SNIPER)
    {
        for(int i = 0; i < player->weapon->nbShot; ++i)
            shootSV(playerShoot.playerID, playerShoot.nuzzleID, 0, p1, p2);
    }
    else if(player->weapon->weaponID == WEAPON_CHAIN_GUN)
    {
        player->weapon->currentImp += 3;
        if(player->weapon->currentImp > player->weapon->impressision)
            player->weapon->currentImp = player->weapon->impressision;

        float imp = player->weapon->currentImp;

        if(gameVar.sv_serverType == SERVER_TYPE_PRO)
        {
            if(player->currentCF.vel.length() < 1.15f)
            {
                imp /= 2.70f;
            }
        }


        for(int i = 0; i < player->weapon->nbShot; ++i)
            shootSV(playerShoot.playerID, playerShoot.nuzzleID, imp, p1, p2);
    }
    else
    {
        player->weapon->currentImp += 3;
        if(player->weapon->currentImp > player->weapon->impressision)
            player->weapon->currentImp = player->weapon->impressision;
        for(int i = 0; i < player->weapon->nbShot; ++i)
            shootSV(playerShoot.playerID, playerShoot.nuzzleID, player->weapon->currentImp, p1, p2);
    }
}

void Game::shootSV(int playerID, int nuzzleID, float imp, CVector3f p1, CVector3f p2)
{
    Player* player = players[playerID];
    if(player == 0)
        return;
    int ident = (int)imp;
    CVector3f oldP2;
    if(player->weapon->weaponID == WEAPON_SHOTGUN)
    {
        switch(ident)
        {
        case 1:
            oldP2 = rotateAboutAxis(p2, 10.0f, CVector3f(0.0f, 0.0f, 1.0f));
            break;
        case 2:
            oldP2 = rotateAboutAxis(p2, 5.0f, CVector3f(0.0f, 0.0f, 1.0f));
            break;
        case 3:
            oldP2 = rotateAboutAxis(p2, 0.0f, CVector3f(0.0f, 0.0f, 1.0f));
            break;
        case 4:
            oldP2 = rotateAboutAxis(p2, -5.0f, CVector3f(0.0f, 0.0f, 1.0f));
            break;
        case 5:
            oldP2 = rotateAboutAxis(p2, -10.0f, CVector3f(0.0f, 0.0f, 1.0f));
            break;
        };
        normalize(oldP2);
    }
    if(player->weapon->weaponID == WEAPON_SHOTGUN)
    {
        imp = 3.5f;
    }
    CVector3f dir = p2;

    if(player->weapon->projectileType == PROJECTILE_DIRECT && player->weapon->weaponID == WEAPON_FLAME_THROWER)
    {
        if(gameVar.sv_ftExpirationTimer > 0)
        {
            float mult = (1.0f - player->secondsFired / gameVar.sv_ftExpirationTimer) * gameVar.sv_ftMaxRange;
            if(mult < gameVar.sv_ftMinRange)
                mult = gameVar.sv_ftMinRange;
            p2 = p2 * mult;//nuvem's awesome idea, ft range decreases the longer it's fired
        }
        else
            p2 = p2 * gameVar.sv_ftMaxRange;
    }
    else
        p2 = p2 * 128;

    p2 = rotateAboutAxis(p2, rand(-imp, imp), CVector3f(0, 0, 1));
    p2 = rotateAboutAxis(p2, rand(0.0f, 360.0f), dir);
    p2[2] *= .5f;
    p2 += p1;

    CVector3f normal;

    if(player->weapon->weaponID == WEAPON_SHOTGUN)
    {
        //--- Clamp shot
        CVector3f dir = p2 - p1;
        normalize(dir);

        float clampShot;
        float variation = 0.01f;
        if(gameVar.sv_serverType == SERVER_TYPE_PRO)
        {  //clampShot = gameVar.sv_shottyRange;
            CVector3f sinThetaVector = cross(dir, oldP2);
            float sinTheta = sinThetaVector.length();
            clampShot = gameVar.sv_shottyDropRadius / sinTheta;
        }
        else
        {
            switch(ident)
            {
            case 1:
                clampShot = gameVar.sv_shottyRange*(0.333f + rand(-variation, variation));
                break;
            case 2:
                clampShot = gameVar.sv_shottyRange*(0.667f + rand(-variation, variation));
                break;
            case 3:
                clampShot = gameVar.sv_shottyRange;
                break;
            case 4:
                clampShot = gameVar.sv_shottyRange*(0.667f + rand(-variation, variation));
                break;
            case 5:
                clampShot = gameVar.sv_shottyRange*(0.333f + rand(-variation, variation));
                break;
            };
        }

        p2 = p1 + dir * clampShot;
    }


    bool isCollision = false;

    if(map->rayTest(player->currentCF.position, p1, normal))
    {
        p1 += normal * .01f;
    }

    // On test s'il y a une collision, sinon, fuck it on envoit pas ça
    if(map->rayTest(p1, p2, normal))
    {
        isCollision = true;
    }

    if(player->weapon->weaponID == WEAPON_PHOTON_RIFLE || player->weapon->weaponID == WEAPON_FLAME_THROWER)
    {
        if(player->weapon->weaponID == WEAPON_PHOTON_RIFLE)
        {
            player->p1 = p1;
            player->p2 = p2;
            player->incShot = 30;
        }
        CVector3f p3 = p2;
        // On test premièrement si on touche un autre joueur!
        Player * hitPlayer = 0;
        for(int i = 0; i < MAX_PLAYER; i++)
        {
            if(players[i])
            {
                if(i != player->playerID)
                {
                    if(players[i]->status == PLAYER_STATUS_ALIVE && (players[i]->teamID != player->teamID || gameType == GAME_TYPE_DM || gameVar.sv_friendlyFire || gameVar.sv_reflectedDamage))
                    {
                        // Ray to sphere test
                        if(segmentToSphere(p1, p3, players[i]->currentCF.position, (player->weapon->weaponID == WEAPON_FLAME_THROWER) ? .50f : .25f))
                        {
                            isCollision = true;
                            hitPlayer = players[i];
                            normal = p3 - p1;
                            p3 = p2; // Full length
                            normalize(normal);

                            // On décrémente sa vie
                            hitPlayer->hitSV(gameVar.weapons[player->weapon->weaponID], player, gameVar.weapons[player->weapon->weaponID]->damage);
                        }
                    }
                }
            }
        }

        // On envoit le résultat à TOUT les joueurs y compris celui qui l'a tiré
        net_svcl_player_shoot playerShootSV;
        playerShootSV.hitPlayerID = -1;
        playerShootSV.playerID = player->playerID;
        playerShootSV.nuzzleID = nuzzleID;
        playerShootSV.p1[0] = (short)(p1[0] * 100);
        playerShootSV.p1[1] = (short)(p1[1] * 100);
        playerShootSV.p1[2] = (short)(p1[2] * 100);
        playerShootSV.p2[0] = (short)(p2[0] * 100);
        playerShootSV.p2[1] = (short)(p2[1] * 100);
        playerShootSV.p2[2] = (short)(p2[2] * 100);
        playerShootSV.normal[0] = (char)(normal[0] * 120);
        playerShootSV.normal[1] = (char)(normal[1] * 120);
        playerShootSV.normal[2] = (char)(normal[2] * 120);
        playerShootSV.weaponID = player->weapon->weaponID;
        bb_serverSend((char*)&playerShootSV, sizeof(net_svcl_player_shoot), NET_SVCL_PLAYER_SHOOT, 0);
    }
    else
    {
        // On test premièrement si on touche un autre joueur!
        Player * hitPlayer = 0;
        for(int i = 0; i < MAX_PLAYER; i++)
        {
            if(players[i])
            {
                if(i != player->playerID)
                {
                    if(players[i]->status == PLAYER_STATUS_ALIVE)
                    {
                        // Ray to sphere test
                        if(segmentToSphere(p1, p2, players[i]->currentCF.position, .25f))
                        {
                            isCollision = true;
                            hitPlayer = players[i];
                            normal = p2 - p1;
                            normalize(normal);
                        }
                    }
                }
            }
        }

        // On envoit le résultat à TOUT les joueurs y compris celui qui l'a tiré
        net_svcl_player_shoot playerShootSV;
        if(hitPlayer)
        {
            playerShootSV.hitPlayerID = hitPlayer->playerID;
            playerShootSV.weaponID = player->weapon->weaponID;

            // On décrémente sa vie
            hitPlayer->hitSV(gameVar.weapons[playerShootSV.weaponID], players[player->playerID]);
        }
        else
        {
            playerShootSV.hitPlayerID = -1;
        }
        playerShootSV.playerID = player->playerID;
        playerShootSV.nuzzleID = nuzzleID;
        playerShootSV.p1[0] = (short)(p1[0] * 100);
        playerShootSV.p1[1] = (short)(p1[1] * 100);
        playerShootSV.p1[2] = (short)(p1[2] * 100);
        playerShootSV.p2[0] = (short)(p2[0] * 100);
        playerShootSV.p2[1] = (short)(p2[1] * 100);
        playerShootSV.p2[2] = (short)(p2[2] * 100);
        playerShootSV.normal[0] = (char)(normal[0] * 120);
        playerShootSV.normal[1] = (char)(normal[1] * 120);
        playerShootSV.normal[2] = (char)(normal[2] * 120);
        playerShootSV.weaponID = player->weapon->weaponID;
        bb_serverSend((char*)&playerShootSV, sizeof(net_svcl_player_shoot), NET_SVCL_PLAYER_SHOOT, 0);
    }
}

//
// Pour toucher les joueurs dans un rayon
//
void Game::radiusHit(CVector3f & pos, float radius, char fromID, char weaponID, bool sameDmg)
{
    CVector3f position;
    position[0] = pos[0];
    position[1] = pos[1];
    position[2] = pos[2];//to keep the nuke explosion from moving when the nuker dies
    // Est-ce que ce joueur existe toujours?
    if(players[fromID])
    {
        for(int i = 0; i < MAX_PLAYER; ++i)
        {
            if(fromID == i)
            {
                if(weaponID == WEAPON_KNIVES) continue;
            }
            Player * player = players[i];
            if(player)
            {
                // Sa distance de l'impact
                float dis = distance(player->currentCF.position, position);
                if(dis < radius)
                {
                    CVector3f p1 = position;
                    CVector3f p2 = player->currentCF.position;
                    CVector3f normal;
                    // On le touche si y a pas un mur qui intercept!
                    if(!map->rayTest(p1, p2, normal))
                    {
                        CVector3f dir = player->currentCF.position - position;
                        normalize(dir);
                        /*  net_svcl_player_hit playerHit;
                            playerHit.damage = (1 - (dis / radius)) * gameVar.weapons[weaponID]->damage;
                            playerHit.playerID = (char)i;
                            playerHit.fromID = fromID;
                            playerHit.weaponID = weaponID;
                            playerHit.vel[0] = (char)((dir[0] * playerHit.damage * 10) / 10.0f);
                            playerHit.vel[1] = (char)((dir[1] * playerHit.damage * 10) / 10.0f);
                            playerHit.vel[2] = (char)((dir[2] * playerHit.damage * 10) / 10.0f);
                            bb_serverSend((char*)&playerHit,sizeof(net_svcl_player_hit),NET_SVCL_PLAYER_HIT,0);*/
                        player->hitSV(gameVar.weapons[weaponID], players[fromID], ((sameDmg) ? 1 : (1 - (dis / radius))) * gameVar.weapons[weaponID]->damage);
                    }
                }
            }
        }
    }
}

//
// Pour ajouter un nouveau joueur
//
int Game::createNewPlayerSV(int babonetID)
{
    // On lui trouve une nouvelle place
    for(int i = 0; i < gameVar.sv_maxPlayer; ++i)
    {
        if(!players[i])
        {
            players[i] = new Player((char)i, map, this);
            players[i]->babonetID = babonetID;

            // On envoit l'info à tout les clients (y compris lui)
            net_svcl_newplayer newPlayer;
            newPlayer.newPlayerID = players[i]->playerID;
            newPlayer.baboNetID = babonetID;
            bb_serverSend((char*)&newPlayer, sizeof(net_svcl_newplayer), NET_SVCL_NEWPLAYER, 0);

            // On lui envoi premièrement la version du jeu
            net_svcl_gameversion gameVersion;
            gameVersion.gameVersion = GAME_VERSION_SV;
            bb_serverSend((char*)&gameVersion, sizeof(net_svcl_gameversion), NET_SVCL_GAMEVERSION, babonetID);

            //--- Est-ce que c'est le seul joueur? ou il y a 2 joueur? On restart le server.


            return i;
        }
    }

    return -1; // Pus de place
}

int Game::numPlayers(void)
{
    int nbPlayer = 0;
    for(int i = 0; i < MAX_PLAYER; ++i)
    {
        if(players[i])
        {
            nbPlayer++;
        }
    }

    return nbPlayer;
}
