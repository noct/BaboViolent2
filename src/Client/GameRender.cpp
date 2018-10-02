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
#include "Console.h"
#include "ClientScene.h"
#include "ClientHelper.h"
#include "ClientConsole.h"
#include "ClientMap.h"
#include <glad/glad.h>

#include <algorithm>


extern Scene * scene;


#ifdef RENDER_LAYER_TOGGLE
extern int renderToggle;
#endif


//
// Render
//
void ClientGame::render()
{
    auto cscene = static_cast<ClientScene*>(scene);
    auto cmap = static_cast<ClientMap*>(map);
    int i;
    dkoDisable(DKO_MULTIPASS);
    //  dkoEnable(DKO_RENDER_NODE);
    //  dkoEnable(DKO_RENDER_FACE);

        // On render la map
    if(cmap)
    {
        if(cmap->fogDensity > 0 && gameVar.r_weatherEffects)
        {
            glEnable(GL_FOG);
            glFogi(GL_FOG_MODE, GL_LINEAR);
            glFogfv(GL_FOG_COLOR, cmap->fogColor.s);
            glFogf(GL_FOG_DENSITY, cmap->fogDensity);
            glFogf(GL_FOG_START, cmap->camPos[2] - cmap->fogStart);
            glFogf(GL_FOG_END, cmap->camPos[2] - cmap->fogEnd);
        }
        else
        {
            glDisable(GL_FOG);
        }

        // Positionne la camera
        glLoadIdentity();
        CVector3f up(0, 1, 1);
        normalize(up);
        for(i = 0; i < 1; ++i)
        {
            if(thisPlayer)
            {
                if(thisPlayer->status == PLAYER_STATUS_ALIVE)
                {
                    if(thisPlayer->scopeMode == true)
                    {
                        CVector2i res = dkwGetResolution();
                        dkglSetProjection(80, .1f, 50, (float)res[1] * 1.333f, (float)res[1]);
                        dkglLookAt(
                            thisPlayer->currentCF.position[0],
                            thisPlayer->currentCF.position[1],
                            .4f,
                            thisPlayer->currentCF.mousePosOnMap[0],
                            thisPlayer->currentCF.mousePosOnMap[1], .4f,
                            0, 0, 1);
                        break;
                    }
                }
            }
            if(gameVar.sv_topView)
            {
                if(thisPlayer)
                {
                    dkglLookAt(
                        cmap->camPos[0]/* + thisPlayer->shootShakeDis[0] * .25f*/, cmap->camPos[1]/* + thisPlayer->shootShakeDis[1] * .25f*/, cmap->camPos[2]/* + thisPlayer->shootShakeDis[2] * .25f*/,
                        cmap->camPos[0]/* + thisPlayer->shootShakeDis[0] * .25f*/, cmap->camPos[1]/* + thisPlayer->shootShakeDis[1] * .25f*/, 0/* + thisPlayer->shootShakeDis[2] * .25f*/,
                        up[0], up[1], up[2]);
                }
                else
                {
                    dkglLookAt(
                        cmap->camPos[0], cmap->camPos[1], cmap->camPos[2],
                        cmap->camPos[0], cmap->camPos[1], 0,
                        up[0], up[1], up[2]);
                }
            }
            else
            {
                dkglLookAt(
                    cmap->camPos[0], cmap->camPos[1] - 4.0f, cmap->camPos[2],
                    cmap->camPos[0], cmap->camPos[1] - 1.0f, 0,
                    up[0], up[1], up[2]);
            }
        }

#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 0)
#endif
            if(cmap->weather == WEATHER_RAIN || cmap->theme == THEME_SNOW)
            {
                glPushMatrix();
                glScalef(1, 1, -1);
                glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);

                {
                    //--- Sky
                    glColor3fv(cmap->fogColor.s);
                    dkglPushOrtho(10, 10);
                    renderTexturedQuad(0, 0, 10, 10, clientVar.tex_sky);
                    dkglPopOrtho();

                    glCullFace(GL_FRONT);
                    glEnable(GL_LIGHTING);
                    dkglSetPointLight(1, -1000, 1000, 2000, 1, 1, 1);

                    // On render les trucs genre flag pod, flag, canon
                    cmap->renderMisc();

                    // On render les players
                    for(i = 0; i < MAX_PLAYER; ++i)
                    {
                        auto cplayer = static_cast<ClientPlayer*>(players[i]);
                        if(cplayer)
                        {
                            cplayer->render();
                        }
                    }

                    // On render les rockets, grenades et autres projectiles
                    for(auto p : projectiles)
                    {
                        static_cast<ClientProjectile*>(p)->render();
                    }

                    // On render les projectiles client (Gibs et autre shit visuel)
                    for(auto p : clientProjectiles)
                    {
                        static_cast<ClientProjectile*>(p)->render();
                    }

                    // On render les douilles
                    ZEVEN_VECTOR_CALL(douilles, i, render());

                    // On render les murs
                    cmap->renderWalls();
                }

                glPopAttrib();

                // Les trails
                glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
                glDepthMask(GL_FALSE);
                glEnable(GL_BLEND);
                //  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                //  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glEnable(GL_TEXTURE_2D);
                //  ZEVEN_VECTOR_CALL(trails, i, render());
                for(i = 0; i < (int)trails.size(); ++i)
                {
                    if(trails[i]->trailType == 0)
                    {
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        glBindTexture(GL_TEXTURE_2D, clientVar.tex_smokeTrail);
                    }
                    else if(trails[i]->trailType == 1)
                    {
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                        glBindTexture(GL_TEXTURE_2D, clientVar.tex_glowTrail);
                    }
                    trails[i]->render();
                }
                //      ZEVEN_VECTOR_CALL(trails, i, renderBullet());
                for(i = 0; i < (int)trails.size(); ++i)
                {
                    if(trails[i]->trailType != 0) continue;
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    glBindTexture(GL_TEXTURE_2D, clientVar.tex_shotGlow);
                    trails[i]->renderBullet();
                }
                glDepthMask(GL_TRUE);
                glPopAttrib();

                // Les particules
                glPushAttrib(GL_ENABLE_BIT);
                glDisable(GL_FOG);
                glEnable(GL_LIGHTING);
                dkglSetPointLight(1, -2000, 2000, 1000, 1, 1, 1);
                dkpRender();
                glPopAttrib();
                glCullFace(GL_BACK);
                glPopMatrix();

                glClear(GL_DEPTH_BUFFER_BIT);
            }

        //  glEnable(GL_LIGHTING);

        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);
        // On trouve la position de la souri
        auto cconsole = static_cast<ClientConsole*>(console);
        if(thisPlayer && !cconsole->isActive())
        {
            if(thisPlayer->status == PLAYER_STATUS_ALIVE && !showMenu && !thisPlayer->scopeMode)
            {
                CVector2i res = dkwGetResolution();
                CVector2i mousePos = dkwGetCursorPos_main();

                mousePos[1] = res[1] - mousePos[1];
                CVector3f nearMouse = dkglUnProject(mousePos, 0.0f);
                CVector3f farMouse = dkglUnProject(mousePos, 1.0f);
                float percent = nearMouse[2] / (nearMouse[2] - farMouse[2]);
                thisPlayer->currentCF.mousePosOnMap = nearMouse + (farMouse - nearMouse) * percent;
            }
        }

        // Le soleil
        glEnable(GL_LIGHTING);
        dkglSetPointLight(1, -1000, 1000, 2000, 1, 1, 1);

        //--- Do we have water drip?
