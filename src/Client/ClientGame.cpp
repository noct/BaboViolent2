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

#include "ClientGame.h"
#include "netPacket.h"
#include "Console.h"
#include "GameVar.h"
#include "Client.h"
#include "ClientConsole.h"
#include "Server.h"
#include "ClientScene.h"
#include <time.h>
#include <algorithm>
#include "Client.h"
#include "ClientMap.h"

NukeFlash::NukeFlash()
{
    life = 1;
    density = 2;
    fadeSpeed = .50f;
    radius = 16;
}
void NukeFlash::update(float pdelay)
{
    life -= pdelay * fadeSpeed;
}

Drip::Drip()
{
    life = 1;
    size = .15f;
    fadeSpeed = 2;
}
void Drip::update(float pdelay)
{
    life -= pdelay * fadeSpeed;
}
FloorMark::FloorMark()
{
    delay = 0;
}

void FloorMark::set(CVector3f & pposition, float pangle, float psize, float pdelay, float pstartDelay, unsigned int ptexture, CVector4f pcolor)
{
    position = pposition;
    angle = pangle;
    size = psize;
    delay = pdelay;
    startDelay = pstartDelay;
    texture = ptexture;
    color = pcolor;
}

void FloorMark::update(float pdelay)
{
    if (startDelay > 0)
    {
        startDelay -= pdelay;
    }
    else
    {
        delay -= pdelay;
    }
}

Douille::Douille(CVector3f pPosition, CVector3f pDirection, CVector3f right, int in_type)
{
    type = in_type;
    vel = pDirection * 1.5f;
    if (type == DOUILLE_TYPE_DOUILLE)
    {
        delay = 2; // Ça dure 2sec ça, en masse
        vel = rotateAboutAxis(vel, rand(-30.0f, 30.0f), right);
        vel = rotateAboutAxis(vel, rand(0.0f, 360.0f), pDirection);
    }
    else if (type == DOUILLE_TYPE_GIB)
    {
        delay = 2; // 5 sec haha malade
    }
    position = pPosition;
    soundPlayed = false;
}

Trail::Trail(CVector3f & pP1, CVector3f & pP2, float pSize, CVector4f & pColor, float duration, int in_trailType)
{
    trailType = in_trailType;
    dis = distance(pP1, pP2);
    delay = 0;
    delaySpeed = 1.0f / (duration);
    p1 = pP1;
    p2 = pP2;
    size = pSize;
    color = pColor;
    right = cross(p2 - p1, CVector3f(0,0,1));
    normalize(right);
    offset = rand(0.0f, 1.0f);
}

void Trail::update(float pDelay)
{
    if (delay > 0)
    {
        delay += pDelay * delaySpeed;
    }
    else
    {
        delay = .001f;
    }
}

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
            clientVar.tex_blood + rand() % 10,
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
        floorMarks[getNextFloorMark()].set(pos, rand(0.0f, 360.0f), rand(.05f, sizeMax), 30, distance*0.5f, clientVar.tex_blood[rand() % 10], CVector4f(rand(.25f, 0.5f), 0, 0, rand(.5f, 1.0f)));
    }
}

extern Scene * scene;

