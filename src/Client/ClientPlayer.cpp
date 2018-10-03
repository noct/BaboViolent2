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
#include "ClientGame.h"
#include "ClientScene.h"
#include "ClientHelper.h"
#include "ClientMap.h"
#include "ClientConsole.h"
#include <glad/glad.h>
extern Scene * scene;

ClientPlayer::ClientPlayer(char pPlayerID, Map * pMap, Game * pGame) : Player(pPlayerID, pMap, pGame)
{
    isThisPlayer = false;
    tex_baboShadow = dktCreateTextureFromFile("main/textures/BaboShadow.png", DKT_FILTER_BILINEAR);
    tex_baboHalo = dktCreateTextureFromFile("main/textures/BaboHalo.png", DKT_FILTER_BILINEAR);
    initedMouseClic = false;
    followingPlayer = 0;
    scopeMode = false;
    tex_skinOriginal = dktCreateTextureFromFile((CString("main/skins/") + skin + ".png").s, DKT_FILTER_BILINEAR);
    tex_skin = dktCreateEmptyTexture(64, 32, 3, DKT_FILTER_BILINEAR);
}

//
// Destructeur
//
ClientPlayer::~ClientPlayer()
{
    ZEVEN_SAFE_DELETE(weapon);
    dktDeleteTexture(&tex_baboShadow);
    dktDeleteTexture(&tex_baboHalo);
    dktDeleteTexture(&tex_skinOriginal);
    dktDeleteTexture(&tex_skin);
}