#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 1)
#endif
            if(gameVar.r_showGroundMark)
            {
                glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
                glDepthMask(GL_FALSE);
                glDisable(GL_LIGHTING);
                glEnable(GL_TEXTURE_2D);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glBindTexture(GL_TEXTURE_2D, clientVar.tex_drip);
                for(int i = 0; i < MAX_FLOOR_MARK; ++i) if(drips[i].life > 0) drips[i].render();
                glPopAttrib();
            }

        // Render la map
        cmap->renderGround();

        // On render les floor mark et projectile shadows
        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
        glDepthMask(GL_FALSE);
        glDisable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Les marke de bomb ou de sang
#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 5)
#endif
            if(gameVar.r_showGroundMark)
            {
                for(int i = 0; i < MAX_FLOOR_MARK; ++i) if(floorMarks[i].delay > 0) floorMarks[i].render();
            }

        // Lui on a pas le choix de l'afficher vu que ça fait parti du gameplay

        // On render les shadows des projectiles
        if(gameVar.r_projectileShadow)
        {
            glBindTexture(GL_TEXTURE_2D, tex_baboShadow);
            glEnable(GL_TEXTURE_2D);
            glColor4f(0, 0, 0, .35f);
            // On render les rockets, grenades et autres projectiles
            for(auto p : projectiles)
            {
                static_cast<ClientProjectile*>(p)->renderShadow();
            }
        }

        // On render les shadows des projectiles client
        if(gameVar.r_projectileShadow)
        {
            glBindTexture(GL_TEXTURE_2D, tex_baboShadow);
            glEnable(GL_TEXTURE_2D);
            glColor4f(0, 0, 0, .35f);
            // On render les rockets, grenades et autres projectiles
            for(auto p : clientProjectiles)
            {
                static_cast<ClientProjectile*>(p)->renderShadow();
            }
        }
        glPopAttrib();

        // On render les trucs genre flag pod, flag, canon