ClientGame::ClientGame(CString pMapName): Game(pMapName), mapBuffer(8192)
{
    font = dkfCreateFont("main/fonts/babo.png");
    tex_miniMapAllied = dktCreateTextureFromFile("main/textures/MiniMapAllied.png", DKT_FILTER_LINEAR);
    tex_miniMapEnemy = dktCreateTextureFromFile("main/textures/MiniMapEnemy.png", DKT_FILTER_LINEAR);;
    tex_baboShadow = dktCreateTextureFromFile("main/textures/BaboShadow.png", DKT_FILTER_BILINEAR);

    // On ne cré pas notre player tout de suite, on attends confirmation du server
    thisPlayer = 0;

    // Pour afficher les stats multiplayers
    showStats = false;

    dkpReset();

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

    CString mapFilename = CString("main/maps/") + mapName + ".bvm";

    if(scene->server)
    {
        FileIO file(mapFilename, "rb");
        map = new ClientMap(mapName, &file, isServerGame, font);
    }
    else
    {
        map = new ClientMap(mapName, &mapBuffer, isServerGame, font);
    }

    if(!map->isValid)
    {
        console->add("\x4> Invalid map");
        ZEVEN_SAFE_DELETE(map);
        auto cscene = static_cast<ClientScene*>(scene);
        if(cscene->client)
        {
            cscene->client->needToShutDown = true;
            cscene->client->isRunning = false;
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

    //--- Reset timer
    dkcJumpToFrame(scene->ctx, 0);
}


void ClientGame::update(float delay)
{
    auto cscene = static_cast<ClientScene*>(scene);
    auto client = cscene->client;
    auto cmap = static_cast<ClientMap*>(map);
    int i;
    Game::update(delay);


    dotAnim += delay * 720;
    while(dotAnim >= 360) dotAnim -= 360;


    auto cconsole = static_cast<ClientConsole*>(console);
    if(!cconsole->isActive() && dkiGetState(clientVar.k_showScore) || roundState != GAME_PLAYING)
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
            if(dkiGetState(clientVar.k_moveRight))
            {
                cmap->camLookAt[0] += 10 * delay;
            }
            if(dkiGetState(clientVar.k_moveLeft))
            {
                cmap->camLookAt[0] -= 10 * delay;
            }
            if(dkiGetState(clientVar.k_moveUp))
            {
                cmap->camLookAt[1] += 10 * delay;
            }
            if(dkiGetState(clientVar.k_moveDown))
            {
                cmap->camLookAt[1] -= 10 * delay;
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
                            if(map) cmap->performCollision(thisPlayer->lastCF, thisPlayer->currentCF, .25f);
                            cmap->collisionClip(thisPlayer->currentCF, .25f);
                            thisPlayer->lastCF.position = thisPlayer->currentCF.position;
                        }
                    }
                }
            }

            if(map) cmap->performCollision(thisPlayer->lastCF, thisPlayer->currentCF, .25f);

            // Performing a final clip cibole de caliss
            cmap->collisionClip(thisPlayer->currentCF, .25f);

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
                    bb_clientSend(cscene->client->uniqueClientID, (char*)&spawnRequest, sizeof(net_clsv_spawn_request), NET_CLSV_SPAWN_REQUEST);
                }
            }
        }
    }

    // On update la map
    if(cmap && thisPlayer)
    {
        Player* flagPlayer0 = players[map->flagState[0]];
        Player* flagPlayer1 = players[map->flagState[1]];
        cmap->update(delay, thisPlayer, flagPlayer0, flagPlayer1);

        //--- Est-ce qu'il pleut?
        if(cmap->weather == WEATHER_RAIN)
        {
            for(int i = 0; i < 5; ++i)
            {
                //--- Spawn da rain!
                int idrip = getNextDrip();
                drips[idrip].life = 1;
                drips[idrip].position = rand(cmap->camPos + CVector3f(-5, -5, 0), cmap->camPos + CVector3f(5, 5, 0));
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
        if(cmap->theme == THEME_LAVA)
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
                                    clientVar.tex_smoke1,//unsigned int texture,
                                    DKP_SRC_ALPHA,//unsigned int srcBlend,
                                    DKP_ONE_MINUS_SRC_ALPHA,//unsigned int dstBlend,
                                    0);//int transitionFunc);
                            }

                            dksPlay3DSound(clientVar.sfx_lavaSteam, -1, 5, players[i]->currentCF.position, 125);
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

            cmap->camPos += dir;
            viewShake -= delay * .75f;
            if(viewShake < 0) viewShake = 0;
        }


        //-- We check for all enable guns
        client->btn_guns[WEAPON_SMG]->enable = gameVar.sv_enableSMG;
        client->btn_guns[WEAPON_SHOTGUN]->enable = gameVar.sv_enableShotgun;
        client->btn_guns[WEAPON_SNIPER]->enable = gameVar.sv_enableSniper;
        client->btn_guns[WEAPON_DUAL_MACHINE_GUN]->enable = gameVar.sv_enableDualMachineGun;
        client->btn_guns[WEAPON_CHAIN_GUN]->enable = gameVar.sv_enableChainGun;
        client->btn_guns[WEAPON_BAZOOKA]->enable = gameVar.sv_enableBazooka;
        client->btn_guns[WEAPON_PHOTON_RIFLE]->enable = gameVar.sv_enablePhotonRifle;
        client->btn_guns[WEAPON_FLAME_THROWER]->enable = gameVar.sv_enableFlameThrower;

        if(gameVar.sv_enableSecondary)
        {
            client->btn_meleeguns[WEAPON_KNIVES - WEAPON_KNIVES]->enable = gameVar.sv_enableKnives;
            client->btn_meleeguns[WEAPON_SHIELD - WEAPON_KNIVES]->enable = gameVar.sv_enableShield;
        }
        else
        {
            client->btn_meleeguns[WEAPON_KNIVES - WEAPON_KNIVES]->enable = false;
            client->btn_meleeguns[WEAPON_SHIELD - WEAPON_KNIVES]->enable = false;
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
                if(type == DOUILLE_TYPE_DOUILLE) dksPlay3DSound(clientVar.sfx_douille[rand() % 3], -1, 1, position, 255);
                else if(type == DOUILLE_TYPE_GIB)
                {
                    auto cscene = static_cast<ClientScene*>(scene);
                    auto cgame = static_cast<ClientGame*>(cscene->client->game);
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
    auto cscene = static_cast<ClientScene*>(scene);
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
            auto cweapon = static_cast<ClientWeapon*>(from->weapon);
            net_clsv_player_shoot playerShoot;
            playerShoot.playerID = from->playerID;
            playerShoot.nuzzleID = (char)cweapon->firingNuzzle;
            playerShoot.p1[0] = (short)(position[0] * 100);
            playerShoot.p1[1] = (short)(position[1] * 100);
            playerShoot.p1[2] = (short)(position[2] * 100);
            playerShoot.p2[0] = (short)(direction[0] * 100);
            playerShoot.p2[1] = (short)(direction[1] * 100);
            playerShoot.p2[2] = (short)(direction[2] * 100);
            playerShoot.weaponID = from->weapon->weaponID;
            bb_clientSend(cscene->client->uniqueClientID, (char*)&playerShoot, sizeof(net_clsv_player_shoot), NET_CLSV_PLAYER_SHOOT);
        }
        else if(projectileType == PROJECTILE_ROCKET && from->weapon)
        {
            auto cweapon = static_cast<ClientWeapon*>(from->weapon);
            // On demande au server de créer une instance d'une rocket
            net_clsv_svcl_player_projectile playerProjectile;
            playerProjectile.playerID = from->playerID;
            playerProjectile.nuzzleID = (char)cweapon->firingNuzzle;
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
            bb_clientSend(cscene->client->uniqueClientID, (char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE);

            // duplicate rocket was for hacking test only
            //playerProjectile.vel[1] = (char)(direction[0] * 10.0f);
            //playerProjectile.vel[2] = (char)(direction[1] * 10.0f);
            //playerProjectile.vel[0] = (char)(direction[2] * 10.0f);

            //bb_clientSend(scene->client->uniqueClientID, (char*)&playerProjectile,sizeof(net_clsv_svcl_player_projectile),NET_CLSV_SVCL_PLAYER_PROJECTILE);
        }
        else if(projectileType == PROJECTILE_GRENADE)
        {
            auto cweapon = static_cast<ClientWeapon*>(from->weapon);
            //  for (int i=0;i<20;++i)
            //  {
                    // On demande au server de créer une instance d'une grenade
            net_clsv_svcl_player_projectile playerProjectile;
            playerProjectile.playerID = from->playerID;
            playerProjectile.nuzzleID = (char)cweapon->firingNuzzle;
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
            bb_clientSend(cscene->client->uniqueClientID, (char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE);
            //  }
        }
        else if(projectileType == PROJECTILE_COCKTAIL_MOLOTOV)
        {
            auto cweapon = static_cast<ClientWeapon*>(from->weapon);
            // On demande au server de créer une instance d'une grenade
            net_clsv_svcl_player_projectile playerProjectile;
            playerProjectile.playerID = from->playerID;
            playerProjectile.nuzzleID = (char)cweapon->firingNuzzle;
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
            bb_clientSend(cscene->client->uniqueClientID, (char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE);
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
        players[playerID] = new ClientPlayer(playerID, map, this);
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

    clientVar.ro_hitPoint = p2;

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
        clientVar.tex_shotGlow,//unsigned int texture,
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
        clientVar.tex_smoke1,//unsigned int texture,
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
        clientVar.tex_smoke1,//unsigned int texture,
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
        clientVar.tex_smoke1,//unsigned int texture,
        DKP_SRC_ALPHA,//unsigned int srcBlend,
        DKP_ONE_MINUS_SRC_ALPHA,//unsigned int dstBlend,
        0);//int transitionFunc);

// on ricoche!
    dksPlay3DSound(clientVar.sfx_ric[rand() % 5], -1, 2, p2, 150);
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
    dksPlay3DSound(clientVar.sfx_explosion[0], -1, 10, position, 255);
    //int channel = FSOUND_PlaySoundEx(-1, clientVar.sfx_explosion[0], 0, TRUE);
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

    float maxDuration = 10.0f;

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
        &(clientVar.tex_smoke1),
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
        &(clientVar.tex_smoke1),
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
        &(clientVar.tex_smoke1),
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
        &(clientVar.tex_smoke1),
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
        &(clientVar.tex_smoke1),
        0, //int textureFrameCount,
        DKP_SRC_ALPHA, //unsigned int srcBlend,
        DKP_ONE);//unsigned int dstBlend);

    floorMarks[getNextFloorMark()].set(CVector3f(position[0], position[1], 0), rand(0.0f, 360.0f), size*.18f, 30, 0, clientVar.tex_explosionMark, CVector4f(1, 1, 1, .5f));

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

void ClientGame::castVote(const net_clsv_svcl_vote_request & voteRequest)
{
    Game::castVote(voteRequest);

    if(thisPlayer && (thisPlayer->teamID != PLAYER_TEAM_AUTO_ASSIGN) && (thisPlayer->teamID != PLAYER_TEAM_SPECTATOR))
        voting.voted = false;
    else
        voting.voted = true;
}

void ClientGame::onTeamSwitch(Player* player)
{
    auto cscene = static_cast<ClientScene*>(scene);
    Game::onTeamSwitch(player);
    // On print dans console
    switch(player->teamID)
    {
    case PLAYER_TEAM_SPECTATOR:
        if(cscene->client) cscene->client->eventMessages.push_back(CString(clientVar.lang_goesSpectator.s, player->name.s));
        break;
    case PLAYER_TEAM_BLUE:
        if(cscene->client) cscene->client->eventMessages.push_back(CString(clientVar.lang_joinBlueTeamP.s, player->name.s));
        break;
    case PLAYER_TEAM_RED:
        if(cscene->client) cscene->client->eventMessages.push_back(CString(clientVar.lang_joinRedTeamP.s, player->name.s));
        break;
    }
}

void ClientGame::onSpawnPlayer(Player* player)
{
    auto cmap = static_cast<ClientMap*>(map);
    cmap->setCameraPos(player->currentCF.position);
}

void ClientGame::spawnProjectileSpecific(CVector3f & position, CVector3f & vel, char pFromID, int pProjectileType, bool pRemoteEntity, long pUniqueProjectileID)
{
    Projectile* projectile = new ClientProjectile(position, vel, pFromID, pProjectileType, pRemoteEntity, pUniqueProjectileID);
    projectiles.push_back(projectile);
}

bool ClientGame::spawnProjectile(net_clsv_svcl_player_projectile & playerProjectile, bool imServer)
{
    if(imServer)
    {
        return Game::spawnProjectile(playerProjectile, imServer);
    }
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
    }
    return true;
}


ClientProjectile::ClientProjectile(CVector3f & position, CVector3f & vel, char pFromID, int pProjectileType, bool pRemoteEntity, long pUniqueProjectileID)
    : Projectile(position, vel, pFromID, pProjectileType, pRemoteEntity, pUniqueProjectileID)
{
    spawnParticleTime = 0;
    rotation = 0;

    switch(projectileType)
    {
    case PROJECTILE_ROCKET:
    {
        rotateVel = 360;
        if(remoteEntity)
        {
            for(int i = 0; i < 10; ++i)
            {
                // On spawn des particules dans son cul (une par frame)
                dkpCreateParticleEx(
                    position - vel, //CVector3f & positionFrom,
                    position - vel, //CVector3f & positionTo,
                    -vel, //CVector3f & direction,
                    1, //float speedFrom,
                    2, //float speedTo,
                    0, //float pitchFrom,
                    45, //float pitchTo,
                    .05f, //float startSizeFrom,
                    .25f, //float startSizeTo,
                    .25f, //float endSizeFrom,
                    .45f, //float endSizeTo,
                    .5f, //float durationFrom,
                    2, //float durationTo,
                    CColor4f(.5f, .5f, .5f, 1), //CColor4f & startColorFrom,
                    CColor4f(.5f, .5f, .5f, 1), //CColor4f & startColorTo,
                    CColor4f(.5f, .5f, .5f, 0), //CColor4f & endColorFrom,
                    CColor4f(.5f, .5f, .5f, 0), //CColor4f & endColorTo,
                    0, //float angleFrom,
                    360, //float angleTo,
                    -30, //float angleSpeedFrom,
                    30, //float angleSpeedTo,
                    0, //float gravityInfluence,
                    .25f, //float airResistanceInfluence,
                    5, //unsigned int particleCountFrom,
                    5, //unsigned int particleCountTo,
                    &clientVar.tex_smoke1,
                    0, //int textureFrameCount,
                    DKP_SRC_ALPHA, //unsigned int srcBlend,
                    DKP_ONE_MINUS_SRC_ALPHA);//unsigned int dstBlend);
                // On spawn des particules dans son cul (une par frame)
                dkpCreateParticleEx(
                    position, //CVector3f & positionFrom,
                    position, //CVector3f & positionTo,
                    vel, //CVector3f & direction,
                    1, //float speedFrom,
                    2, //float speedTo,
                    0, //float pitchFrom,
                    45, //float pitchTo,
                    .05f, //float startSizeFrom,
                    .25f, //float startSizeTo,
                    .25f, //float endSizeFrom,
                    .45f, //float endSizeTo,
                    .5f, //float durationFrom,
                    2, //float durationTo,
                    CColor4f(.5f, .5f, .5f, 1), //CColor4f & startColorFrom,
                    CColor4f(.5f, .5f, .5f, 1), //CColor4f & startColorTo,
                    CColor4f(.5f, .5f, .5f, 0), //CColor4f & endColorFrom,
                    CColor4f(.5f, .5f, .5f, 0), //CColor4f & endColorTo,
                    0, //float angleFrom,
                    360, //float angleTo,
                    -30, //float angleSpeedFrom,
                    30, //float angleSpeedTo,
                    0, //float gravityInfluence,
                    .25f, //float airResistanceInfluence,
                    5, //unsigned int particleCountFrom,
                    5, //unsigned int particleCountTo,
                    &clientVar.tex_smoke1,
                    0, //int textureFrameCount,
                    DKP_SRC_ALPHA, //unsigned int srcBlend,
                    DKP_ONE_MINUS_SRC_ALPHA);//unsigned int dstBlend);
            }
        }
        break;
    }
    case PROJECTILE_GRENADE:
    {
        rotateVel = 360;
    }
    case PROJECTILE_COCKTAIL_MOLOTOV:
    {
        rotateVel = 360;
        break;
    }
    case PROJECTILE_LIFE_PACK:
    {
        rotateVel = rand(-90.0f, 90.0f);
        break;
    }
    case PROJECTILE_DROPED_WEAPON:
    {
        rotateVel = rand(-90.0f, 90.0f);
        break;
    }
    case PROJECTILE_DROPED_GRENADE:
    {
        rotateVel = rand(-90.0f, 90.0f);
        break;
    }
    case PROJECTILE_FLAME:
    {
        rotateVel = 0;
        break;
    }
    }
}

void ClientProjectile::onGrenadeRebound(CVector3f p)
{
    dksPlay3DSound(clientVar.sfx_grenadeRebond, -1, 1, p, 200);
}

void ClientProjectile::update(float delay, Map* map)
{
    auto cscene = static_cast<ClientScene*>(scene);

    rotation += delay * rotateVel; // 1 tour à la seconde :D
    while(rotation >= 360) rotation -= 360;
    while(rotation < 0) rotation += 360;

    Projectile::update(delay, map);

    //--- Le feu est pogné sur un player
    if(projectileType == PROJECTILE_FLAME)
    {
        if(remoteEntity)
        {
            if(stickToPlayer >= 0)
            {
                auto cscene = static_cast<ClientScene*>(scene);
                if(cscene->client->game->players[stickToPlayer])
                {
                    if(cscene->client->game->players[stickToPlayer]->status == PLAYER_STATUS_DEAD)
                    {
                        stickToPlayer = -1;
                    }
                    else
                    {
                        currentCF.position = cscene->client->game->players[stickToPlayer]->currentCF.position;
                    }
                }
            }
        }
    }

    if(remoteEntity)
    {
        // Là on va créer une genre d'interpolation (cubic spline (bezier), pour être plus précis)
    //  currentCF.interpolate(cFProgression, netCF0, netCF1, delay); //--- Client side maintenant

        if(projectileType == PROJECTILE_ROCKET)
        {
            //  scene->client->game->trails.push_back(new Trail(lastCF.position, currentCF.position, 1.5f, CVector4f(.6f,.6f,.6f,.5f), 4));
    //rotation

            float colorArg1 = 1.0;
            float colorArg2 = 1.0;
            float colorArg3 = 1.0;
            if((cscene->client->game->gameType == 1 || cscene->client->game->gameType == 2) && cscene->client->game->players[fromID])
                //If we're CTF or TDM, our zooka trails should be coloured
            {
                if(cscene->client->game->players[fromID]->teamID == PLAYER_TEAM_BLUE)
                {
                    colorArg3 = 1.0f;
                    colorArg1 = colorArg2 = 0.25f;
                }
                else if(cscene->client->game->players[fromID]->teamID == PLAYER_TEAM_RED)
                {
                    colorArg1 = 1.0f;
                    colorArg2 = colorArg3 = 0.25f;
                }
            }
            // On spawn des particules dans son cul (une par frame)
            dkpCreateParticle(currentCF.position.s,//float *position,
                (-currentCF.vel * 0/*.25f*/).s,//float *vel,
                CVector4f(colorArg1, colorArg2, colorArg3, 0.75f).s,//float *startColor,
                CVector4f(colorArg1, colorArg2, colorArg3, 0.0f).s,//float *endColor,
                .125f,//float startSize,
                rand(.6f, 1.0f),//float endSize,
                5.0f,//float duration,
                0,//float gravityInfluence,
                0,//float airResistanceInfluence,
                rand(0.0f, 30.0f),//float rotationSpeed,
                clientVar.tex_smoke1,//unsigned int texture,
                DKP_SRC_ALPHA,//unsigned int srcBlend,
                DKP_ONE_MINUS_SRC_ALPHA,//unsigned int dstBlend,
                0);//int transitionFunc);

// On spawn des particules dans son cul (une par frame)
/*  dkpCreateParticle(  currentCF.position.s,//float *position,
                        (-currentCF.vel*.015f).s,//float *vel,
                        CVector4f(1,1,1,.75f).s,//float *startColor,
                        CVector4f(1,1,1,0.0f).s,//float *endColor,
                        .125f,//float startSize,
                        rand(.6f,1.0f),//float endSize,
                        5.0f,//float duration,
                        0,//float gravityInfluence,
                        0,//float airResistanceInfluence,
                        rand(0.0f, 30.0f),//float rotationSpeed,
                        clientVar.tex_smoke1,//unsigned int texture,
                        DKP_SRC_ALPHA,//unsigned int srcBlend,
                        DKP_ONE_MINUS_SRC_ALPHA,//unsigned int dstBlend,
                        0);//int transitionFunc);*/
        }

        if(projectileType == PROJECTILE_GRENADE)
        {
            //--- Depending of the team, the color change
            if(cscene->client->game->gameType == GAME_TYPE_DM)
            {
                // On spawn des particules dans son cul (une par frame)
                dkpCreateParticle(currentCF.position.s,//float *position,
                    (-currentCF.vel * 0/*.25f*/).s,//float *vel,
                    CVector4f(1, 1, 1, .25f).s,//float *startColor,
                    CVector4f(1, 1, 1, 0.0f).s,//float *endColor,
                    .125f,//float startSize,
                    rand(.2f, .2f),//float endSize,
                    2.0f,//float duration,,//float duration,
                    0,//float gravityInfluence,
                    0,//float airResistanceInfluence,
                    rand(0.0f, 30.0f),//float rotationSpeed,
                    clientVar.tex_shotGlow,//unsigned int texture,
                    DKP_SRC_ALPHA,//unsigned int srcBlend,
                    DKP_ONE/*_MINUS_SRC_ALPHA*/,//unsigned int dstBlend,
                    0);//int transitionFunc);
            }
            else if(cscene->client->game->players[fromID])
            {
                if(cscene->client->game->players[fromID]->teamID == PLAYER_TEAM_RED)
                {
                    // On spawn des particules dans son cul (une par frame)
                    dkpCreateParticle(currentCF.position.s,//float *position,
                        (-currentCF.vel * 0/*.25f*/).s,//float *vel,
                        CVector4f(1, .25f, .25f, .25f).s,//float *startColor,
                        CVector4f(1, .25f, .25f, 0.0f).s,//float *endColor,
                        .125f,//float startSize,
                        rand(.2f, .2f),//float endSize,
                        2.0f,//float duration,
                        0,//float gravityInfluence,
                        0,//float airResistanceInfluence,
                        rand(0.0f, 30.0f),//float rotationSpeed,
                        clientVar.tex_shotGlow,//unsigned int texture,
                        DKP_SRC_ALPHA,//unsigned int srcBlend,
                        DKP_ONE/*_MINUS_SRC_ALPHA*/,//unsigned int dstBlend,
                        0);//int transitionFunc);
                }
                else if(cscene->client->game->players[fromID]->teamID == PLAYER_TEAM_BLUE)
                {
                    // On spawn des particules dans son cul (une par frame)
                    dkpCreateParticle(currentCF.position.s,//float *position,
                        (-currentCF.vel * 0/*.25f*/).s,//float *vel,
                        CVector4f(.25f, .25f, 1, .25f).s,//float *startColor,
                        CVector4f(.25f, .25f, 1, 0.0f).s,//float *endColor,
                        .125f,//float startSize,
                        rand(.2f, .2f),//float endSize,
                        2.0f,//float duration,
                        0,//float gravityInfluence,
                        0,//float airResistanceInfluence,
                        rand(0.0f, 30.0f),//float rotationSpeed,
                        clientVar.tex_shotGlow,//unsigned int texture,
                        DKP_SRC_ALPHA,//unsigned int srcBlend,
                        DKP_ONE/*_MINUS_SRC_ALPHA*/,//unsigned int dstBlend,
                        0);//int transitionFunc);
                }
            }
        }

        if(projectileType == PROJECTILE_COCKTAIL_MOLOTOV)
        {
            CVector3f fireDirection(0, 0, 1);
            fireDirection = rotateAboutAxis(fireDirection, rotation, CVector3f(currentCF.vel[0], currentCF.vel[1], 0));
            // On spawn des particules de feu dans son cul (une par frame)
            dkpCreateParticle(currentCF.position.s,//float *position,
                (fireDirection*.15f).s,//float *vel,
                CVector4f(1, .75f, 0, 1.0f).s,//float *startColor,
                CVector4f(1, .75f, 0, 0.0f).s,//float *endColor,
                .25f,//float startSize,
                .025f,//float endSize,
                0.25f,//float duration,
                0,//float gravityInfluence,
                0,//float airResistanceInfluence,
                rand(0.0f, 30.0f),//float rotationSpeed,
                clientVar.tex_smoke1,//unsigned int texture,
                DKP_SRC_ALPHA,//unsigned int srcBlend,
                DKP_ONE,//unsigned int dstBlend,
                0);//int transitionFunc);
        }

        if(projectileType == PROJECTILE_FLAME)
        {
            spawnParticleTime++;
            if(spawnParticleTime >= 30) spawnParticleTime = 0;

            if(spawnParticleTime % 3 == 0)
            {
                // On spawn des particules de feu dans son cul (une par frame)
                dkpCreateParticle((currentCF.position + rand(CVector3f(-.20f, -.20f, 0), CVector3f(.20f, .20f, 0))).s,//float *position,
                    (CVector3f(0, 0, 1)).s,//float *vel,
                    rand(CVector4f(1, 0, 0, 0.0f), CVector4f(1, .75f, 0, 0.0f)).s,//float *startColor,
                    CVector4f(1, .75f, 0, 1.0f).s,//float *endColor,
                    .3f,//float startSize,
                    .0f,//float endSize,
                    1.0f,//float duration,
                    0,//float gravityInfluence,
                    0,//float airResistanceInfluence,
                    rand(0.0f, 30.0f),//float rotationSpeed,
                    clientVar.tex_smoke1,//unsigned int texture,
                    DKP_SRC_ALPHA,//unsigned int srcBlend,
                    DKP_ONE,//unsigned int dstBlend,
                    0);//int transitionFunc);
// On spawn des particules de feu dans son cul (une par frame)
                dkpCreateParticle((currentCF.position + rand(CVector3f(-.20f, -.20f, 0), CVector3f(.20f, .20f, 0))).s,//float *position,
                    (CVector3f(0, 0, 1) + rand(CVector3f(-.20f, -.20f, 0), CVector3f(.20f, .20f, 0))).s,//float *vel,
                    rand(CVector4f(1, 0, 0, 1.0f), CVector4f(1, .75f, 0, 1.0f)).s,//float *startColor,
                    CVector4f(1, .75f, 0, 0.0f).s,//float *endColor,
                    .0f,//float startSize,
                    .3f,//float endSize,
                    1.0f,//float duration,
                    0,//float gravityInfluence,
                    0,//float airResistanceInfluence,
                    rand(0.0f, 30.0f),//float rotationSpeed,
                    clientVar.tex_smoke1,//unsigned int texture,
                    DKP_SRC_ALPHA,//unsigned int srcBlend,
                    DKP_ONE,//unsigned int dstBlend,
                    0);//int transitionFunc);
            }
            if(spawnParticleTime % 10 == 0)
            {
                // On spawn des particules de feu dans son cul (une par frame)
                dkpCreateParticle((currentCF.position).s,//float *position,
                    (CVector3f(-.5f, 0, .5f)).s,//float *vel,
                    CVector4f(.5f, .5f, .5f, 0.5f).s,//float *startColor,
                    CVector4f(.5f, .5f, .5f, 0.0f).s,//float *endColor,
                    .15f,//float startSize,
                    1.0,//float endSize,
                    3.0f,//float duration,
                    0,//float gravityInfluence,
                    0,//float airResistanceInfluence,
                    rand(0.0f, 30.0f),//float rotationSpeed,
                    clientVar.tex_smoke1,//unsigned int texture,
                    DKP_SRC_ALPHA,//unsigned int srcBlend,
                    DKP_ONE_MINUS_SRC_ALPHA,//unsigned int dstBlend,
                    0);//int transitionFunc);
            }
        }
    }
    //  else
    //  {
}
