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
#include "Client.h"
#include "ClientConsole.h"
#include "Server.h"
#include "Scene.h"
#include <time.h>
#include <algorithm>
#include "Client.h"

//
// Pour spawner du sang quand on touche quelqu'un
//
void ClientGame::spawnBlood(CVector3f & position, float damage)
{
    float bloodColor;

    if(damage > 2.0) damage = 2.0;

    for(int i = 0; i < (int)(damage * 10); ++i)
    {
        bloodColor = rand(0.3f, 1.0f);

        dkpCreateParticleEx(
            position, //CVector3f & positionFrom,
            position, //CVector3f & positionTo,
            CVector3f(0, 0, 1), //CVector3f & direction,
            damage, //float speedFrom,
            damage * 2, //float speedTo,
            0, //float pitchFrom,
            90, //float pitchTo,
            .25f, //float startSizeFrom,
            .25f, //float startSizeTo,
            .25f, //float endSizeFrom,
            .25f, //float endSizeTo,
            2, //float durationFrom,
            2, //float durationTo,
            CColor4f(bloodColor, 0, 0, 1), //CColor4f & startColorFrom,
            CColor4f(bloodColor, 0, 0, 1), //CColor4f & startColorTo,
            CColor4f(bloodColor, 0, 0, 0), //CColor4f & endColorFrom,
            CColor4f(bloodColor, 0, 0, 0), //CColor4f & endColorTo,
            0, //float angleFrom,
            360, //float angleTo,
            -30, //float angleSpeedFrom,
            30, //float angleSpeedTo,
            .1f, //float gravityInfluence,
            0, //float airResistanceInfluence,
            1, //unsigned int particleCountFrom,
            1, //unsigned int particleCountTo,
            gameVar.tex_blood + rand() % 10,
            0, //int textureFrameCount,
            DKP_SRC_ALPHA, //unsigned int srcBlend,
            DKP_ONE_MINUS_SRC_ALPHA);//unsigned int dstBlend);

        // On cré les marks de sang au sol
        CVector3f pos(1, 0, 0);
        pos = rotateAboutAxis(pos, rand(0.0f, 360.0f), CVector3f(0, 0, 1));
        float distance = rand(0.0f, damage*2.5f);
        float sizeMax = (1 - (distance) / 2.5f) * 1.0f;
        pos *= distance;
        pos += position;
        pos[2] = 0;
        floorMarks[getNextFloorMark()].set(pos, rand(0.0f, 360.0f), rand(.05f, sizeMax), 30, distance*0.5f, gameVar.tex_blood[rand() % 10], CVector4f(rand(.25f, 0.5f), 0, 0, rand(.5f, 1.0f)));
    }
}

extern Scene * scene;

ClientGame::ClientGame(CString pMapName): Game(pMapName), mapBuffer(8192)
{
    font = dkfCreateFont("main/fonts/babo.tga");
    tex_miniMapAllied = dktCreateTextureFromFile("main/textures/MiniMapAllied.tga", DKT_FILTER_LINEAR);
    tex_miniMapEnemy = dktCreateTextureFromFile("main/textures/MiniMapEnemy.tga", DKT_FILTER_LINEAR);;
    tex_baboShadow = dktCreateTextureFromFile("main/textures/BaboShadow.tga", DKT_FILTER_BILINEAR);

    // On ne cré pas notre player tout de suite, on attends confirmation du server
    thisPlayer = 0;

    // Pour afficher les stats multiplayers
    showStats = false;

    dkpReset();
    dkpSetSorting(false);

    nextWriteFloorMark = 0;
    nextWriteDrip = 0;
    viewShake = 0;
    dotAnim = 0;
    mapBytesRecieved = 0;

    // On load nos sons pour le mode CTF
    sfx_fcapture = dksCreateSoundFromFile("main/sounds/ftook.wav", false);
    sfx_ecapture = dksCreateSoundFromFile("main/sounds/etook.wav", false);
    sfx_return = dksCreateSoundFromFile("main/sounds/return.wav", false);
    sfx_win = dksCreateSoundFromFile("main/sounds/cheerRedTeam.wav", false);
    sfx_loose = dksCreateSoundFromFile("main/sounds/cheerBlueTeam.wav", false);
}

ClientGame::~ClientGame()
{
    ZEVEN_DELETE_VECTOR(clientProjectiles, i);
    ZEVEN_DELETE_VECTOR(trails, i);
    ZEVEN_DELETE_VECTOR(douilles, i);
    dksDeleteSound(sfx_fcapture);
    dksDeleteSound(sfx_ecapture);
    dksDeleteSound(sfx_return);
    dksDeleteSound(sfx_win);
    dksDeleteSound(sfx_loose);
    ZEVEN_DELETE_VECTOR(nikeFlashes, i);

    dkfDeleteFont(&font);
    dktDeleteTexture(&tex_baboShadow);
    dktDeleteTexture(&tex_miniMapAllied);
    dktDeleteTexture(&tex_miniMapEnemy);
}