#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 6)
#endif
            cmap->renderMisc();

        // On render les players
#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 6)
#endif

            for(i = 0; i < MAX_PLAYER; ++i)
            {
                auto cplayer = static_cast<ClientPlayer*>(players[i]);
                if(cplayer)
                {
                    cplayer->render();
                }
            }

        // On render les rockets, grenades et autres projectiles
#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 7) {
#endif
            for(auto p : projectiles)
            {
                static_cast<ClientProjectile*>(p)->render();
            }

            // On render les projectiles clients
            for(auto p : clientProjectiles)
            {
                static_cast<ClientProjectile*>(p)->render();
            }
#ifdef RENDER_LAYER_TOGGLE
        }
#endif

        // On render les douilles
#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 9)
#endif
            ZEVEN_VECTOR_CALL(douilles, i, render());

        {
            // On render les shadows
#ifdef RENDER_LAYER_TOGGLE
            if(renderToggle >= 10)
#endif
                cmap->renderShadow();

            // On render les murs
#ifdef RENDER_LAYER_TOGGLE
            if(renderToggle >= 11)
#endif
                cmap->renderWalls();
        }
        glPopAttrib();

        // Les trails
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        //  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glEnable(GL_TEXTURE_2D);
        //  ZEVEN_VECTOR_CALL(trails, i, render());
#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 12)
#endif
            for(i = 0; i < (int)trails.size(); ++i)
            {
                if(trails[i]->trailType == 0)
                {
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glBindTexture(GL_TEXTURE_2D, clientVar.tex_smokeTrail);
                }
                else if(trails[i]->trailType == 1)
                {
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    glBindTexture(GL_TEXTURE_2D, clientVar.tex_glowTrail);
                }
                trails[i]->render();
            }
        //      ZEVEN_VECTOR_CALL(trails, i, renderBullet());
#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 13)
#endif
            for(i = 0; i < (int)trails.size(); ++i)
            {
                if(trails[i]->trailType != 0) continue;
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glBindTexture(GL_TEXTURE_2D, clientVar.tex_shotGlow);
                trails[i]->renderBullet();
            }
        glDepthMask(GL_TRUE);
        glPopAttrib();

        // Les particules
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_FOG);
        glEnable(GL_LIGHTING);
        dkglSetPointLight(1, -2000, 2000, 1000, 1, 1, 1);