void ClientPlayer::update(float delay)
{
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

    Player::update(delay);

    if(status == PLAYER_STATUS_ALIVE)
    {
        if(!remoteEntity)
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

        {
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
}

void ClientPlayer::spawn(const CVector3f & spawnPoint)
{
    Player::spawn(spawnPoint);

    auto cmap = static_cast<ClientMap*>(game->map);
    if(isThisPlayer) cmap->setCameraPos(spawnPoint);

    if(!game->isServerGame)
    {
        updateSkin();
    }
}

void ClientPlayer::onSpawnRequest()
{
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
}

void ClientPlayer::kill(bool silenceDeath)
{
    status = PLAYER_STATUS_DEAD;
    deadSince = 0;

    if(!silenceDeath)
    {
        dksPlay3DSound(clientVar.sfx_baboCreve[rand() % 3], -1, 5, currentCF.position, 255);
        auto cgame = static_cast<ClientGame*>(game);
        cgame->spawnBlood(currentCF.position, 1);

        //--- Spawn some gibs :D
    /*  for (int i=0;i<10;++i)
        {
            if (game) game->douilles.push_back(new Douille(currentCF.position,
                rand(CVector3f(-2.5,-2.5,1),CVector3f(2.5,2.5,2.5)),
                CVector3f(1,0,0), DOUILLE_TYPE_GIB));
        }*/
    }

    // Si il avait le flag, on le laisse tomber
    for(int i = 0; i < 2; ++i)
    {
        if(game->map->flagState[i] == playerID)
        {
            game->map->flagState[i] = -1; // Le server va nous communiquer la position du flag exacte
            game->map->flagPos[i] = currentCF.position;
            game->map->flagPos[i][2] = 0;

            auto cmap = static_cast<ClientMap*>(game->map);
            cmap->flagAngle[i] = 0;

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

void ClientPlayer::switchWeapon(int newWeaponID, bool forceSwitch)
{
    if(weapon && forceSwitch)
    {
        if(weapon->weaponID == newWeaponID) return;
    }
    // Bon testing, on va lui refiler un gun
    ZEVEN_SAFE_DELETE(weapon);
    weapon = new ClientWeapon(clientVar.weapons[newWeaponID]);
    weapon->currentFireDelay = 1; // On a une 1sec de delait quand on switch de gun
    weapon->m_owner = this;

    gameVar.cl_primaryWeapon = newWeaponID;

    // On entends clientVar.dkpp_
    auto cscene = static_cast<ClientScene*>(scene);
    if(cscene->client) dksPlay3DSound(clientVar.sfx_equip, -1, 1, currentCF.position, 255);

    // Reset rapid-fire hack check
    shotCount = 0;
    shotsPerSecond = 0;
}

void ClientPlayer::switchMeleeWeapon(int newWeaponID, bool forceSwitch)
{
    if(meleeWeapon && forceSwitch)
    {
        if(meleeWeapon->weaponID == newWeaponID) return;
    }
    // Bon testing, on va lui refiler un gun
    ZEVEN_SAFE_DELETE(meleeWeapon);

    meleeWeapon = new ClientWeapon(clientVar.weapons[newWeaponID]);
    meleeWeapon->currentFireDelay = 0;
    meleeWeapon->m_owner = this;

    gameVar.cl_secondaryWeapon = newWeaponID - WEAPON_KNIVES;
}

void ClientPlayer::setThisPlayerInfo()
{
    Player::setThisPlayerInfo();
    isThisPlayer = true;
}

void ClientPlayer::onShotgunReload()
{
    auto cscene = static_cast<ClientScene*>(scene);
    dksPlay3DSound(cscene->client->sfxShotyReload, -1, 5, currentCF.position, 230);
}

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
void ClientPlayer::render()
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

        //--- On pogne la position sur l'clientVar.dkpp_ran
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

void ClientPlayer::renderName()
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

void ClientPlayer::updateSkin()
{
    CColor3f redDecalT;
    CColor3f greenDecalT;
    CColor3f blueDecalT;
    CString skinT;

    //--- Ici c'est nowhere on update les couleurs lol
    //--- Si clientVar.dkpp_ changclientVar.dkpp_on update clientVar.dkpp_ au autres joueur!
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

    //--- On reload le skin si clientVar.dkpp_ changclientVar.dkpp_
    if(lastSkin != skinT)
    {
        skin = skinT;
        dktDeleteTexture(&tex_skinOriginal);
        tex_skinOriginal = dktCreateTextureFromFile(CString("main/skins/%s.png", skin.s).s, DKT_FILTER_BILINEAR);
    }

    redDecal = redDecalT;
    greenDecal = greenDecalT;
    blueDecal = blueDecalT;
    lastSkin = skin;

    //--- Hey oui, on recrclientVar.dkpp_une texture ogl clientVar.dkpp_chaque fois pour chaque babo qui spawn!!!!
    //--- On est en ogl, faq clientVar.dkpp_ kick ass MOUHOUHOUHAHAHA
    int w = 0;
    int h = 0;
    int bpp = 0;
    unsigned char* imgData = dktGetTextureData(tex_skinOriginal, &w, &h, &bpp);

    //--- Celon son team, on set la couleur du babo en consclientVar.dkpp_uence
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

    if(imgData)
    {
        for(int y = 0; y < h; ++y)
        {
            for(int x = 0; x < w; ++x)
            {
                int k = ((y * w) + x) * bpp;
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
        dktCreateTextureFromBuffer(&tex_skin, imgData, w, h, bpp, DKT_FILTER_BILINEAR);
        delete[] imgData;
    }
}

void ClientPlayer::onDeath(Player* from, Weapon * fromWeapon, bool friendlyFire)
{
    auto cscene = static_cast<ClientScene*>(scene);
    if(cscene->client || (scene->server && gameVar.sv_showKills))
    {
        CString message = name;
        switch(teamID)
        {
        case PLAYER_TEAM_BLUE:message.insert("\x1", 0); break;
        case PLAYER_TEAM_RED:message.insert("\x4", 0); break;
        }
        message.insert("-----> ", 0);
        message.insert(clientVar.lang_friendlyFire.s, 0);
        message.insert(fromWeapon->weaponName.s, 0);
        message.insert("\x8 -----", 0);
        message.insert(from->name.s, 0);
        switch(from->teamID)
        {
        case PLAYER_TEAM_BLUE:message.insert("\x1", 0); break;
        case PLAYER_TEAM_RED:message.insert("\x4", 0); break;
        }
        console->add(message);
    }
}


//
// Si on se fait toucher !
//
void ClientPlayer::hit(ClientWeapon * fromWeapon, ClientPlayer * from, float damage)
{
    auto cgame = static_cast<ClientGame*>(game);
    auto cscene = static_cast<ClientScene*>(scene);
    float cdamage = life - damage; // La diffclientVar.dkpp_ence :) (boom headshot)
    if(damage == -1) cdamage = fromWeapon->damage; // C'est pus possible clientVar.dkpp_

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

            // Oups, on crclientVar.dkpp_e?
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

            // Oups, on crclientVar.dkpp_e?
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

void ClientPlayer::controlIt(float delay)
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