void ClientGame::resetRound()
{
    ZEVEN_DELETE_VECTOR(clientProjectiles, i);
    ZEVEN_DELETE_VECTOR(trails, i);
    ZEVEN_DELETE_VECTOR(douilles, i);
    ZEVEN_DELETE_VECTOR(nikeFlashes, i);
    for(int i = 0; i < MAX_FLOOR_MARK; floorMarks[i++].delay = 0);
    for(int i = 0; i < MAX_FLOOR_MARK; drips[i++].life = 0);

    Game::resetRound();
}

void ClientGame::createMap()
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

    map = new Map(mapName, this, font);

    if(!map->isValid)
    {
        console->add("\x4> Invalid map");
        ZEVEN_SAFE_DELETE(map);
        if(scene->client)
        {
            scene->client->needToShutDown = true;
            scene->client->isRunning = false;
        }
        if(scene->server)
        {
            scene->server->needToShutDown = true;
            scene->server->isRunning = false;
        }
        return;
    }
    else
    {
        if(thisPlayer)
        {
            thisPlayer->map = map;
            dkpReset();
            if(gameVar.s_inGameMusic)
            {
                dksPlayMusic("main/sounds/Music.ogg", -1, 60);
            }
        }
    }
    dkcJumpToFrame(scene->ctx, 0);
}