#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 14)
#endif
            dkpRender();
        glPopAttrib();

        //--- Le weather
#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 15)
#endif
            if(cmap->m_weather) cmap->m_weather->render();

        //--- Nuke flash!!!!!!
        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
        glDepthMask(GL_FALSE);
        glDisable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_FOG);
        glBindTexture(GL_TEXTURE_2D, clientVar.tex_shotGlow);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        for(i = 0; i < (int)nikeFlashes.size(); ++i)
        {
            nikeFlashes[i]->render();
        }
        glPopAttrib();

        // La minimap
#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 16)
#endif
            renderMiniMap();

        CVector2i res = dkwGetResolution();
        dkglPushOrtho((float)res[0], (float)res[1]);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // On render le nom des players

        for(i = 0; i < MAX_PLAYER; ++i)
        {
            auto cplayer = static_cast<ClientPlayer*>(players[i]);
            if(cplayer && cplayer != thisPlayer)
            {
                cplayer->renderName();
            }
        }

        // On va renderer le nb de projectile (temporaire)
    //  dkfBindFont(font);
    //  dkfPrint(50,0,100,0,CString("%i",(int)projectiles.size()).s);
        glPopAttrib();
        dkglPopOrtho();
    }

    //--- On render le voting
    if(voting.votingInProgress)
    {
        CVector2i res = dkwGetResolution();
        dkglPushOrtho((float)res[0], (float)res[1]);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //--- Render le vote à l'écran
        if(voting.votingInProgress)
        {
            if(cscene->client->blink <= .25f) printRightText((float)res[0] - 10, 10, 30, CString("\x9VOTING : %i", (int)(voting.remaining)));
            if(voting.voted)
            {
                //--- Draw the vote bla bla bla
                printRightText((float)res[0] - 10, 40, 30, CString(
                    "\x9%s casted a vote:\n\x8\"%s\"\n\x2Yes: %i\n\x4No: %i",
                    voting.votingFrom.s,
                    voting.votingWhat.s,
                    voting.votingResults[0],
                    voting.votingResults[1]
                ));
            }
            else
            {
                //--- Draw the vote bla bla bla
                printRightText((float)res[0] - 10, 40, 30, CString(
                    "\x9%s casted a vote:\n\x8\"%s\"\n\x2[F1] Yes: %i\n\x4[F2] No: %i",
                    voting.votingFrom.s,
                    voting.votingWhat.s,
                    voting.votingResults[0],
                    voting.votingResults[1]
                ));
            }
        }
        glPopAttrib();
        dkglPopOrtho();
    }
}

