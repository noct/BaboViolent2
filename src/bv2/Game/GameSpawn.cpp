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
#include "Console.h"
#include "GameVar.h"
#include "baboNet.h"

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
            projectiles.push_back(new Projectile(position, vel, playerProjectile.weaponID, playerProjectile.projectileType, false, Projectile::uniqueProjectileID));
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
            projectiles.push_back(new Projectile(position, vel, playerProjectile.playerID, playerProjectile.projectileType, false, Projectile::uniqueProjectileID));
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
            projectiles.push_back(new Projectile(position, vel, playerProjectile.weaponID, playerProjectile.projectileType, true, playerProjectile.uniqueID));
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
            projectiles.push_back(new Projectile(position, vel, playerProjectile.playerID, playerProjectile.projectileType, true, playerProjectile.uniqueID));
        }
        projectiles[projectiles.size() - 1]->projectileID = (short)projectiles.size() - 1;
        //  projectiles[projectiles.size()-1]->remoteEntity = false;
        //  projectiles[projectiles.size()-1]->uniqueID = playerProjectile.uniqueID;
    }
#endif

    return true;
}