void ClientGame::update(float delay)
{
    int i;
    Game::update(delay);


    dotAnim += delay * 720;
    while(dotAnim >= 360) dotAnim -= 360;


    auto cconsole = static_cast<ClientConsole*>(console);
    if(!cconsole->isActive() && dkiGetState(gameVar.k_showScore) || roundState != GAME_PLAYING)
    {
        showStats = true;
    }
    else
    {
        showStats = false;
    }

    for(i = 0; i < (int)nikeFlashes.size(); ++i)
    {
        nikeFlashes[i]->update(delay);
        if(nikeFlashes[i]->life <= 0)
        {
            delete nikeFlashes[i];
            nikeFlashes.erase(nikeFlashes.begin() + i);
            --i;
        }
    }

    if(thisPlayer && roundState == GAME_PLAYING)
    {
        if(thisPlayer->teamID == PLAYER_TEAM_SPECTATOR && !cconsole->isActive() && !writting && !showMenu && !(menuManager.root && menuManager.root->visible))
        {
            // On est spectateur, alors on peut se déplacer comme on veut dans la map
            // Pour l'instant les flèches (a,s,w,d, pomal temp)
            if(dkiGetState(gameVar.k_moveRight))
            {
                map->camLookAt[0] += 10 * delay;
            }
            if(dkiGetState(gameVar.k_moveLeft))
            {
                map->camLookAt[0] -= 10 * delay;
            }
            if(dkiGetState(gameVar.k_moveUp))
            {
                map->camLookAt[1] += 10 * delay;
            }
            if(dkiGetState(gameVar.k_moveDown))
            {
                map->camLookAt[1] -= 10 * delay;
            }
        }

        // On performe les collisions sur notre joueur
        if(thisPlayer->status == PLAYER_STATUS_ALIVE)
        {
            for(int i = 0; i < MAX_PLAYER; ++i)
            {
                if(players[i] && players[i] != thisPlayer)
                {
                    if(players[i]->status == PLAYER_STATUS_ALIVE && players[i]->timeAlive > 3.0f && thisPlayer->timeAlive > 3.0f) // player msut have been on the field for more than 3 second before we check collisions with him
                    {
                        float disSq = distanceSquared(thisPlayer->currentCF.position, players[i]->currentCF.position);
                        if(disSq <= .5f*.5f)
                        {
                            CVector3f dis = players[i]->currentCF.position - thisPlayer->currentCF.position;
                            normalize(dis);
                            thisPlayer->currentCF.position = players[i]->currentCF.position - dis * .51f;
                            thisPlayer->currentCF.vel = -thisPlayer->currentCF.vel * BOUNCE_FACTOR;
                            if(map) map->performCollision(thisPlayer->lastCF, thisPlayer->currentCF, .25f);
                            map->collisionClip(thisPlayer->currentCF, .25f);
                            thisPlayer->lastCF.position = thisPlayer->currentCF.position;
                        }
                    }
                }
            }

            if(map) map->performCollision(thisPlayer->lastCF, thisPlayer->currentCF, .25f);

            // Performing a final clip cibole de caliss
            map->collisionClip(thisPlayer->currentCF, .25f);

            //--- Est-ce qu'on est stuck dans un wall??? Oui? on respawn request
            int x = (int)thisPlayer->currentCF.position[0];
            int y = (int)thisPlayer->currentCF.position[1];
            if((!map->cells[(y)*map->size[0] + (x)].passable))
            {
                // Respawn request!
                if(!thisPlayer->spawnRequested)
                {
                    // Ici on le call juste une fois, isshh sinon ça sera pas trop bon...
                    // On request to spawn
                    thisPlayer->spawnRequested = true;
                    net_clsv_spawn_request spawnRequest;
                    spawnRequest.playerID = thisPlayer->playerID;
                    spawnRequest.weaponID = thisPlayer->nextSpawnWeapon;
                    spawnRequest.meleeID = thisPlayer->nextMeleeWeapon;
                    memcpy(spawnRequest.skin, thisPlayer->skin.s, (thisPlayer->skin.len() <= 6) ? thisPlayer->skin.len() + 1 : 7);
                    spawnRequest.blueDecal[0] = (unsigned char)(thisPlayer->blueDecal[0] * 255.0f);
                    spawnRequest.blueDecal[1] = (unsigned char)(thisPlayer->blueDecal[1] * 255.0f);
                    spawnRequest.blueDecal[2] = (unsigned char)(thisPlayer->blueDecal[2] * 255.0f);
                    spawnRequest.greenDecal[0] = (unsigned char)(thisPlayer->greenDecal[0] * 255.0f);
                    spawnRequest.greenDecal[1] = (unsigned char)(thisPlayer->greenDecal[1] * 255.0f);
                    spawnRequest.greenDecal[2] = (unsigned char)(thisPlayer->greenDecal[2] * 255.0f);
                    spawnRequest.redDecal[0] = (unsigned char)(thisPlayer->redDecal[0] * 255.0f);
                    spawnRequest.redDecal[1] = (unsigned char)(thisPlayer->redDecal[1] * 255.0f);
                    spawnRequest.redDecal[2] = (unsigned char)(thisPlayer->redDecal[2] * 255.0f);
                    bb_clientSend(scene->client->uniqueClientID, (char*)&spawnRequest, sizeof(net_clsv_spawn_request), NET_CLSV_SPAWN_REQUEST);
                }
            }
        }
    }

    // On update la map
    if(map && thisPlayer)
    {
        map->update(delay, thisPlayer);

        //--- Est-ce qu'il pleut?
        if(map->weather == WEATHER_RAIN)
        {
            for(int i = 0; i < 5; ++i)
            {
                //--- Spawn da rain!
                int idrip = getNextDrip();
                drips[idrip].life = 1;
                drips[idrip].position = rand(map->camPos + CVector3f(-5, -5, 0), map->camPos + CVector3f(5, 5, 0));
                drips[idrip].position[2] = 0;
                drips[idrip].size = .15f;
                drips[idrip].fadeSpeed = 2;
            }

            //--- Spawn des drip sous les players
            for(int i = 0; i < MAX_PLAYER; ++i)
            {
                if(players[i])
                {
                    if(players[i]->status == PLAYER_STATUS_ALIVE)
                    {
                        if(map->cells[(int)(players[i]->currentCF.position[1] - .5f) * map->size[0] + (int)(players[i]->currentCF.position[0] - .5f)].splater[0] > .5f)
                        {
                            if(players[i]->currentCF.vel.length() >= 2.25f)
                            {
                                //--- Spawn da rain!
                                int idrip = getNextDrip();
                                drips[idrip].life = .5f;
                                drips[idrip].position = players[i]->currentCF.position;
                                drips[idrip].position[2] = 0;
                                drips[idrip].size = .3f;
                                drips[idrip].fadeSpeed = 1;
                            }
                        }
                    }
                }
            }
        }

        //--- Si on roule dans la lave, on spawn de la fumé :D
        if(map->theme == THEME_LAVA)
        {
            //--- Spawn des drip sous les players
            for(int i = 0; i < MAX_PLAYER; ++i)
            {
                if(players[i])
                {
                    if(players[i]->status == PLAYER_STATUS_ALIVE && rand() % 50 == 5)
                    {
                        if(map->cells[(int)(players[i]->currentCF.position[1] - .5f) * map->size[0] + (int)(players[i]->currentCF.position[0] - .5f)].splater[0] > .5f)
                        {
                            //--- Spawn da smoke psssiiii
                            for(int j = 0; j < 4; ++j)
                            {
                                dkpCreateParticle(players[i]->currentCF.position.s,//float *position,
                                    CVector3f(0, 0, (float)j* .25f).s,//float *vel,
                                    CVector4f(.7f, .7f, .7f, 1).s,//float *startColor,
                                    CVector4f(.7f, .7f, .7f, 0).s,//float *endColor,
                                    .25f,//float startSize,
                                    .5f,//float endSize,
                                    2,//float duration,
                                    0,//float gravityInfluence,
                                    0,//float airResistanceInfluence,
                                    30,//float rotationSpeed,
                                    gameVar.tex_smoke1,//unsigned int texture,
                                    DKP_SRC_ALPHA,//unsigned int srcBlend,
                                    DKP_ONE_MINUS_SRC_ALPHA,//unsigned int dstBlend,
                                    0);//int transitionFunc);
                            }

                            dksPlay3DSound(gameVar.sfx_lavaSteam, -1, 5, players[i]->currentCF.position, 125);
                        }
                    }
                }
            }
        }

        // La view qui shake
        if(viewShake > 0)
        {
            if(viewShake > 2.5f) viewShake = 2.5f;

            CVector3f dir(1, 0, 0);
            dir = rotateAboutAxis(dir, rand(0.0f, 360.0f), CVector3f(0, 0, 1));
            dir *= viewShake * .10f;

            map->camPos += dir;
            viewShake -= delay * .75f;
            if(viewShake < 0) viewShake = 0;
        }


        //-- We check for all enable guns
        scene->client->btn_guns[WEAPON_SMG]->enable = gameVar.sv_enableSMG;
        scene->client->btn_guns[WEAPON_SHOTGUN]->enable = gameVar.sv_enableShotgun;
        scene->client->btn_guns[WEAPON_SNIPER]->enable = gameVar.sv_enableSniper;
        scene->client->btn_guns[WEAPON_DUAL_MACHINE_GUN]->enable = gameVar.sv_enableDualMachineGun;
        scene->client->btn_guns[WEAPON_CHAIN_GUN]->enable = gameVar.sv_enableChainGun;
        scene->client->btn_guns[WEAPON_BAZOOKA]->enable = gameVar.sv_enableBazooka;
        scene->client->btn_guns[WEAPON_PHOTON_RIFLE]->enable = gameVar.sv_enablePhotonRifle;
        scene->client->btn_guns[WEAPON_FLAME_THROWER]->enable = gameVar.sv_enableFlameThrower;

        if(gameVar.sv_enableSecondary)
        {
            scene->client->btn_meleeguns[WEAPON_KNIVES - WEAPON_KNIVES]->enable = gameVar.sv_enableKnives;
            scene->client->btn_meleeguns[WEAPON_SHIELD - WEAPON_KNIVES]->enable = gameVar.sv_enableShield;
        }
        else
        {
            scene->client->btn_meleeguns[WEAPON_KNIVES - WEAPON_KNIVES]->enable = false;
            scene->client->btn_meleeguns[WEAPON_SHIELD - WEAPON_KNIVES]->enable = false;
        }

    }

    // On update les trails
    for(int i = 0; i < (int)trails.size(); ++i)
    {
        trails[i]->update(delay);
        if(trails[i]->delay >= 1)
        {
            delete trails[i];
            trails.erase(trails.begin() + i);
            i--;
        }
    }

    // On update les floor mark
    for(int i = 0; i < MAX_FLOOR_MARK; ++i)
    {
        if(floorMarks[i].delay > 0)
        {
            floorMarks[i].update(delay);
        }
        if(drips[i].life > 0)
        {
            drips[i].update(delay);
        }
    }

    // On update les projectiles client
    for(int i = 0; i < (int)clientProjectiles.size(); ++i)
    {
        Projectile * projectile = clientProjectiles[i];
        projectile->update(delay, map);
        projectile->projectileID = (short)i; // On l'update toujours
        if(projectile->needToBeDeleted)
        {
            clientProjectiles.erase(projectiles.begin() + i);
            i--;
            delete projectile;
        }
    }

    // On update les douilles
    for(int i = 0; i < (int)douilles.size(); ++i)
    {
        Douille * douille = douilles[i];
        douille->update(delay, map);
        if(douille->delay <= 0)
        {
            douilles.erase(douilles.begin() + i);
            i--;
            delete douille;
        }
    }
}