//
// Pour afficher la minimap (ouff, je mélange pomal les affaires, tk)
// On la render ici et non dans l'objet map,
// car on a besoin de plein d'info comme la position des joueurs, flag, etc
//
void ClientGame::renderMiniMap()
{
    auto cscene = static_cast<ClientScene*>(scene);
    auto cmap = static_cast<ClientMap*>(map);
    CVector2i res(1280, 720);// = dkwGetResolution();

    dkglPushOrtho((float)res[0], (float)res[1]);
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D, cmap->texMap);
    glPushMatrix();
    float scalar = 128.0f / (float)std::max<int>(map->size[0], map->size[1]);
    glTranslatef(20, (float)res[1] - 20, 0);
    glScalef(scalar, -scalar, scalar); // On inverse le y ben oui :P

    glBegin(GL_QUADS);
    glColor4f(1, 1, 1, .5f);
    glTexCoord2f(0.0f, cmap->texMapSize[1]);
    glVertex2i(0, cmap->size[1]);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2i(0, 0);
    glTexCoord2f(cmap->texMapSize[0], 0.0f);
    glVertex2i(cmap->size[0], 0);
    glTexCoord2f(cmap->texMapSize[0], cmap->texMapSize[1]);
    glVertex2i(cmap->size[0], cmap->size[1]);
    glEnd();

    // On dessine les alliers avant
    if(thisPlayer)
    {
        if(gameType == GAME_TYPE_CTF)
        {
            glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
            glDisable(GL_TEXTURE_2D);
            // render pods, if flags are not on them!
            if(map->flagState[0] != -2)
            {
                // rendering blue pod
                glPushMatrix();
                glTranslatef(map->flagPodPos[0][0], map->flagPodPos[0][1], 0);
                glColor3f(0, 0, 1);
                glBegin(GL_QUADS);
                glVertex2f(-2 / scalar, 2 / scalar);
                glVertex2f(-2 / scalar, -2 / scalar);
                glVertex2f(2 / scalar, -2 / scalar);
                glVertex2f(2 / scalar, 2 / scalar);
                glEnd();
                glPopMatrix();
            }
            if(map->flagState[1] != -2)
            {
                // rendering red pod
                glPushMatrix();
                glTranslatef(map->flagPodPos[1][0], map->flagPodPos[1][1], 0);
                glColor3f(1, 0, 0);
                glBegin(GL_QUADS);
                glVertex2f(-2 / scalar, 2 / scalar);
                glVertex2f(-2 / scalar, -2 / scalar);
                glVertex2f(2 / scalar, -2 / scalar);
                glVertex2f(2 / scalar, 2 / scalar);
                glEnd();
                glPopMatrix();
            }
            glPopAttrib();
        }
        glBindTexture(GL_TEXTURE_2D, tex_miniMapAllied);
        for(int i = 0; i < MAX_PLAYER; ++i)
        {
            // On render le people sur la map
            if(players[i])
            {
                if(players[i]->status == PLAYER_STATUS_ALIVE)
                {
                    if((players[i]->teamID == thisPlayer->teamID || thisPlayer->teamID == PLAYER_TEAM_SPECTATOR) && gameType != GAME_TYPE_DM || thisPlayer == players[i])
                    {
                        glPushMatrix();
                        glTranslatef(players[i]->currentCF.position[0], players[i]->currentCF.position[1], 0);
                        glRotatef(players[i]->currentCF.angle, 0, 0, 1);
                        if(players[i] == thisPlayer)
                        {
                            switch(players[i]->teamID)
                            {
                            case PLAYER_TEAM_BLUE:glColor3f(0, 1, 1); break;
                            case PLAYER_TEAM_RED:glColor3f(1, 1, 0); break;
                            }
                        }
                        else
                        {
                            switch(players[i]->teamID)
                            {
                            case PLAYER_TEAM_BLUE:glColor3f(players[i]->firedShowDelay*.35f, players[i]->firedShowDelay*.35f, 1); break;
                            case PLAYER_TEAM_RED:glColor3f(1, players[i]->firedShowDelay*.35f, players[i]->firedShowDelay*.35f); break;
                            }
                        }
                        glBegin(GL_QUADS);
                        glTexCoord2f(0, 1);
                        glVertex2f(-4 / scalar, 4 / scalar);
                        glTexCoord2f(0, 0);
                        glVertex2f(-4 / scalar, -4 / scalar);
                        glTexCoord2f(1, 0);
                        glVertex2f(4 / scalar, -4 / scalar);
                        glTexCoord2f(1, 1);
                        glVertex2f(4 / scalar, 4 / scalar);
                        glEnd();
                        glPopMatrix();
                    }
                }
            }
        }
        glBindTexture(GL_TEXTURE_2D, tex_miniMapEnemy);
        if(thisPlayer->teamID != PLAYER_TEAM_SPECTATOR)
        {
            for(int i = 0; i < MAX_PLAYER; ++i)
            {
                // On render le people sur la map
                if(players[i] && players[i] != thisPlayer)
                {
                    if(players[i]->status == PLAYER_STATUS_ALIVE || players[i]->firedShowDelay > 0)
                    {
                        if(players[i]->teamID != thisPlayer->teamID || gameType == GAME_TYPE_DM)
                        {
                            glPushMatrix();
                            glTranslatef(players[i]->currentCF.position[0], players[i]->currentCF.position[1], 0);
                            glRotatef(players[i]->currentCF.angle, 0, 0, 1);
                            switch(players[i]->teamID)
                            {
                            case PLAYER_TEAM_BLUE:glColor4f(.3f, .3f, 1, players[i]->firedShowDelay*.5f); break;
                            case PLAYER_TEAM_RED:glColor4f(1, 0, 0, players[i]->firedShowDelay*.5f); break;
                            }

                            glBegin(GL_QUADS);
                            glTexCoord2f(0, 1);
                            glVertex2f(-4 / scalar, 4 / scalar);
                            glTexCoord2f(0, 0);
                            glVertex2f(-4 / scalar, -4 / scalar);
                            glTexCoord2f(1, 0);
                            glVertex2f(4 / scalar, -4 / scalar);
                            glTexCoord2f(1, 1);
                            glVertex2f(4 / scalar, 4 / scalar);
                            glEnd();
                            glPopMatrix();
                        }
                    }
                }
            }
        }
        // Si on est en CTF, on dessine les flag
        if(gameType == GAME_TYPE_CTF)
        {
            //      if (map->flagState[0] == -2) map->flagPos[0] = map->flagPodPos[0];
            //      if (map->flagState[1] == -2) map->flagPos[1] = map->flagPodPos[1];
            //      if (map->flagState[0] >= 0) if (players[map->flagState[0]]) map->flagPos[0] = players[map->flagState[0]]->currentCF.position;
            //      if (map->flagState[1] >= 0) if (players[map->flagState[1]]) map->flagPos[1] = players[map->flagState[1]]->currentCF.position;

            if(thisPlayer->teamID == PLAYER_TEAM_RED ||
                (thisPlayer->teamID == PLAYER_TEAM_BLUE && map->flagState[0] < 0))
            {
                glBindTexture(GL_TEXTURE_2D, cscene->client->tex_blueFlag);
                glPushMatrix();
                glTranslatef(map->flagPos[0][0], map->flagPos[0][1], 0);
                //--- Da feel flag :)
                glColor3f(1, 1, 1);
                glBegin(GL_QUADS);
                glTexCoord2f(0, 1);
                glVertex2f(-8 / scalar, 8 / scalar);
                glTexCoord2f(0, 0);
                glVertex2f(-8 / scalar, -8 / scalar);
                glTexCoord2f(1, 0);
                glVertex2f(8 / scalar, -8 / scalar);
                glTexCoord2f(1, 1);
                glVertex2f(8 / scalar, 8 / scalar);
                glEnd();
                glPopMatrix();
            }
            if(thisPlayer->teamID == PLAYER_TEAM_BLUE ||
                (thisPlayer->teamID == PLAYER_TEAM_RED && map->flagState[1] < 0))
            {
                glBindTexture(GL_TEXTURE_2D, cscene->client->tex_redFlag);
                glPushMatrix();
                glTranslatef(map->flagPos[1][0], map->flagPos[1][1], 0);
                glColor3f(1, 1, 1);
                glBegin(GL_QUADS);
                glTexCoord2f(0, 1);
                glVertex2f(-8 / scalar, 8 / scalar);
                glTexCoord2f(0, 0);
                glVertex2f(-8 / scalar, -8 / scalar);
                glTexCoord2f(1, 0);
                glVertex2f(8 / scalar, -8 / scalar);
                glTexCoord2f(1, 1);
                glVertex2f(8 / scalar, 8 / scalar);
                glEnd();
                glPopMatrix();
            }
        }
    }
    glPopMatrix();
    glPopAttrib();
    dkglPopOrtho();
}
