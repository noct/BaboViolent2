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

long Projectile::uniqueProjectileID = 0;

void Game::castVote(const net_clsv_svcl_vote_request & voteRequest)
{
    //nbActivePlayers = 0;
    voting.activePlayersID.clear();
    for(int i = 0; i < MAX_PLAYER; ++i)
    {
        if(players[i])
        {
            if((players[i]->teamID != PLAYER_TEAM_AUTO_ASSIGN) &&
                (players[i]->teamID != PLAYER_TEAM_SPECTATOR))
            {
                voting.activePlayersID.push_back(players[i]->babonetID);
                players[i]->voted = false;
            }
            else
                players[i]->voted = true;
        }
    }
    voting.voted = true;
    voting.votingWhat = voteRequest.vote;
    voting.votingResults[0] = 0;
    voting.votingResults[1] = 0;
    voting.remaining = 30; // 30 sec to vote
    voting.votingInProgress = true;
}

bool Game::votingUpdate(float delay)
{
    if(voting.votingInProgress)
    {
        voting.remaining -= delay;
        if(voting.remaining <= 0)
        {
            voting.remaining = 0;
            voting.votingInProgress = false;
            return true;
        }

        if(voting.votingResults[0] > (int)voting.activePlayersID.size() / 2 ||
            voting.votingResults[1] > (int)voting.activePlayersID.size() / 2 ||
            voting.votingResults[0] + voting.votingResults[1] >= (int)voting.activePlayersID.size())
        {
            voting.votingInProgress = false;
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


    voting.voted = false;
    voting.remaining = 0;
    voting.votingInProgress = false;
    voting.votingResults[0] = 0;
    voting.votingResults[1] = 0;
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
        if(votingUpdate(delay))
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

void Game::onTeamSwitch(Player* player)
{
    // On print dans console
    switch(player->teamID)
    {
    case PLAYER_TEAM_SPECTATOR:
        console->add(CString("%s\x8 goes spectator ID:%i", player->name.s, player->playerID));
        break;
    case PLAYER_TEAM_BLUE:
        console->add(CString("%s\x1 joins blue team ID:%i", player->name.s, player->playerID));
        break;
    case PLAYER_TEAM_RED:
        console->add(CString("%s\x4 joins red team ID:%i", player->name.s, player->playerID));
        break;
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

            onTeamSwitch(players[playerID]);
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

//
// Spawner un player
//
bool Game::spawnPlayer(int playerID)
{
    if(players[playerID] && map)
    {
        if(players[playerID]->teamID == PLAYER_TEAM_BLUE || players[playerID]->teamID == PLAYER_TEAM_RED)
        {
            // On lui trouve une place libre loin des ennemies
            if(gameType == GAME_TYPE_DM)
            {
                float currentScore = 0;
                int bestFound = 0;
                if(map->dm_spawns.size() > 0)
                {
                    for(int i = 0; i < (int)map->dm_spawns.size(); ++i)
                    {
                        float nearestPlayer = 100000;
                        int nbPlayer = 0;
                        // Pour chaque spawn point on check pour chaque player
                        for(int j = 0; j < MAX_PLAYER; ++j)
                        {
                            if(players[j] && j != playerID)
                            {
                                if(players[j]->status == PLAYER_STATUS_ALIVE)
                                {
                                    nbPlayer++;
                                    float dis = distanceSquared(map->dm_spawns[i], players[j]->currentCF.position);
                                    if(dis < nearestPlayer) nearestPlayer = dis;
                                }
                            }
                        }
                        if(nearestPlayer > currentScore)
                        {
                            currentScore = nearestPlayer;
                            bestFound = i;
                        }
                        if(nbPlayer == 0)
                        {
                            bestFound = rand() % (int)map->dm_spawns.size();
                            break;
                        }
                    }
                    players[playerID]->spawn(CVector3f(map->dm_spawns[bestFound][0], map->dm_spawns[bestFound][1], .25f));
#ifndef DEDICATED_SERVER
                    map->setCameraPos(players[playerID]->currentCF.position);
#endif
                    return true;
                }
            }

            // On lui trouve une place libre loin des ennemies
            if(gameType == GAME_TYPE_TDM || gameType == GAME_TYPE_CTF)
            {
                float currentScore = 0;
                int bestFound = 0;
                if(map->dm_spawns.size() > 0)
                {
                    for(int i = 0; i < (int)map->dm_spawns.size(); ++i)
                    {
                        float nearestPlayer = 100000;
                        int nbPlayer = 0;
                        // Pour chaque spawn point on check pour chaque player
                        for(int j = 0; j < MAX_PLAYER; ++j)
                        {
                            if(players[j] && j != playerID)
                            {
                                if(players[j]->teamID != players[playerID]->teamID && players[j]->status == PLAYER_STATUS_ALIVE)
                                {
                                    nbPlayer++;
                                    float dis = distanceSquared(map->dm_spawns[i], players[j]->currentCF.position);
                                    if(dis < nearestPlayer) nearestPlayer = dis;
                                }
                            }
                        }
                        if(nearestPlayer > currentScore)
                        {
                            currentScore = nearestPlayer;
                            bestFound = i;
                        }
                        if(nbPlayer == 0)
                        {
                            bestFound = rand() % (int)map->dm_spawns.size();
                            break;
                        }
                    }
                    CVector3f spawnPosition(map->dm_spawns[bestFound][0], map->dm_spawns[bestFound][1], .25f);

                    if((gameType == GAME_TYPE_CTF) && (spawnType == SPAWN_TYPE_LADDER))
                    {
                        float timeElapsed = gameVar.sv_gameTimeLimit - gameTimeLeft;
                        if(timeElapsed < 10.0f)
                        {
                            spawnPosition = map->flagPodPos[players[playerID]->teamID];
                        }
                    }

                    players[playerID]->spawn(spawnPosition);
#ifndef DEDICATED_SERVER
                    map->setCameraPos(players[playerID]->currentCF.position);
#endif
                    return true;
                }
            }
            /*  while (!found)
                {
                    int index = rand()%(int)map->dm_spawns.size();
                    int x = rand((int)0, (int)map->size[0]);
                    int y = rand((int)0, (int)map->size[1]);
                    if (map->cells[y*map->size[0]+x].passable)
                    {
                        found = true;
                        players[playerID]->spawn(CVector3f((float)x+.5f,(float)y+.5f,.25f));
                        if (players[playerID] == thisPlayer)
                        {
                            map->setCameraPos(players[playerID]->currentCF.position);
                        }
                    }
                }
                return true;*/
        }
    }
    return false;
}

void Game::spawnProjectileSpecific(CVector3f & position, CVector3f & vel, char pFromID, int pProjectileType, bool pRemoteEntity, long pUniqueProjectileID)
{
    Projectile* projectile = new Projectile(position, vel, pFromID, pProjectileType, pRemoteEntity, pUniqueProjectileID);
    projectiles.push_back(projectile);
}

//
// On spawn un projectile!
//
bool Game::spawnProjectile(net_clsv_svcl_player_projectile & playerProjectile, bool imServer)
{
    //  if (playerProjectile.projectileType == PROJECTILE_FLAME) return;
        // On le push toujours à la fin du vector, si on respect bien ça les clients devraient tous les avoir
        // dans l'ordre
    if(imServer)
    {
        ++(Projectile::uniqueProjectileID);
        playerProjectile.uniqueID = Projectile::uniqueProjectileID;

        if(playerProjectile.projectileType == PROJECTILE_GRENADE)
        {
            if(players[playerProjectile.playerID]->nbGrenadeLeft <= 0) return false;
            if(players[playerProjectile.playerID]) players[playerProjectile.playerID]->nbGrenadeLeft--;
        }
        if(playerProjectile.projectileType == PROJECTILE_COCKTAIL_MOLOTOV)
        {
            if(players[playerProjectile.playerID]->nbMolotovLeft <= 0) return false;
            if(players[playerProjectile.playerID]) players[playerProjectile.playerID]->nbMolotovLeft--;
        }
        if(playerProjectile.projectileType == PROJECTILE_DROPED_WEAPON)
        {
            CVector3f position;
            position[0] = (float)playerProjectile.position[0] / 100.0f;
            position[1] = (float)playerProjectile.position[1] / 100.0f;
            position[2] = (float)playerProjectile.position[2] / 100.0f;
            CVector3f vel;
            vel[0] = (float)playerProjectile.vel[0] / 10.0f;
            vel[1] = (float)playerProjectile.vel[1] / 10.0f;
            vel[2] = (float)playerProjectile.vel[2] / 10.0f;
            spawnProjectileSpecific(position, vel, playerProjectile.weaponID, playerProjectile.projectileType, false, Projectile::uniqueProjectileID);
        }
        else
        {
            CVector3f position;
            position[0] = (float)playerProjectile.position[0] / 100.0f;
            position[1] = (float)playerProjectile.position[1] / 100.0f;
            position[2] = (float)playerProjectile.position[2] / 100.0f;
            CVector3f vel;
            vel[0] = (float)playerProjectile.vel[0] / 10.0f;
            vel[1] = (float)playerProjectile.vel[1] / 10.0f;
            vel[2] = (float)playerProjectile.vel[2] / 10.0f;
            spawnProjectileSpecific(position, vel, playerProjectile.playerID, playerProjectile.projectileType, false, Projectile::uniqueProjectileID);
        }
        projectiles[projectiles.size() - 1]->projectileID = (short)projectiles.size() - 1;

        if(playerProjectile.projectileType == PROJECTILE_FLAME)
        {
            Projectile * projectile = projectiles[projectiles.size() - 1];

            // On demande au server de créer une instance d'une flame
            net_clsv_svcl_player_projectile playerProjectile;
            playerProjectile.playerID = projectile->fromID;
            playerProjectile.nuzzleID = 0;
            playerProjectile.projectileType = projectile->projectileType;
            playerProjectile.weaponID = WEAPON_COCKTAIL_MOLOTOV;
            playerProjectile.position[0] = (short)(projectile->currentCF.position[0] * 100);
            playerProjectile.position[1] = (short)(projectile->currentCF.position[1] * 100);
            playerProjectile.position[2] = (short)(projectile->currentCF.position[2] * 100);
            playerProjectile.vel[0] = (char)(projectile->currentCF.vel[0] * 10);
            playerProjectile.vel[1] = (char)(projectile->currentCF.vel[1] * 10);
            playerProjectile.vel[2] = (char)(projectile->currentCF.vel[2] * 10);
            playerProjectile.uniqueID = projectile->uniqueID;
            bb_serverSend((char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, 0);
        }
    }
#ifndef DEDICATED_SERVER
    else
    {
        if(playerProjectile.projectileType == PROJECTILE_DROPED_WEAPON)
        {
            CVector3f position;
            position[0] = (float)playerProjectile.position[0] / 100.0f;
            position[1] = (float)playerProjectile.position[1] / 100.0f;
            position[2] = (float)playerProjectile.position[2] / 100.0f;
            CVector3f vel;
            vel[0] = (float)playerProjectile.vel[0] / 10.0f;
            vel[1] = (float)playerProjectile.vel[1] / 10.0f;
            vel[2] = (float)playerProjectile.vel[2] / 10.0f;
            spawnProjectileSpecific(position, vel, playerProjectile.weaponID, playerProjectile.projectileType, true, playerProjectile.uniqueID);
        }
        else
        {
            CVector3f position;
            position[0] = (float)playerProjectile.position[0] / 100.0f;
            position[1] = (float)playerProjectile.position[1] / 100.0f;
            position[2] = (float)playerProjectile.position[2] / 100.0f;
            CVector3f vel;
            vel[0] = (float)playerProjectile.vel[0] / 10.0f;
            vel[1] = (float)playerProjectile.vel[1] / 10.0f;
            vel[2] = (float)playerProjectile.vel[2] / 10.0f;
            spawnProjectileSpecific(position, vel, playerProjectile.playerID, playerProjectile.projectileType, true, playerProjectile.uniqueID);
        }
        projectiles[projectiles.size() - 1]->projectileID = (short)projectiles.size() - 1;
        //  projectiles[projectiles.size()-1]->remoteEntity = false;
        //  projectiles[projectiles.size()-1]->uniqueID = playerProjectile.uniqueID;
    }
#endif

    return true;
}

//
// Constructeur
//
Projectile::Projectile(CVector3f & position, CVector3f & vel, char pFromID, int pProjectileType, bool pRemoteEntity, long pUniqueProjectileID)
{
    fromID = pFromID;

    uniqueID = pUniqueProjectileID;


    timeSinceThrown = 0.0f;

    projectileType = pProjectileType;
    currentCF.position = position;
    currentCF.vel = vel;
    damageTime = 0;


    lastCF = currentCF;
    netCF0.reset();
    netCF1.reset();
    cFProgression = 0;

    stickToPlayer = -1;
    stickFor = 0;

    whenToShoot = 0;

    remoteEntity = pRemoteEntity; // Ça c'est server only
    needToBeDeleted = false;
    reallyNeedToBeDeleted = false;
    movementLock = false;

    projectileID = 0;

    switch(projectileType)
    {
    case PROJECTILE_ROCKET:
    {
        duration = 10; // 10 sec
        // On calcul l'angle que la rocket devrait avoir (dépendanment de sa vel, qui est l'Orientation)
        CVector3f dirVect = vel;
        dirVect[2] = 0; // L'orientation est juste en Z
        normalize(dirVect);
        float dotWithY = dot(CVector3f(0, 1, 0), dirVect);
        float dotWithX = dot(CVector3f(1, 0, 0), dirVect);
        currentCF.angle = acosf(dotWithY)*TO_DEGREE;
        if(dotWithX > 0) currentCF.angle = -currentCF.angle;
        // La rocket démarre plus vite
        currentCF.vel *= 2.5f;

        break;
    }
    case PROJECTILE_GRENADE:
    {
        duration = 2;
        // La grenade démarre plus vite
        currentCF.vel *= 5;
        currentCF.vel[2] += 5; // Pas trop apique, on veut pogner les murs
        break;
    }
    case PROJECTILE_COCKTAIL_MOLOTOV:
    {
        duration = 10;
        currentCF.vel *= 6;
        currentCF.vel[2] += 2;
        break;
    }
    case PROJECTILE_LIFE_PACK:
    {
        //  currentCF.vel.set(0,0,1);
        //  currentCF.vel = rotateAboutAxis(currentCF.vel, rand(-45.0f, 45.0f), CVector3f(1,0,0));
        //  currentCF.vel = rotateAboutAxis(currentCF.vel, rand(-0.0f, 360.0f), CVector3f(0,0,1)) * 3;
        duration = 20;
        break;
    }
    case PROJECTILE_DROPED_WEAPON:
    {
        //  currentCF.vel.set(0,0,1);
        //  currentCF.vel = rotateAboutAxis(currentCF.vel, rand(-45.0f, 45.0f), CVector3f(1,0,0));
        //  currentCF.vel = rotateAboutAxis(currentCF.vel, rand(-0.0f, 360.0f), CVector3f(0,0,1)) * 3;
        duration = 30;
        break;
    }
    case PROJECTILE_DROPED_GRENADE:
    {
        //  currentCF.vel.set(0,0,1);
        //  currentCF.vel = rotateAboutAxis(currentCF.vel, rand(-45.0f, 45.0f), CVector3f(1,0,0));
        //  currentCF.vel = rotateAboutAxis(currentCF.vel, rand(-0.0f, 360.0f), CVector3f(0,0,1)) * 3;
        duration = 25;
        break;
    }
    case PROJECTILE_FLAME:
    {
        duration = 10;
        break;
    }
    case PROJECTILE_GIB:
    {
        //      isClientOnly
        break;
    }
    }
}

//
// Son update
//
void Projectile::update(float delay, Map* map)
{
    lastCF = currentCF; // On garde une copie du dernier coordFrame
    currentCF.frameID++; // Ça ça reste inchangé

    timeSinceThrown += delay;

        // On la clamp quand meme (1 sqrtf par rocket, pas si pire..)
    float speed = currentCF.vel.length();
    if(projectileType == PROJECTILE_ROCKET)
    {
        if(speed > 10) // 10 = fast enough :P
        {
            currentCF.vel /= speed;
            speed = 10;
            currentCF.vel *= speed;
        }

        // On déplace avec la velocity
        currentCF.position += currentCF.vel * delay;

        // On incrémente la vel, la rocket à fuse en sale! (accélération exponentiel!)
        currentCF.vel += currentCF.vel * delay * 3;
    }

    if(projectileType == PROJECTILE_COCKTAIL_MOLOTOV)
    {
        // On déplace avec la velocity
        currentCF.position += currentCF.vel * delay;

        // On affecte la gravitée!
        currentCF.vel[2] -= 9.8f * delay;
    }

    if(projectileType == PROJECTILE_FLAME && !movementLock)
    {
        // On déplace avec la velocity
        currentCF.position += currentCF.vel * delay;

        // On affecte la gravitée!
        currentCF.vel[2] -= 9.8f * delay;
    }

    //--- Le feu est pogné sur un player
    if(projectileType == PROJECTILE_FLAME)
    {
        if(!remoteEntity)
        {
            if(stickToPlayer >= 0 && scene->server)
            {
                if(scene->server->game->players[stickToPlayer])
                {
                    if(scene->server->game->players[stickToPlayer]->status == PLAYER_STATUS_DEAD)
                    {
                        stickToPlayer = -1;
                    }
                    else
                    {
                        currentCF.position = scene->server->game->players[stickToPlayer]->currentCF.position;
                    }
                }
                stickFor -= delay;
                if(stickFor <= 0)
                {
                    stickFor = 0;
                    stickToPlayer = -1;
                    net_svcl_flame_stick_to_player flameStickToPlayer;
                    flameStickToPlayer.playerID = -1;
                    flameStickToPlayer.projectileID = projectileID;
                    bb_serverSend((char*)&flameStickToPlayer, sizeof(net_svcl_flame_stick_to_player), NET_SVCL_FLAME_STICK_TO_PLAYER, 0);
                    movementLock = false;
                    stickFor = 1.0f; // 1 sec sans retoucher à un autre joueur (quand meme là)
                //  net_svcl_flame_stick_to_player flameStickToPlayer;
                    flameStickToPlayer.playerID = -1;
                    flameStickToPlayer.projectileID = projectileID;
                    bb_serverSend((char*)&flameStickToPlayer, sizeof(net_svcl_flame_stick_to_player), NET_SVCL_FLAME_STICK_TO_PLAYER, 0);
                }
            }
            if(stickToPlayer == -1)
            {
                stickFor -= delay;
                if(stickFor <= 0)
                {
                    stickFor = 0;
                    Player * playerInRadius = scene->server->game->playerInRadius(currentCF.position, .5f, this->timeSinceThrown > 0.5f ? -1 : this->fromID);
                    if(playerInRadius)
                    {
                        movementLock = true;
                        stickToPlayer = playerInRadius->playerID;
                        stickFor = 3;
                        net_svcl_flame_stick_to_player flameStickToPlayer;
                        flameStickToPlayer.playerID = playerInRadius->playerID;
                        flameStickToPlayer.projectileID = projectileID;
                        bb_serverSend((char*)&flameStickToPlayer, sizeof(net_svcl_flame_stick_to_player), NET_SVCL_FLAME_STICK_TO_PLAYER, 0);
                    }
                }
            }

            damageTime++;
            if(damageTime >= 20)
            {
                damageTime = 0;
                if(scene->server)
                {
                    scene->server->game->radiusHit(currentCF.position, .5f, fromID, WEAPON_COCKTAIL_MOLOTOV);
                }
            }
        }
        else
        {
            if(stickToPlayer >= 0)
            {
#ifndef DEDICATED_SERVER
                if(scene->client->game->players[stickToPlayer])
                {
                    if(scene->client->game->players[stickToPlayer]->status == PLAYER_STATUS_DEAD)
                    {
                        stickToPlayer = -1;
                    }
                    else
                    {
                        currentCF.position = scene->client->game->players[stickToPlayer]->currentCF.position;
                    }
                }
#endif
            }
        }
    }

    // On check les collisions
    if(map && projectileType == PROJECTILE_FLAME && !movementLock)
    {
        // On test si on ne pogne pas un babo!
        CVector3f p1 = lastCF.position;
        CVector3f p2 = currentCF.position;
        CVector3f normal;
        if(map->rayTest(p1, p2, normal))
        {
            // On lock les mouvements du feu
            movementLock = true;

            // On se cré un spot par terre, pis on lock les mouvement du feu là
            currentCF.position = p2 + normal * .1f;
        }
    }

    if(speed > .5f || currentCF.position[2] > .2f)
    {
        if(projectileType == PROJECTILE_GRENADE || projectileType == PROJECTILE_LIFE_PACK || projectileType == PROJECTILE_DROPED_WEAPON || projectileType == PROJECTILE_DROPED_GRENADE)
        {
            // On déplace avec la velocity
            currentCF.position += currentCF.vel * delay;

            // On affecte la gravitée!
            currentCF.vel[2] -= 9.8f * delay; // (suposont q'un babo fait 50cm de diamètre)
        }

        if(map && projectileType == PROJECTILE_GRENADE || projectileType == PROJECTILE_LIFE_PACK || projectileType == PROJECTILE_DROPED_WEAPON || projectileType == PROJECTILE_DROPED_GRENADE)
        {
            CVector3f p1 = lastCF.position;
            CVector3f p2 = currentCF.position;
            CVector3f normal;
            if(map->rayTest(p1, p2, normal))
            {
                if(remoteEntity)
                {
                    //--- Play the sound
#ifndef DEDICATED_SERVER
                    dksPlay3DSound(gameVar.sfx_grenadeRebond, -1, 1, p2, 200);
#endif
                }
                currentCF.position = p2 + normal * .01f;
                currentCF.vel = reflect(currentCF.vel, normal);
                currentCF.vel *= .65f;
            }
        }
    }
    else
    {
        currentCF.vel.set(0, 0, 0);
    }

    // C le server et lui seul qui décide quand il est temps de mettre fin à ses jours
    if(!remoteEntity)
    {
        duration -= delay;
        if(duration <= 0)
        {

            // KATABOOM
            if(projectileType == PROJECTILE_GRENADE)
            {
                if(needToBeDeleted) return;

                needToBeDeleted = true;

                // On se cré DA explosion :P
                net_svcl_explosion explosion;
                explosion.position[0] = currentCF.position[0];
                explosion.position[1] = currentCF.position[1];
                explosion.position[2] = currentCF.position[2];
                explosion.playerID = (char)(-1);
                explosion.normal[0] = 0;
                explosion.normal[1] = 0;
                explosion.normal[2] = 1;
                explosion.radius = 1.5f;
                bb_serverSend((char*)&explosion, sizeof(net_svcl_explosion), NET_SVCL_EXPLOSION, 0);
                if(scene->server) scene->server->game->radiusHit(currentCF.position, 3, fromID, WEAPON_GRENADE);
                return;
            }
            else
            {
                needToBeDeleted = true;
            }
            return;
        }
    }

    // On check les collisions
    float zookaRadius = 3.0;
    if(gameVar.sv_zookaRemoteDet && gameVar.sv_serverType == 1)
        zookaRadius = gameVar.sv_zookaRadius;
    if(gameVar.sv_serverType = 1)
    {
        gameVar.weapons[WEAPON_BAZOOKA]->damage = gameVar.sv_zookaDamage;
    }
    else
    {
        gameVar.weapons[WEAPON_BAZOOKA]->damage = 0.75f;
    }
    if(map && projectileType == PROJECTILE_ROCKET && !remoteEntity && !needToBeDeleted)
    {
        // On test si on ne pogne pas un babo!
        Player * playerInRadius = (scene->server) ? scene->server->game->playerInRadius(currentCF.position, .25f) : 0;
        if(playerInRadius) if(playerInRadius->playerID == fromID) playerInRadius = 0;
        if(playerInRadius)
        {
            scene->server->game->players[fromID]->rocketInAir = false;
            scene->server->game->players[fromID]->detonateRocket = false;
            // On frappe un mec !!! KKAAAABBOOOUUMM PLEIN DE SANG MOUHOUHAHAHAHHA
            needToBeDeleted = true;
            // On se cré DA explosion :P
            net_svcl_explosion explosion;
            explosion.position[0] = playerInRadius->currentCF.position[0];
            explosion.position[1] = playerInRadius->currentCF.position[1];
            explosion.position[2] = playerInRadius->currentCF.position[2];
            explosion.normal[0] = 0;
            explosion.normal[1] = 0;
            explosion.normal[2] = 1;
            explosion.radius = zookaRadius;
            explosion.playerID = fromID;
            bb_serverSend((char*)&explosion, sizeof(net_svcl_explosion), NET_SVCL_EXPLOSION, 0);
            if(scene->server) scene->server->game->radiusHit(playerInRadius->currentCF.position, zookaRadius, fromID, WEAPON_BAZOOKA);
            return;
        }
        else
        {
            CVector3f p1 = lastCF.position;
            CVector3f p2 = currentCF.position;
            CVector3f normal;
            if(map->rayTest(p1, p2, normal) || scene->server->game->players[fromID]->detonateRocket)
            {
                scene->server->game->players[fromID]->rocketInAir = false;
                scene->server->game->players[fromID]->detonateRocket = false;
                p2 += normal * .1f;
                // On frappe un mur !!! KKAAAABBOOOUUMM
                needToBeDeleted = true;
                // On se cré DA explosion :P
                net_svcl_explosion explosion;
                explosion.position[0] = p2[0];
                explosion.position[1] = p2[1];
                explosion.position[2] = p2[2];
                explosion.normal[0] = normal[0];
                explosion.normal[1] = normal[1];
                explosion.normal[2] = normal[2];
                explosion.radius = zookaRadius;
                explosion.playerID = fromID;
                bb_serverSend((char*)&explosion, sizeof(net_svcl_explosion), NET_SVCL_EXPLOSION, 0);
                if(scene->server) scene->server->game->radiusHit(p2, zookaRadius, fromID, WEAPON_BAZOOKA);

                return;
            }
        }
    }

    // On check les collisions
    if(map && projectileType == PROJECTILE_COCKTAIL_MOLOTOV && !remoteEntity && !needToBeDeleted)
    {
        // On test si on ne pogne pas un babo!
        Player * playerInRadius = (scene->server) ? scene->server->game->playerInRadius(currentCF.position, .25f, (int)fromID) : 0;
        if(playerInRadius)
        {
            if(playerInRadius->playerID == fromID) playerInRadius = 0;
        }
        if(playerInRadius)
        {
            //console->add( CString("from id : %i",playerInRadius->playerID));
            // On frappe un mec !!! Flak MOLOTOV PARTY!
            needToBeDeleted = true;

            // On se cré DA FLAME explosion :P
            net_svcl_play_sound playSound;
            playSound.position[0] = (unsigned char)currentCF.position[0];
            playSound.position[1] = (unsigned char)currentCF.position[1];
            playSound.position[2] = (unsigned char)currentCF.position[2];
            playSound.volume = 250;
            playSound.range = 5;
            playSound.soundID = SOUND_MOLOTOV;
            bb_serverSend((char*)&playSound, sizeof(net_svcl_play_sound), NET_SVCL_PLAY_SOUND, 0);

            // On spawn du feu
            net_clsv_svcl_player_projectile playerProjectile;
            playerProjectile.playerID = fromID;
            playerProjectile.nuzzleID = 0;
            playerProjectile.position[0] = (short)(currentCF.position[0] * 100);
            playerProjectile.position[1] = (short)(currentCF.position[1] * 100);
            playerProjectile.position[2] = (short)(currentCF.position[2] * 100);
            playerProjectile.weaponID = 0;//WEAPON_COCKTAIL_MOLOTOV;
            playerProjectile.projectileType = PROJECTILE_FLAME;
            playerProjectile.vel[0] = 0;
            playerProjectile.vel[1] = 0;
            playerProjectile.vel[2] = 0;
            scene->server->game->spawnProjectile(playerProjectile, true);

            //for (int i=0;i<1;++i)
            //{
            //  console->add("Creating flame");
                //net_clsv_svcl_player_projectile playerProjectile;
            playerProjectile.playerID = fromID;
            playerProjectile.nuzzleID = 0;
            playerProjectile.position[0] = (short)(currentCF.position[0] * 100);
            playerProjectile.position[1] = (short)(currentCF.position[1] * 100);
            playerProjectile.position[2] = (short)(currentCF.position[2] * 100);
            playerProjectile.weaponID = 0;//WEAPON_COCKTAIL_MOLOTOV;
            playerProjectile.projectileType = PROJECTILE_FLAME;
            CVector3f vel = currentCF.vel*.5f + rand(CVector3f(-1, -1, 1), CVector3f(1, 1, 2));
            playerProjectile.vel[0] = 0; (char)(vel[0] * 10);
            playerProjectile.vel[1] = 0; (char)(vel[1] * 10);
            playerProjectile.vel[2] = 0; (char)(vel[2] * 10);
            scene->server->game->spawnProjectile(playerProjectile, true);
            //}
            return;
        }
        else
        {
            CVector3f p1 = lastCF.position;
            CVector3f p2 = currentCF.position;
            CVector3f normal;
            if(map->rayTest(p1, p2, normal))
            {
                currentCF.position = p2 + normal * .1f;

                // On frappe un mur ou un plancher, Molotov Party time
                needToBeDeleted = true;
                // On se cré DA FLAME explosion :P
                net_svcl_play_sound playSound;
                playSound.position[0] = (unsigned char)p2[0];
                playSound.position[1] = (unsigned char)p2[1];
                playSound.position[2] = (unsigned char)p2[2];
                playSound.volume = 250;
                playSound.range = 5;
                playSound.soundID = SOUND_MOLOTOV;
                bb_serverSend((char*)&playSound, sizeof(net_svcl_play_sound), NET_SVCL_PLAY_SOUND, 0);

                // On spawn du feu
                net_clsv_svcl_player_projectile playerProjectile;
                playerProjectile.playerID = fromID;
                playerProjectile.nuzzleID = 0;
                playerProjectile.position[0] = (short)(currentCF.position[0] * 100);
                playerProjectile.position[1] = (short)(currentCF.position[1] * 100);
                playerProjectile.position[2] = (short)(currentCF.position[2] * 100);
                playerProjectile.weaponID = 0;//WEAPON_COCKTAIL_MOLOTOV;
                playerProjectile.projectileType = PROJECTILE_FLAME;
                playerProjectile.vel[0] = 0;
                playerProjectile.vel[1] = 0;
                playerProjectile.vel[2] = 0;
                scene->server->game->spawnProjectile(playerProjectile, true);
                for(int i = 0; i < 1; ++i)
                {
                    //  console->add("Creating flame");
                    net_clsv_svcl_player_projectile playerProjectile;
                    playerProjectile.playerID = fromID;
                    playerProjectile.nuzzleID = 0;
                    playerProjectile.position[0] = (short)(currentCF.position[0] * 100);
                    playerProjectile.position[1] = (short)(currentCF.position[1] * 100);
                    playerProjectile.position[2] = (short)(currentCF.position[2] * 100);
                    playerProjectile.weaponID = 0;//WEAPON_COCKTAIL_MOLOTOV;
                    playerProjectile.projectileType = PROJECTILE_FLAME;
                    CVector3f vel = reflect(currentCF.vel*.5f, normal) + rand(CVector3f(-1, -1, 0), CVector3f(1, 1, 1));
                    playerProjectile.vel[0] = (char)(vel[0] * 10);
                    playerProjectile.vel[1] = (char)(vel[1] * 10);
                    playerProjectile.vel[2] = (char)(vel[2] * 10);
                    scene->server->game->spawnProjectile(playerProjectile, true);
                }
                return;
            }
        }
    }

    // On ramasse de la vie
    if(projectileType == PROJECTILE_LIFE_PACK && !remoteEntity && !needToBeDeleted)
    {
        // On test si on ne pogne pas un babo!
        Player * playerInRadius = (scene->server) ? scene->server->game->playerInRadius(CVector3f(currentCF.position[0], currentCF.position[1], .25f), .25f) : 0;
        if(playerInRadius)
        {
            // On lui donne de la vie yééé
            playerInRadius->life += .5f;
            if(playerInRadius->life > 1) playerInRadius->life = 1;
            needToBeDeleted = true;
            net_svcl_pickup_item pickupItem;
            pickupItem.playerID = playerInRadius->playerID;
            pickupItem.itemType = ITEM_LIFE_PACK;
            pickupItem.itemFlag = 0;
            bb_serverSend((char*)&pickupItem, sizeof(net_svcl_pickup_item), NET_SVCL_PICKUP_ITEM, 0);
            return;
        }
    }

    // On ramasse les grenades
    if(projectileType == PROJECTILE_DROPED_GRENADE && !remoteEntity && !needToBeDeleted)
    {
        // On test si on ne pogne pas un babo!
        Player * playerInRadius = (scene->server) ? scene->server->game->playerInRadius(CVector3f(currentCF.position[0], currentCF.position[1], .25f), .25f) : 0;
        if(playerInRadius)
        {
            // On lui donne la grenade
            playerInRadius->nbGrenadeLeft += 1;
            if(playerInRadius->nbGrenadeLeft > 3) playerInRadius->nbGrenadeLeft = 3;
            needToBeDeleted = true;
            net_svcl_pickup_item pickupItem;
            pickupItem.playerID = playerInRadius->playerID;
            pickupItem.itemType = ITEM_GRENADE;
            pickupItem.itemFlag = 0;
            bb_serverSend((char*)&pickupItem, sizeof(net_svcl_pickup_item), NET_SVCL_PICKUP_ITEM, 0);
            return;
        }
    }
}

//
// pour updater le coordFrame avec celui du server
//
void Projectile::setCoordFrame(net_svcl_projectile_coord_frame & projectileCoordFrame)
{
    if(netCF1.frameID > projectileCoordFrame.frameID) return;

    // Notre dernier keyframe change pour celui qu'on est rendu
    netCF0 = currentCF;
    netCF0.frameID = netCF1.frameID; // On pogne le frameID de l'ancien packet par contre
    cFProgression = 0; // On commence au début de la courbe ;)

    // On donne la nouvelle velocity à notre entity
    currentCF.vel[0] = (float)projectileCoordFrame.vel[0] / 10.0f;
    currentCF.vel[1] = (float)projectileCoordFrame.vel[1] / 10.0f;
    currentCF.vel[2] = (float)projectileCoordFrame.vel[2] / 10.0f;

    // Son frame ID
    netCF1.frameID = projectileCoordFrame.frameID;

    // Va faloir interpoler ici et prédire (job's done!)
    netCF1.position[0] = (float)projectileCoordFrame.position[0] / 100.0f;
    netCF1.position[1] = (float)projectileCoordFrame.position[1] / 100.0f;
    netCF1.position[2] = (float)projectileCoordFrame.position[2] / 100.0f;

    // Sa velocity
    netCF1.vel[0] = (float)projectileCoordFrame.vel[0] / 10.0f;
    netCF1.vel[1] = (float)projectileCoordFrame.vel[1] / 10.0f;
    netCF1.vel[2] = (float)projectileCoordFrame.vel[2] / 10.0f;

    // Si notre frameID était à 0, on le copie direct
    if(netCF0.frameID == 0)
    {
        netCF0 = netCF1;
    }
}