void Douille::update(float pDelay, Map * map)
{
    CVector3f lastPos = position;
    delay -= pDelay;
    if(vel.length() > .5f)
    {
        position += vel * pDelay;
        vel[2] -= 9.8f * pDelay;
        CVector3f p1 = lastPos;
        CVector3f p2 = position;
        CVector3f normal;
        if(map->rayTest(p1, p2, normal))
        {
            // On dit à tout le monde de jouer le son (pour l'instant juste server side)
            if(!soundPlayed)
            {
                if(type == DOUILLE_TYPE_DOUILLE) dksPlay3DSound(gameVar.sfx_douille[rand() % 3], -1, 1, position, 255);
                else if(type == DOUILLE_TYPE_GIB)
                {
                    auto cgame = static_cast<ClientGame*>(scene->client->game);
                    cgame->spawnBlood(position, .1f);
                    //  delay = 0;
                }
                soundPlayed = true;
            }
            position = p2 + normal * .1f;
            vel = reflect(vel, normal);
            vel *= .3f;
        }
    }
}

//
// Pour ajouter une trainer d'une shot
//
void ClientGame::shoot(const CVector3f & position, const CVector3f & direction, float imp, float damage, Player * from, int projectileType)
{
    if(map)
    {
        CVector3f p2 = direction * 128; // Ça c'est le range, 128 c'est assez, grosseur max de map (c fucking big ça)
        if(projectileType == PROJECTILE_DIRECT && from->weapon->weaponID == WEAPON_FLAME_THROWER)
        {
            p2 = direction * 3;
        }
        p2 = rotateAboutAxis(p2, rand(-imp, imp), CVector3f(0, 0, 1));
        p2 = rotateAboutAxis(p2, rand(0.0f, 360.0f), direction);
        p2[2] *= .5f;
        p2 += position;

        if(projectileType == PROJECTILE_DIRECT && from->weapon)
        {
            net_clsv_player_shoot playerShoot;
            playerShoot.playerID = from->playerID;
            playerShoot.nuzzleID = (char)from->weapon->firingNuzzle;
            playerShoot.p1[0] = (short)(position[0] * 100);
            playerShoot.p1[1] = (short)(position[1] * 100);
            playerShoot.p1[2] = (short)(position[2] * 100);
            playerShoot.p2[0] = (short)(direction[0] * 100);
            playerShoot.p2[1] = (short)(direction[1] * 100);
            playerShoot.p2[2] = (short)(direction[2] * 100);
            playerShoot.weaponID = from->weapon->weaponID;
            bb_clientSend(scene->client->uniqueClientID, (char*)&playerShoot, sizeof(net_clsv_player_shoot), NET_CLSV_PLAYER_SHOOT);
        }
        else if(projectileType == PROJECTILE_ROCKET && from->weapon)
        {
            // On demande au server de créer une instance d'une rocket
            net_clsv_svcl_player_projectile playerProjectile;
            playerProjectile.playerID = from->playerID;
            playerProjectile.nuzzleID = (char)from->weapon->firingNuzzle;
            playerProjectile.projectileType = (char)projectileType;
            playerProjectile.weaponID = from->weapon->weaponID;
            playerProjectile.position[0] = (short)(position[0] * 100.0f);
            playerProjectile.position[1] = (short)(position[1] * 100.0f);
            playerProjectile.position[2] = (short)(position[2] * 100.0f);
            //  CVector3f dir = from->currentCF.mousePosOnMap - position; // Pas une bonne idée ça, trop facile
            //  normalize(dir);
            playerProjectile.vel[0] = (char)(direction[0] * 10.0f);
            playerProjectile.vel[1] = (char)(direction[1] * 10.0f);
            playerProjectile.vel[2] = (char)(direction[2] * 10.0f);
            bb_clientSend(scene->client->uniqueClientID, (char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE);

            // duplicate rocket was for hacking test only
            //playerProjectile.vel[1] = (char)(direction[0] * 10.0f);
            //playerProjectile.vel[2] = (char)(direction[1] * 10.0f);
            //playerProjectile.vel[0] = (char)(direction[2] * 10.0f);

            //bb_clientSend(scene->client->uniqueClientID, (char*)&playerProjectile,sizeof(net_clsv_svcl_player_projectile),NET_CLSV_SVCL_PLAYER_PROJECTILE);
        }
        else if(projectileType == PROJECTILE_GRENADE)
        {
            //  for (int i=0;i<20;++i)
            //  {
                    // On demande au server de créer une instance d'une grenade
            net_clsv_svcl_player_projectile playerProjectile;
            playerProjectile.playerID = from->playerID;
            playerProjectile.nuzzleID = (char)from->weapon->firingNuzzle;
            playerProjectile.projectileType = (char)projectileType;
            playerProjectile.weaponID = WEAPON_GRENADE;
            playerProjectile.position[0] = (short)(position[0] * 100.0f);
            playerProjectile.position[1] = (short)(position[1] * 100.0f);
            playerProjectile.position[2] = (short)(position[2] * 100.0f);
            //  CVector3f dir = from->currentCF.mousePosOnMap - position; // Pas une bonne idée ça, trop facile
            //  normalize(dir);
            playerProjectile.vel[0] = (char)(direction[0] * 10.0f);
            playerProjectile.vel[1] = (char)(direction[1] * 10.0f);
            playerProjectile.vel[2] = (char)(direction[2] * 10.0f);
            bb_clientSend(scene->client->uniqueClientID, (char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE);
            //  }
        }
        else if(projectileType == PROJECTILE_COCKTAIL_MOLOTOV)
        {
            // On demande au server de créer une instance d'une grenade
            net_clsv_svcl_player_projectile playerProjectile;
            playerProjectile.playerID = from->playerID;
            playerProjectile.nuzzleID = (char)from->weapon->firingNuzzle;
            playerProjectile.projectileType = (char)projectileType;
            playerProjectile.weaponID = WEAPON_COCKTAIL_MOLOTOV;
            playerProjectile.position[0] = (short)(position[0] * 100.0f);
            playerProjectile.position[1] = (short)(position[1] * 100.0f);
            playerProjectile.position[2] = (short)(position[2] * 100.0f);
            //  CVector3f dir = from->currentCF.mousePosOnMap - position; // Pas une bonne idée ça, trop facile
            //  normalize(dir);
            playerProjectile.vel[0] = (char)(direction[0] * 10.0f);
            playerProjectile.vel[1] = (char)(direction[1] * 10.0f);
            playerProjectile.vel[2] = (char)(direction[2] * 10.0f);
            bb_clientSend(scene->client->uniqueClientID, (char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE);
        }
    }
}

void ClientGame::createNewPlayerCL(int playerID, long babonetID)
{
    if(playerID < 0 || playerID >= MAX_PLAYER)
    {
        console->add(CString("\x4> Error : invalid playerID : %i", playerID));
    }
    else
    {
        // On l'efface au cas quil existe déjà (très pas bon ça)
        ZEVEN_SAFE_DELETE(players[playerID]);
        players[playerID] = new Player(playerID, map, this);
        players[playerID]->babonetID = babonetID;
    }
}

//
// Pour spawner des particules sur le murs l'hors d'un impact
//
void ClientGame::spawnImpact(CVector3f & p1, CVector3f & p2, CVector3f & normal, Weapon*weapon, float damage, int team)
{
    // Bon, ben on spawn des particules là
    CVector3f front = normal;
    CVector3f right, up;
    createRightUpVectors(right, front, up);
    CMatrix3x3f mat;
    mat.setRight(right);
    mat.setFront(front);
    mat.setUp(up);
    mat.RotateAboutFront(rand(0.0f, 360.0f));
    mat.RotateAboutRight(rand(-30.0f, 30.0f));

    int type = 0;
    if(weapon->weaponID == WEAPON_PHOTON_RIFLE) type = 1;

    // On se cré une trail yo yea
    if(type == 0)
    {
        trails.push_back(new Trail(p1, p2, damage, CVector4f((team == PLAYER_TEAM_RED) ? .9f : .5f, .5f, (team == PLAYER_TEAM_BLUE) ? .9f : .5f, 1), damage * 4, 0));
    }
    else if(type == 1)
    {
        if(weapon->weaponID == WEAPON_PHOTON_RIFLE) damage = 2.0f;
        trails.push_back(new Trail(p1, p2, damage, CVector4f((team == PLAYER_TEAM_RED) ? .9f : .25f, .25f, (team == PLAYER_TEAM_BLUE) ? .9f : .25f, 1), damage * 4, 0));
        trails.push_back(new Trail(p1, p2, damage / 4, CVector4f((team == PLAYER_TEAM_RED) ? .9f : .25f, .25f, (team == PLAYER_TEAM_BLUE) ? .9f : .25f, 1), damage, 1));
        trails.push_back(new Trail(p1, p2, damage / 8, CVector4f((team == PLAYER_TEAM_RED) ? .9f : .25f, .25f, (team == PLAYER_TEAM_BLUE) ? .9f : .25f, 1), damage, 1));
        trails.push_back(new Trail(p1, p2, damage / 16, CVector4f((team == PLAYER_TEAM_RED) ? .9f : .25f, .25f, (team == PLAYER_TEAM_BLUE) ? .9f : .25f, 1), damage, 1));
    }

    gameVar.ro_hitPoint = p2;

    dkpCreateParticle(p2.s,//float *position,
        (mat.getFront()*.0f).s,//float *vel,
        CVector4f(1, 1, 0, 1).s,//float *startColor,
        CVector4f(1, 1, 0, 0.0f).s,//float *endColor,
        .15f,//float startSize,
        .15f,//float endSize,
        .3f,//float duration,
        -.1f,//float gravityInfluence,
        0,//float airResistanceInfluence,
        0,//float rotationSpeed,
        gameVar.tex_shotGlow,//unsigned int texture,
        DKP_SRC_ALPHA,//unsigned int srcBlend,
        DKP_ONE,//unsigned int dstBlend,
        0);//int transitionFunc);

    dkpCreateParticle(p2.s,//float *position,
        (mat.getFront()*.25f).s,//float *vel,
        CVector4f(1, 1, 1, .5f).s,//float *startColor,
        CVector4f(1, 1, 1, 0.0f).s,//float *endColor,
        0,//float startSize,
        .5f,//float endSize,
        1,//float duration,
        -.1f,//float gravityInfluence,
        0,//float airResistanceInfluence,
        90,//float rotationSpeed,
        gameVar.tex_smoke1,//unsigned int texture,
        DKP_SRC_ALPHA,//unsigned int srcBlend,
        DKP_ONE_MINUS_SRC_ALPHA,//unsigned int dstBlend,
        0);//int transitionFunc);
    dkpCreateParticle(p2.s,//float *position,
        (mat.getFront()*2.0f).s,//float *vel,
        CVector4f(1, 1, 1, .75f).s,//float *startColor,
        CVector4f(1, 1, 1, 0.0f).s,//float *endColor,
        .125f,//float startSize,
        .25f,//float endSize,
        .5f,//float duration,
        0,//float gravityInfluence,
        0,//float airResistanceInfluence,
        90,//float rotationSpeed,
        gameVar.tex_smoke1,//unsigned int texture,
        DKP_SRC_ALPHA,//unsigned int srcBlend,
        DKP_ONE_MINUS_SRC_ALPHA,//unsigned int dstBlend,
        0);//int transitionFunc);
    dkpCreateParticle(p2.s,//float *position,
        (mat.getFront()*4.0f).s,//float *vel,
        CVector4f(1, 1, 1, .75f).s,//float *startColor,
        CVector4f(1, 1, 1, 0.0f).s,//float *endColor,
        .125f,//float startSize,
        .25f,//float endSize,
        .25f,//float duration,
        0,//float gravityInfluence,
        0,//float airResistanceInfluence,
        90,//float rotationSpeed,
        gameVar.tex_smoke1,//unsigned int texture,
        DKP_SRC_ALPHA,//unsigned int srcBlend,
        DKP_ONE_MINUS_SRC_ALPHA,//unsigned int dstBlend,
        0);//int transitionFunc);

// on ricoche!
    dksPlay3DSound(gameVar.sfx_ric[rand() % 5], -1, 2, p2, 150);
}

void ClientGame::spawnExplosion(CVector3f & position, CVector3f & normal, float size)
{
    if(size >= 4.0f)
    {
        NukeFlash* nukeFlash = new NukeFlash();
        nukeFlash->position = position;
        nukeFlash->radius = size * 3;
        nikeFlashes.push_back(nukeFlash);
    }

    float trueSize = size;
    size *= 3;

    // Make it sounds and shake !!!
    dksPlay3DSound(gameVar.sfx_explosion[0], -1, 10, position, 255);
    //int channel = FSOUND_PlaySoundEx(-1, gameVar.sfx_explosion[0], 0, TRUE);
    //FSOUND_3D_SetMinMaxDistance(channel, 10, 10000000.0f);
    //FSOUND_3D_SetAttributes(channel, position.s, 0);
    //if (trueSize >= 4.0f)
    //{
    //  FSOUND_SetFrequency(channel, 5000);
    //}
    //else
    //{
    //  FSOUND_SetFrequency(channel, 22050);
    //}
    //FSOUND_SetVolume(channel, 255);
    //FSOUND_SetPaused(channel, FALSE);

    float duration = size * .5f;

    float maxDuration = gameVar.r_reducedParticles ? 1.0f : 10.0f;

    if(duration > maxDuration) duration = maxDuration;

    dkpCreateParticleEx(
        position, //CVector3f & positionFrom,
        position, //CVector3f & positionTo,
        normal, //CVector3f & direction,
        .5f, //float speedFrom,
        .5f, //float speedTo,
        80, //float pitchFrom,
        90, //float pitchTo,
        size*.03f, //float startSizeFrom,
        size*.03f, //float startSizeTo,
        size*.2f, //float endSizeFrom,
        size*.2f, //float endSizeTo,
        duration*.5f, //float durationFrom,
        duration*.5f, //float durationTo,
        CColor4f(0, 0, 0, 1), //CColor4f & startColorFrom,
        CColor4f(0, 0, 0, 1), //CColor4f & startColorTo,
        CColor4f(.7f, .7f, .7f, 0), //CColor4f & endColorFrom,
        CColor4f(.7f, .7f, .7f, 0), //CColor4f & endColorTo,
        0, //float angleFrom,
        360, //float angleTo,
        0, //float angleSpeedFrom,
        30, //float angleSpeedTo,
        0,//.025f, //float gravityInfluence,
        1, //float airResistanceInfluence,
        20, //unsigned int particleCountFrom,
        20, //unsigned int particleCountTo,
        &(gameVar.tex_smoke1),
        0, //int textureFrameCount,
        DKP_SRC_ALPHA, //unsigned int srcBlend,
        DKP_ONE_MINUS_SRC_ALPHA);//unsigned int dstBlend);

    dkpCreateParticleEx(
        position, //CVector3f & positionFrom,
        position, //CVector3f & positionTo,
        normal, //CVector3f & direction,
        .05f, //float speedFrom,
        .55f, //float speedTo,
        0, //float pitchFrom,
        45, //float pitchTo,
        size*.03f, //float startSizeFrom,
        size*.03f, //float startSizeTo,
        size*.2f, //float endSizeFrom,
        size*.2f, //float endSizeTo,
        duration, //float durationFrom,
        duration, //float durationTo,
        CColor4f(0, 0, 0, 1), //CColor4f & startColorFrom,
        CColor4f(0, 0, 0, 1), //CColor4f & startColorTo,
        CColor4f(.7f, .7f, .7f, 0), //CColor4f & endColorFrom,
        CColor4f(.7f, .7f, .7f, 0), //CColor4f & endColorTo,
        0, //float angleFrom,
        360, //float angleTo,
        0, //float angleSpeedFrom,
        30, //float angleSpeedTo,
        0,//.025f, //float gravityInfluence,
        1, //float airResistanceInfluence,
        10, //unsigned int particleCountFrom,
        10, //unsigned int particleCountTo,
        &(gameVar.tex_smoke1),
        0, //int textureFrameCount,
        DKP_SRC_ALPHA, //unsigned int srcBlend,
        DKP_ONE_MINUS_SRC_ALPHA);//unsigned int dstBlend);

    dkpCreateParticleEx(
        position, //CVector3f & positionFrom,
        position, //CVector3f & positionTo,
        normal, //CVector3f & direction,
        1.5f, //float speedFrom,
        3.5f, //float speedTo,
        0, //float pitchFrom,
        90, //float pitchTo,
        size*.01f, //float startSizeFrom,
        size*.01f, //float startSizeTo,
        size*.02f, //float endSizeFrom,
        size*.02f, //float endSizeTo,
        duration*.1f, //float durationFrom,
        duration*.1f, //float durationTo,
        CColor4f(1, 0, 0, 1), //CColor4f & startColorFrom,
        CColor4f(1, 1, 0, 1), //CColor4f & startColorTo,
        CColor4f(1, 1, 0, 0), //CColor4f & endColorFrom,
        CColor4f(1, 1, 0, 0), //CColor4f & endColorTo,
        0, //float angleFrom,
        360, //float angleTo,
        0, //float angleSpeedFrom,
        30, //float angleSpeedTo,
        0, //float gravityInfluence,
        1, //float airResistanceInfluence,
        20, //unsigned int particleCountFrom,
        20, //unsigned int particleCountTo,
        &(gameVar.tex_smoke1),
        0, //int textureFrameCount,
        DKP_SRC_ALPHA, //unsigned int srcBlend,
        DKP_ONE);//unsigned int dstBlend);

    dkpCreateParticleEx(
        position, //CVector3f & positionFrom,
        position, //CVector3f & positionTo,
        normal, //CVector3f & direction,
        .05f, //float speedFrom,
        .35f, //float speedTo,
        0, //float pitchFrom,
        90, //float pitchTo,
        size*.05f, //float startSizeFrom,
        size*.05f, //float startSizeTo,
        size*.1f, //float endSizeFrom,
        size*.1f, //float endSizeTo,
        duration*.1f, //float durationFrom,
        duration*.1f, //float durationTo,
        CColor4f(1, 0, 0, 1), //CColor4f & startColorFrom,
        CColor4f(1, 1, 0, 1), //CColor4f & startColorTo,
        CColor4f(1, 1, 0, 0), //CColor4f & endColorFrom,
        CColor4f(1, 1, 0, 0), //CColor4f & endColorTo,
        0, //float angleFrom,
        360, //float angleTo,
        0, //float angleSpeedFrom,
        30, //float angleSpeedTo,
        0, //float gravityInfluence,
        1, //float airResistanceInfluence,
        5, //unsigned int particleCountFrom,
        5, //unsigned int particleCountTo,
        &(gameVar.tex_smoke1),
        0, //int textureFrameCount,
        DKP_SRC_ALPHA, //unsigned int srcBlend,
        DKP_ONE);//unsigned int dstBlend);

    dkpCreateParticleEx(
        position - CVector3f(trueSize*.35f, trueSize*.35f, 0), //CVector3f & positionFrom,
        position + CVector3f(trueSize*.35f, trueSize*.35f, 0), //CVector3f & positionTo,
        normal, //CVector3f & direction,
        0, //float speedFrom,
        0, //float speedTo,
        0, //float pitchFrom,
        90, //float pitchTo,
        trueSize*.25f, //float startSizeFrom,
        trueSize*.25f, //float startSizeTo,
        trueSize*.5f, //float endSizeFrom,
        trueSize*.5f, //float endSizeTo,
        .25f, //float durationFrom,
        .25f, //float durationTo,
        CColor4f(1, 0, 0, 1), //CColor4f & startColorFrom,
        CColor4f(1, 1, 0, 1), //CColor4f & startColorTo,
        CColor4f(1, 0, 0, 0), //CColor4f & endColorFrom,
        CColor4f(1, 1, 0, 0), //CColor4f & endColorTo,
        0, //float angleFrom,
        360, //float angleTo,
        -180, //float angleSpeedFrom,
        180, //float angleSpeedTo,
        0, //float gravityInfluence,
        1, //float airResistanceInfluence,
        8, //unsigned int particleCountFrom,
        8, //unsigned int particleCountTo,
        &(gameVar.tex_smoke1),
        0, //int textureFrameCount,
        DKP_SRC_ALPHA, //unsigned int srcBlend,
        DKP_ONE);//unsigned int dstBlend);

    floorMarks[getNextFloorMark()].set(CVector3f(position[0], position[1], 0), rand(0.0f, 360.0f), size*.18f, 30, 0, gameVar.tex_explosionMark, CVector4f(1, 1, 1, .5f));

    /*if (thisPlayer)
    {

        // La distance
        float dis = distance(position, thisPlayer->currentCF.position);
        if (dis < trueSize*2)
        {
            viewShake += (1-(dis / (trueSize*2))) * 2;
        }
    }// screen shake moved elsewhere (so that it shakes only when you are hit!) */
}
