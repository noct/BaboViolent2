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
#include "SceneRender.h"
#include "ClientScene.h"
#include "ClientConsole.h"
#include "ClientMap.h"
#include "KeyManager.h"
#include <Zeven/Gfx.h>
#include <glad/glad.h>


extern Scene * scene;


#ifdef RENDER_LAYER_TOGGLE
extern int renderToggle;
#endif

static void IntroScreen_Render(float t, unsigned int tex)
{
    dkglPushOrtho(1,1);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
            glEnable(GL_TEXTURE_2D);

            if (t > 2)
            {
                glBindTexture(GL_TEXTURE_2D, tex);
                glColor3f(1-(t-2),1-(t-2),1-(t -2));
            }
            else if (t > 1)
            {
                glBindTexture(GL_TEXTURE_2D, tex);
                glColor3f(1,1,1);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, tex);
                glColor3f(t, t, t);
            }
            glBegin(GL_QUADS);
                glTexCoord2i(0,1);
                glVertex2i(0,0);
                glTexCoord2i(0,0);
                glVertex2i(0,1);
                glTexCoord2i(1,0);
                glVertex2i(1,1);
                glTexCoord2i(1,1);
                glVertex2i(1,0);
            glEnd();
        glPopAttrib();
    dkglPopOrtho();
}


void ClientScene_Render(ClientScene* scene)
{
    // On clear les buffers, on init la camera, etc
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CVector2i res = dkwGetResolution();

    glViewport(0, 0, res[0], res[1]);
    dkglSetProjection(60, 1, 50, (float)res[1] * 1.333f, (float)res[1]);

    // Truc par default ê enabeler
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1, 1, 1);

    if(scene->introDelay > 0)
    {
        IntroScreen_Render(scene->introDelay, scene->tex_rndLogo);
    }
    else
    {
        // On render le client
        float alphaScope = 0;
        if(scene->client) Client_Render(scene->client, alphaScope);

        // On render les menus
        if((menuManager.root) && (menuManager.root->visible))
        {
            menuManager.render();
            dkwClipMouse(false);
        }
        menuManager.renderDialogs();

        dkglPushOrtho((float)res[0], (float)res[1]);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        dkfBindFont(scene->font);

        glColor3f(1, 1, 1);
        if(gameVar.r_showStats)
        {
            printRightText((float)res[0], 0, 20, true, CString("FPS : %i", (int)dkcGetFPS(scene->ctx)));
        }

        // On affiche la version du jeu
        if(!scene->client)
        {
            if(scene->server)
            {
                printRightText((float)res[0] - 5, (float)res[1] - 32 - 5, 32, true, CString(clientVar.lang_serverVersion.s, (int)GAME_VERSION_SV / 10000, (int)(GAME_VERSION_SV % 10000) / 100, ((int)GAME_VERSION_SV % 100)));
            }
            else
            {
                printRightText((float)res[0] - 5, (float)res[1] - 32 - 5, 32, true, CString(clientVar.lang_clientVersion.s, (int)GAME_VERSION_CL / 10000, (int)(GAME_VERSION_CL % 10000) / 100, ((int)GAME_VERSION_CL % 100)));
            }
        }
        glPopAttrib();
        dkglPopOrtho();

        // Render la console sur toute
        auto cconsole = static_cast<ClientConsole*>(console);
        ClientConsole_Render(cconsole);

        // Non, le curseur sur TOUUTEEE
        CVector2i cursor = dkwGetCursorPos_main();
        int xM = (int)(((float)cursor[0] / (float)res[0])*800.0f);
        int yM = (int)(((float)cursor[1] / (float)res[1])*600.0f);
        dkglPushOrtho(800, 600);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if(menuManager.root)
            if(!menuManager.root->visible && scene->client)
            {
                if(scene->client->showMenu)
                {
                    glColor3f(1, 1, 1);
                    //  glColor3f(1,1,.6f);
                    renderTexturedQuad(xM, yM, 32, 32, scene->tex_menuCursor);
                }
                else
                {
                    glColor4f(0, 0, 0, 1 - alphaScope);
                    renderTexturedQuad(xM - 15, yM - 15, 32, 32, scene->tex_crosshair);
                    //  glColor4f(1,1,.6f,1-alphaScope);
                    glColor4f(1, 1, 1, 1 - alphaScope);
                    renderTexturedQuad(xM - 16, yM - 16, 32, 32, scene->tex_crosshair);
                }
            }
            else
            {
                glColor3f(1, 1, 1);
                //  glColor3f(1,1,.6f);
                renderTexturedQuad(xM, yM, 32, 32, scene->tex_menuCursor);
            }
        glPopAttrib();
        dkglPopOrtho();
    }
}

void ClientConsole_Render(ClientConsole* console)
{
    int i;
    if(console->m_vPos > 0)
    {
        CVector2i res = dkwGetResolution();

        // on print ?l'?ran les 10 dernier messages encouru
        dkglPushOrtho((float)res[0], (float)res[1]);
        glTranslatef(0, console->m_vPos, 0);
        dkfBindFont(console->m_font);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_QUADS);
        glColor4f(0, 0, 0, .75f);
        glVertex2i(0, -((gameVar.c_huge) ? 510 : 310));
        glColor4f(.3f, .3f, .3f, .75f);
        glVertex2i(0, 0);
        glVertex2i(res[0], 0);
        glColor4f(0, 0, 0, .75f);
        glVertex2i(res[0], -((gameVar.c_huge) ? 510 : 310));
        glEnd();
        glColor4f(.7f, .8f, 1, .75f);
        glBegin(GL_QUADS);
        glVertex2i(0, 0);
        glVertex2i(0, 5);
        glVertex2i(res[0], 5);
        glVertex2i(res[0], 0);
        glEnd();
        glColor4f(.3f, .3f, .3f, .75f);
        glBegin(GL_QUADS);
        glVertex2i(0, 5);
        glVertex2i(0, 35);
        glVertex2i(res[0], 35);
        glVertex2i(res[0], 5);
        glEnd();
        glColor4f(.7f, .8f, 1, .75f);
        glBegin(GL_QUADS);
        glVertex2i(0, 35);
        glVertex2i(0, 40);
        glVertex2i(res[0], 40);
        glVertex2i(res[0], 35);
        glEnd();
        glEnable(GL_TEXTURE_2D);
        glColor3f(1, 1, 0);
        dkfPrint(30, res[0] - 190.0f, 5, 0, "F1 - events, F2 - chat");
        glColor3f(1, 1, 1);
        console->m_currentText->print(30, 20, 5, 0);
        glPushMatrix();
        const std::vector<CString>& displayMessages = console->GetActiveMessages();

        int linesPerPage;
        if(displayMessages.size() > 0)
            linesPerPage = (int)ceil(((gameVar.c_huge) ? 510 : 310) /
                dkfGetStringHeight(24, displayMessages[0].s));
        else
            linesPerPage = 0;
        for(i = console->m_visibleMsgOffset;
            i < std::min<int>(console->m_visibleMsgOffset + linesPerPage, (int)displayMessages.size());
            ++i)
        {
            if((int)(displayMessages.size()) - 1 - i < 0) break;
            float height = dkfGetStringHeight(24, displayMessages[displayMessages.size() - i - 1].s);
            glTranslatef(0, -height, 0);
            dkfPrint(24, 20, 0, 0, displayMessages[displayMessages.size() - i - 1].s);
        }
        glPopMatrix();
        if(console->showRecognitionVar)
        {
            glDisable(GL_TEXTURE_2D);
            glBegin(GL_QUADS);
            glColor4f(0, 0, 0, .75f);
            glVertex2i(0, 45);
            glColor4f(.3f, .3f, .3f, .75f);
            glVertex2i(0, 45 + 30 * CONSOLE_MAX_RECOGNITION_VAR + 5);
            glVertex2i(res[0], 45 + 30 * CONSOLE_MAX_RECOGNITION_VAR + 5);
            glColor4f(0, 0, 0, .75f);
            glVertex2i(res[0], 45);
            glEnd();
            glEnable(GL_TEXTURE_2D);
            glColor3f(1, 1, 1);
            glPushMatrix();

            // Render first normally
            dkfPrint(30, 40, 45, 0, console->recognitionVar[0]);
            glTranslatef(0, dkfGetStringHeight(30, console->recognitionVar[0]), 0);

            // If we are cycling, get the other matches
            CString temp = CString(console->recognitionVar[0]).getFirstToken(' ');
            if(console->lastRecognitionVar != "")
                dksvarGetFilteredVar(console->lastRecognitionVar.s, console->recognitionVar, CONSOLE_MAX_RECOGNITION_VAR);

            // Render the rest
            for(i = console->curRecognitionVar + 1; i < CONSOLE_MAX_RECOGNITION_VAR; ++i)
            {
                dkfPrint(30, 40, 45, 0, console->recognitionVar[i]);
                float height = dkfGetStringHeight(30, console->recognitionVar[i]);
                glTranslatef(0, height, 0);
            }

            // Restore the recognition vars
            if(console->lastRecognitionVar != "")
                dksvarGetFilteredVar(temp.s, console->recognitionVar, CONSOLE_MAX_RECOGNITION_VAR);

            glPopMatrix();
        }
        glPopAttrib();
        dkglPopOrtho();
    }
}

//
// Pour êrire du texte centrê
//
void printCenterText(float x, float y, float size, bool enableShadow, const CString & text)
{
    float width = dkfGetStringWidth(size, text.s);

    if (enableShadow && (gameVar.r_simpleText != true))
    {
        int shadowDis = (size > 32) ? 2 : 1;
        float curColor[4];

        glGetFloatv(GL_CURRENT_COLOR, curColor);
        glPushAttrib(GL_CURRENT_BIT);
            glColor4f(0,0,0, curColor[3]);
            dkfPrint(size,x-width/2+shadowDis,y+shadowDis,0,textColorLess(text).s);
        glPopAttrib();
        glColor4fv(curColor);
    }
    dkfPrint(size,x-width/2,y,0,text.s);
}

void printLeftText(float x, float y, float size, bool enableShadow, const CString & text)
{
    if (enableShadow && (gameVar.r_simpleText != true))
    {
        int shadowDis = (size > 32) ? 2 : 1;
        float curColor[4];

        glGetFloatv(GL_CURRENT_COLOR, curColor);
        glPushAttrib(GL_CURRENT_BIT);
            glColor4f(0,0,0, curColor[3]);
            dkfPrint(size,x/*+shadowDis*/,y+1,0,textColorLess(text).s);
        glPopAttrib();
        glColor4fv(curColor);
    }
    dkfPrint(size,x,y,0,text.s);
}

void printRightText(float x, float y, float size, bool enableShadow, const CString & text)
{

    float width = dkfGetStringWidth(size, text.s);


    if (enableShadow && (gameVar.r_simpleText != true))
    {
        float curColor[4];
        int shadowDis = (size > 32) ? 2 : 1;

        glGetFloatv(GL_CURRENT_COLOR, curColor);
        glPushAttrib(GL_CURRENT_BIT);
            glColor4f(0,0,0, curColor[3]);
            dkfPrint(size,x-width+shadowDis,y+shadowDis,0,textColorLess(text).s);
        glPopAttrib();
        glColor4fv(curColor);
    }

    dkfPrint(size,x-width,y,0,text.s);
}

//
// Pour afficher un quad truc simple de meme
//
void renderTexturedQuad(int x, int y, int w, int h, unsigned int texture)
{
    glPushAttrib(GL_ENABLE_BIT);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBegin(GL_QUADS);
            glTexCoord2i(0,1);
            glVertex2i(x,y);
            glTexCoord2i(0,0);
            glVertex2i(x,y+h);
            glTexCoord2i(1,0);
            glVertex2i(x+w,y+h);
            glTexCoord2i(1,1);
            glVertex2i(x+w,y);
        glEnd();
    glPopAttrib();
}

void renderTexturedQuadSmooth(int x, int y, int w, int h, unsigned int texture)
{
    float curColor[4];
    glGetFloatv(GL_CURRENT_COLOR, curColor);

    glPushAttrib(GL_ENABLE_BIT);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBegin(GL_QUAD_STRIP);
            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,1);
            glVertex2i(x,y);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,1);
            glVertex2i(x,y+10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,1);
            glVertex2i(x+10,y);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(0,1);
            glVertex2i(x+10,y+10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,1);
            glVertex2i(x+w-10,y);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(1,1);
            glVertex2i(x+w-10,y+10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,1);
            glVertex2i(x+w,y);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,1);
            glVertex2i(x+w,y+10);
        glEnd();
        glBegin(GL_QUAD_STRIP);
            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,1);
            glVertex2i(x,y+10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,0);
            glVertex2i(x,y+h-10);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(0,1);
            glVertex2i(x+10,y+10);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(0,0);
            glVertex2i(x+10,y+h-10);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(1,1);
            glVertex2i(x+w-10,y+10);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(1,0);
            glVertex2i(x+w-10,y+h-10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,1);
            glVertex2i(x+w,y+10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,0);
            glVertex2i(x+w,y+h-10);
        glEnd();
        glBegin(GL_QUAD_STRIP);
            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,0);
            glVertex2i(x,y+h-10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,0);
            glVertex2i(x,y+h);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(0,0);
            glVertex2i(x+10,y+h-10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,0);
            glVertex2i(x+10,y+h);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(1,0);
            glVertex2i(x+w-10,y+h-10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,0);
            glVertex2i(x+w-10,y+h);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,0);
            glVertex2i(x+w,y+h-10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,0);
            glVertex2i(x+w,y+h);
        glEnd();
    glPopAttrib();
}

void renderMenuQuad(int x, int y, int w, int h)
{
    glPushAttrib(GL_CURRENT_BIT);
    {
        CVector4f color4;
        glGetFloatv(GL_CURRENT_COLOR, color4.s);
        //--- Round corner of 5 units

        glBegin(GL_QUADS);
        {
            glColor4fv(color4.s);
            glVertex2i(x, y);
            glColor4fv(color4.s);
            glVertex2i(x, y + h);
            glColor4fv(color4.s);
            glVertex2i(x + w, y + h);
            glColor4fv(color4.s);
            glVertex2i(x + w, y);
        }
        glEnd();
    }
    glPopAttrib();
}

void renderLoadingScreen(unsigned int font)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CVector2i res = dkwGetResolution();

    glViewport(0, 0, res[0], res[1]);
    dkglSetProjection(60, 1, 50, (float)res[0], (float)res[1]);

    // Truc par default à enabeler
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1, 1, 1);

    dkglPushOrtho(800, 600);

    // Print au millieu
    glColor3f(1, 1, 1);
    dkfBindFont(font);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    printCenterText(400, 268, 64, true, CString("LOADING"));

    dkglPopOrtho();

    // On swap les buffers
    dkwSwap();
}

void ClientProjectile_Render(ClientProjectile* proj)
{
    // Les effects de la rocket
    if(proj->projectileType == PROJECTILE_ROCKET)
    {
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
        glDisable(GL_LIGHTING);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glPushMatrix();
        // Shot glow
        glTranslatef(proj->currentCF.position[0], proj->currentCF.position[1], 0);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, clientVar.tex_shotGlow);
        glColor4f(1, 1, 1, rand(.05f, .25f));
        glBegin(GL_QUADS);
        glTexCoord2f(0, 1);
        glVertex3f(-2.5f, 2.5f, 0);
        glTexCoord2f(0, 0);
        glVertex3f(-2.5f, -2.5f, 0);
        glTexCoord2f(1, 0);
        glVertex3f(2.5f, -2.5f, 0);
        glTexCoord2f(1, 1);
        glVertex3f(2.5f, 2.5f, 0);
        glEnd();
        glEnable(GL_DEPTH_TEST);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(proj->currentCF.position[0], proj->currentCF.position[1], proj->currentCF.position[2]);
        glRotatef(proj->currentCF.angle, 0, 0, 1);
        glRotatef(rand(.0f, 360.0f), 0, 1, 0);
        glScalef(.5f, .5f, .5f);
        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, clientVar.tex_nuzzleFlash);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDisable(GL_CULL_FACE);
        glDisable(GL_FOG);
        glColor4f(1, 1, 1, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex3f(-.25f, 0, 0);
        glTexCoord2f(0, 1);
        glVertex3f(-.25f, -1, 0);
        glTexCoord2f(1, 1);
        glVertex3f(.25f, -1, 0);
        glTexCoord2f(1, 0);
        glVertex3f(.25f, 0, 0);

        glTexCoord2f(0, 0);
        glVertex3f(0, 0, .25f);
        glTexCoord2f(0, 1);
        glVertex3f(0, -1, .25f);
        glTexCoord2f(1, 1);
        glVertex3f(0, -1, -.25f);
        glTexCoord2f(1, 0);
        glVertex3f(0, 0, -.25f);
        glEnd();
        glPopMatrix();
        glPopAttrib();
    }

    // Les effects du molotov
    if(proj->projectileType == PROJECTILE_COCKTAIL_MOLOTOV)
    {
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
        glDisable(GL_LIGHTING);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glPushMatrix();
        // Le glow lumineux
        glTranslatef(proj->currentCF.position[0], proj->currentCF.position[1], 0);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, clientVar.tex_shotGlow);
        glColor4f(1, 1, 1, rand(.05f, .25f));
        glBegin(GL_QUADS);
        glTexCoord2f(0, 1);
        glVertex3f(-2.5f, 2.5f, 0);
        glTexCoord2f(0, 0);
        glVertex3f(-2.5f, -2.5f, 0);
        glTexCoord2f(1, 0);
        glVertex3f(2.5f, -2.5f, 0);
        glTexCoord2f(1, 1);
        glVertex3f(2.5f, 2.5f, 0);
        glEnd();
        glEnable(GL_DEPTH_TEST);
        glPopMatrix();
        glPopAttrib();
    }

    if(proj->projectileType != PROJECTILE_FLAME)
    {
        glPushAttrib(GL_ENABLE_BIT);
        glEnable(GL_LIGHTING);
        glPushMatrix();
        glTranslatef(proj->currentCF.position[0], proj->currentCF.position[1], proj->currentCF.position[2]);
        if(proj->projectileType == PROJECTILE_ROCKET)
        {
            glRotatef(proj->currentCF.angle, 0, 0, 1);
            glScalef(.0025f, .0025f, .0025f);
            dkoRender(clientVar.dko_rocket); // Voilà!
        }
        if(proj->projectileType == PROJECTILE_GRENADE)
        {
            glRotatef(proj->rotation, proj->currentCF.vel[0], proj->currentCF.vel[1], 0);
            glScalef(.0025f, .0025f, .0025f);
            dkoRender(clientVar.dko_grenade); // Voilà!
        }
        if(proj->projectileType == PROJECTILE_COCKTAIL_MOLOTOV)
        {
            glRotatef(proj->rotation, proj->currentCF.vel[0], proj->currentCF.vel[1], 0);
            glScalef(.0025f, .0025f, .0025f);
            dkoRender(clientVar.dko_cocktailMolotov); // Voilà!
        }
        if(proj->projectileType == PROJECTILE_DROPED_GRENADE)
        {
            //  glTranslatef(0,0,.30f);
            glScalef(.0025f, .0025f, .0025f);
            dkoRender(clientVar.dko_grenade); // Voilà!
        }
        if(proj->projectileType == PROJECTILE_LIFE_PACK)
        {
            glTranslatef(0, 0, -.20f);
            glScalef(.0025f, .0025f, .0025f);
            dkoRender(clientVar.dko_lifePack); // Voilà!
        }
        if(proj->projectileType == PROJECTILE_DROPED_WEAPON)
        {
            glTranslatef(0, 0, -.30f);
            glRotatef(proj->rotation, 0, 0, 1);
            glScalef(.005f, .005f, .005f);
            if(proj->fromID >= 0)
            {
                if(clientVar.weapons[proj->fromID]) dkoRender(clientVar.weapons[proj->fromID]->dkoModel); // Voilà!
            }
        }
        glPopMatrix();
        glPopAttrib();
    }
    else
    {
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
        glDisable(GL_LIGHTING);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glPushMatrix();
        // Shot glow
        glTranslatef(proj->currentCF.position[0], proj->currentCF.position[1], 0);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, clientVar.tex_shotGlow);
        glColor4f(1, .75f, 0, rand(.10f, .15f)*(1 - proj->currentCF.position[2]));
        glBegin(GL_QUADS);
        glTexCoord2f(0, 1);
        glVertex3f(-1.0f, 1.0f, 0);
        glTexCoord2f(0, 0);
        glVertex3f(-1.0f, -1.0f, 0);
        glTexCoord2f(1, 0);
        glVertex3f(1.0f, -1.0f, 0);
        glTexCoord2f(1, 1);
        glVertex3f(1.0f, 1.0f, 0);
        glEnd();
        glEnable(GL_DEPTH_TEST);
        glPopMatrix();
        glPopAttrib();
    }
}

void ClientProjectile_RenderShadow(ClientProjectile* proj)
{
    // On render son shadow :)
    glPushMatrix();
    glTranslatef(proj->currentCF.position[0] + .1f, proj->currentCF.position[1] - .1f, 0.025f);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1);
    glVertex2f(-.25f, .25f);
    glTexCoord2f(0, 0);
    glVertex2f(-.25f, -.25f);
    glTexCoord2f(1, 0);
    glVertex2f(.25f, -.25f);
    glTexCoord2f(1, 1);
    glVertex2f(.25f, .25f);
    glEnd();
    glPopMatrix();
}

void NukeFlash_Render(NukeFlash* nuke)
{
    glPushMatrix();
        glTranslatef(nuke->position[0], nuke->position[1], nuke->position[2]);
        glScalef(nuke->radius, nuke->radius, nuke->radius);
        glColor4f(1, 1, 1, nuke->density*nuke->life);
        glBegin(GL_QUADS);
            glTexCoord2i(0,1);
            glVertex2i(-1,1);
            glTexCoord2i(0,0);
            glVertex2i(-1,-1);
            glTexCoord2i(1,0);
            glVertex2i(1,-1);
            glTexCoord2i(1,1);
            glVertex2i(1,1);
        glEnd();
    glPopMatrix();
}

void Drip_Render(Drip* drip)
{
    glPushMatrix();
        glTranslatef(drip->position[0], drip->position[1], drip->position[2]);
        float _size = (1 - drip->life) * drip->size;
        glScalef(_size, _size, _size);
        glColor4f(.25f, .7f, .3f, drip->life*2);
        glBegin(GL_QUADS);
            glTexCoord2i(0,1);
            glVertex2i(-1,1);
            glTexCoord2i(0,0);
            glVertex2i(-1,-1);
            glTexCoord2i(1,0);
            glVertex2i(1,-1);
            glTexCoord2i(1,1);
            glVertex2i(1,1);
        glEnd();
    glPopMatrix();
}

void FloorMark_Render(FloorMark* mark)
{
    if (mark->startDelay <= 0)
    {
        glBindTexture(GL_TEXTURE_2D, mark->texture);
        glPushMatrix();
            glTranslatef(mark->position[0], mark->position[1], mark->position[2] + .025f);
            glRotatef(mark->angle, 0, 0, 1);
            glScalef(mark->size, mark->size, mark->size);
            if (mark->delay < 10)
                glColor4f(mark->color[0], mark->color[1], mark->color[2], mark->color[3] * ((mark->delay)*0.1f));
            else
                glColor4fv(mark->color.s);
            glBegin(GL_QUADS);
                glTexCoord2i(0,1);
                glVertex2i(-1,1);
                glTexCoord2i(0,0);
                glVertex2i(-1,-1);
                glTexCoord2i(1,0);
                glVertex2i(1,-1);
                glTexCoord2i(1,1);
                glVertex2i(1,1);
            glEnd();
        glPopMatrix();
    }
}

void Douille_Render(Douille* douille)
{
    glPushMatrix();
        glTranslatef(douille->position[0], douille->position[1], douille->position[2]);
        glRotatef(douille->delay*90, douille->vel[0], douille->vel[1],0);
        glScalef(.005f,.005f,.005f);
        if (douille->type == DOUILLE_TYPE_DOUILLE) dkoRender(clientVar.dko_douille);
        else if (douille->type == DOUILLE_TYPE_GIB) dkoRender(clientVar.dko_gib);
    glPopMatrix();
}

void Trail_Render(Trail* trail)
{
    glColor4f(.7f, .7f, .7f, (1- trail->delay)*.5f);
    if (trail->trailType == 1) glColor4f(trail->color[0], trail->color[1], trail->color[2],(1- trail->delay));
    glBegin(GL_QUADS);
    {
        glTexCoord2f(0, trail->dis);
        glVertex3fv((trail->p2 - trail->right* trail->delay*trail->size).s);
        glTexCoord2f(0, 0);
        glVertex3fv((trail->p1 - trail->right* trail->delay*trail->size).s);
        glTexCoord2f(1, 0);
        glVertex3fv((trail->p1 + trail->right* trail->delay*trail->size).s);
        glTexCoord2f(1, trail->dis);
        glVertex3fv((trail->p2 + trail->right* trail->delay*trail->size).s);
    }
    glEnd();
}

void Trail_RenderBullet(Trail* trail)
{
    float progress = ((trail->delay/ trail->delaySpeed)*40 + trail->offset*1) / trail->dis;
    if (progress < 1)
    {
        CVector3f dir = trail->p2 - trail->p1;
        float x = trail->p1[0]+dir[0]*progress;
        float y = trail->p1[1]+dir[1]*progress;

        glColor4f(trail->color[0], trail->color[1], trail->color[2],.1f);
        glBegin(GL_QUADS);
            glTexCoord2f(0,1);
            glVertex3f(x-1.0f,y+1.0f,0);
            glTexCoord2f(0,0);
            glVertex3f(x-1.0f,y-1.0f,0);
            glTexCoord2f(1,0);
            glVertex3f(x+1.0f,y-1.0f,0);
            glTexCoord2f(1,1);
            glVertex3f(x+1.0f,y+1.0f,0);
        glEnd();

        glColor4f(trail->color[0], trail->color[1], trail->color[2], 1);
        glBegin(GL_QUADS);
            glTexCoord2f(0, 1);
            glVertex3fv((trail->p1 + dir * progress + dir / trail->dis - trail->right*.05f).s);
            glTexCoord2f(0, 0);
            glVertex3fv((trail->p1 + dir * progress - trail->right*.05f).s);
            glTexCoord2f(1, 0);
            glVertex3fv((trail->p1 + dir * progress + trail->right*.05f).s);
            glTexCoord2f(1, 1);
            glVertex3fv((trail->p1 + dir * progress + dir / trail->dis + trail->right*.05f).s);
        glEnd();
    }
}

void ClientGame_Render(ClientGame* game)
{
    auto cscene = static_cast<ClientScene*>(scene);
    auto cmap = static_cast<ClientMap*>(game->map);
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
            if(game->thisPlayer)
            {
                if(game->thisPlayer->status == PLAYER_STATUS_ALIVE)
                {
                    if(game->thisPlayer->scopeMode == true)
                    {
                        CVector2i res = dkwGetResolution();
                        dkglSetProjection(80, .1f, 50, (float)res[1] * 1.333f, (float)res[1]);
                        dkglLookAt(
                            game->thisPlayer->currentCF.position[0],
                            game->thisPlayer->currentCF.position[1],
                            .4f,
                            game->thisPlayer->currentCF.mousePosOnMap[0],
                            game->thisPlayer->currentCF.mousePosOnMap[1], .4f,
                            0, 0, 1);
                        break;
                    }
                }
            }
            if(gameVar.sv_topView)
            {
                if(game->thisPlayer)
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
                        auto cplayer = static_cast<ClientPlayer*>(game->players[i]);
                        if(cplayer)
                        {
                            ClientPlayer_Render(cplayer);
                        }
                    }

                    // On render les rockets, grenades et autres projectiles
                    for(auto p : game->projectiles)
                    {
                        ClientProjectile_Render(static_cast<ClientProjectile*>(p));
                    }

                    // On render les projectiles client (Gibs et autre shit visuel)
                    for(auto p : game->clientProjectiles)
                    {
                        ClientProjectile_Render(static_cast<ClientProjectile*>(p));
                    }

                    // On render les douilles
                    for(auto d : game->douilles)
                    {
                        Douille_Render(d);
                    }

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
                for(i = 0; i < (int)game->trails.size(); ++i)
                {
                    if(game->trails[i]->trailType == 0)
                    {
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        glBindTexture(GL_TEXTURE_2D, clientVar.tex_smokeTrail);
                    }
                    else if(game->trails[i]->trailType == 1)
                    {
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                        glBindTexture(GL_TEXTURE_2D, clientVar.tex_glowTrail);
                    }
                    Trail_Render(game->trails[i]);
                }
                //      ZEVEN_VECTOR_CALL(trails, i, renderBullet());
                for(i = 0; i < (int)game->trails.size(); ++i)
                {
                    if(game->trails[i]->trailType != 0) continue;
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    glBindTexture(GL_TEXTURE_2D, clientVar.tex_shotGlow);
                    Trail_RenderBullet(game->trails[i]);
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
        if(game->thisPlayer && !cconsole->isActive())
        {
            if(game->thisPlayer->status == PLAYER_STATUS_ALIVE && !game->showMenu && !game->thisPlayer->scopeMode)
            {
                CVector2i res = dkwGetResolution();
                CVector2i mousePos = dkwGetCursorPos_main();

                mousePos[1] = res[1] - mousePos[1];
                CVector3f nearMouse = dkglUnProject(mousePos, 0.0f);
                CVector3f farMouse = dkglUnProject(mousePos, 1.0f);
                float percent = nearMouse[2] / (nearMouse[2] - farMouse[2]);
                game->thisPlayer->currentCF.mousePosOnMap = nearMouse + (farMouse - nearMouse) * percent;
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
                for(int i = 0; i < MAX_FLOOR_MARK; ++i)
                {
                    if(game->drips[i].life > 0)
                    {
                        Drip_Render(&game->drips[i]);
                    }
                }
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
                for(int i = 0; i < MAX_FLOOR_MARK; ++i)
                {
                    if(game->floorMarks[i].delay > 0)
                    {
                        FloorMark_Render(&game->floorMarks[i]);
                    }
                }
            }

        // Lui on a pas le choix de l'afficher vu que ça fait parti du gameplay

        // On render les shadows des projectiles
        if(gameVar.r_projectileShadow)
        {
            glBindTexture(GL_TEXTURE_2D, game->tex_baboShadow);
            glEnable(GL_TEXTURE_2D);
            glColor4f(0, 0, 0, .35f);
            // On render les rockets, grenades et autres projectiles
            for(auto p : game->projectiles)
            {
                ClientProjectile_RenderShadow(static_cast<ClientProjectile*>(p));
            }
        }

        // On render les shadows des projectiles client
        if(gameVar.r_projectileShadow)
        {
            glBindTexture(GL_TEXTURE_2D, game->tex_baboShadow);
            glEnable(GL_TEXTURE_2D);
            glColor4f(0, 0, 0, .35f);
            // On render les rockets, grenades et autres projectiles
            for(auto p : game->clientProjectiles)
            {
                ClientProjectile_RenderShadow(static_cast<ClientProjectile*>(p));
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
                auto cplayer = static_cast<ClientPlayer*>(game->players[i]);
                if(cplayer)
                {
                    ClientPlayer_Render(cplayer);
                }
            }

        // On render les rockets, grenades et autres projectiles
#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 7) {
#endif
            for(auto p : game->projectiles)
            {
                ClientProjectile_Render(static_cast<ClientProjectile*>(p));
            }

            // On render les projectiles clients
            for(auto p : game->clientProjectiles)
            {
                ClientProjectile_Render(static_cast<ClientProjectile*>(p));
            }
#ifdef RENDER_LAYER_TOGGLE
        }
#endif

        // On render les douilles
#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 9)
#endif
            for(auto d : game->douilles)
            {
                Douille_Render(d);
            }

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
            for(i = 0; i < (int)game->trails.size(); ++i)
            {
                if(game->trails[i]->trailType == 0)
                {
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glBindTexture(GL_TEXTURE_2D, clientVar.tex_smokeTrail);
                }
                else if(game->trails[i]->trailType == 1)
                {
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    glBindTexture(GL_TEXTURE_2D, clientVar.tex_glowTrail);
                }
                Trail_Render(game->trails[i]);
            }
        //      ZEVEN_VECTOR_CALL(trails, i, renderBullet());
#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 13)
#endif
            for(i = 0; i < (int)game->trails.size(); ++i)
            {
                if(game->trails[i]->trailType != 0) continue;
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glBindTexture(GL_TEXTURE_2D, clientVar.tex_shotGlow);
                Trail_RenderBullet(game->trails[i]);
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
            cmap->renderWeather();

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
        for(i = 0; i < (int)game->nikeFlashes.size(); ++i)
        {
            NukeFlash_Render(game->nikeFlashes[i]);
        }
        glPopAttrib();

        // La minimap
#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 16)
#endif
            ClientGame_RenderMiniMap(game);

        CVector2i res = dkwGetResolution();
        dkglPushOrtho((float)res[0], (float)res[1]);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // On render le nom des players

        for(i = 0; i < MAX_PLAYER; ++i)
        {
            auto cplayer = static_cast<ClientPlayer*>(game->players[i]);
            if(cplayer && cplayer != game->thisPlayer)
            {
                ClientPlayer_RenderName(cplayer);
            }
        }

        // On va renderer le nb de projectile (temporaire)
    //  dkfBindFont(font);
    //  dkfPrint(50,0,100,0,CString("%i",(int)projectiles.size()).s);
        glPopAttrib();
        dkglPopOrtho();
    }

    //--- On render le voting
    if(game->voting.votingInProgress)
    {
        CVector2i res = dkwGetResolution();
        dkglPushOrtho((float)res[0], (float)res[1]);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //--- Render le vote à l'écran
        if(game->voting.votingInProgress)
        {
            if(cscene->client->blink <= .25f) printRightText((float)res[0] - 10, 10, 30, true, CString("\x9VOTING : %i", (int)(game->voting.remaining)));
            if(game->voting.voted)
            {
                //--- Draw the vote bla bla bla
                printRightText((float)res[0] - 10, 40, 30, true, CString(
                    "\x9%s casted a vote:\n\x8\"%s\"\n\x2Yes: %i\n\x4No: %i",
                    game->voting.votingFrom.s,
                    game->voting.votingWhat.s,
                    game->voting.votingResults[0],
                    game->voting.votingResults[1]
                ));
            }
            else
            {
                //--- Draw the vote bla bla bla
                printRightText((float)res[0] - 10, 40, 30, true, CString(
                    "\x9%s casted a vote:\n\x8\"%s\"\n\x2[F1] Yes: %i\n\x4[F2] No: %i",
                    game->voting.votingFrom.s,
                    game->voting.votingWhat.s,
                    game->voting.votingResults[0],
                    game->voting.votingResults[1]
                ));
            }
        }
        glPopAttrib();
        dkglPopOrtho();
    }
}


static void renderStatsSlice(const CVector4f & sliceColor, char * text1, char* c1, char* c2, char* c3, char* c4, char* c5, char * pingText, int & vPos)
{
    glDisable(GL_TEXTURE_2D);
    glColor4fv(sliceColor.s);
    glBegin(GL_QUADS);
        glVertex2i(150, vPos);
        glVertex2i(150, vPos+22);
        glVertex2i(450, vPos+22);
        glVertex2i(450, vPos);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2i(452, vPos);
        glVertex2i(452, vPos+22);
        glVertex2i(500, vPos+22);
        glVertex2i(500, vPos);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2i(502, vPos);
        glVertex2i(502, vPos+22);
        glVertex2i(550, vPos+22);
        glVertex2i(550, vPos);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2i(552, vPos);
        glVertex2i(552, vPos+22);
        glVertex2i(600, vPos+22);
        glVertex2i(600, vPos);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2i(602, vPos);
        glVertex2i(602, vPos+22);
        glVertex2i(650, vPos+22);
        glVertex2i(650, vPos);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2i(652, vPos);
        glVertex2i(652, vPos+22);
        glVertex2i(700, vPos+22);
        glVertex2i(700, vPos);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2i(702, vPos);
        glVertex2i(702, vPos+22);
        glVertex2i(750, vPos+22);
        glVertex2i(750, vPos);
    glEnd();
    glEnable(GL_TEXTURE_2D);

    glColor3f(1,1,1);

    printLeftText(154, (float)vPos-2, 28, true, CString(text1));
    printRightText(500, (float)vPos-2, 28, true, CString(c1));
    printRightText(550, (float)vPos-2, 28, true, CString(c2));
    printRightText(600, (float)vPos-2, 28, true, CString(c3));
    printRightText(650, (float)vPos-2, 28, true, CString(c4));
    printRightText(700, (float)vPos-2, 28, true, CString(c5));
    printRightText(750, (float)vPos-2, 28, true, CString(pingText));
    // Ping
    vPos += 26;
}

void ClientGame_RenderStats(ClientGame* game)
{
    CVector2i res(800,600);// = dkwGetResolution();
    dkglPushOrtho((float)res[0], (float)res[1]);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBegin(GL_QUADS);
                glColor4f(0,0,0,1);
                glVertex2f(0,0);
                glColor4f(0,0,0,0);
                glVertex2f(0,300);
                glVertex2f(800,300);
                glColor4f(0,0,0,1);
                glVertex2f(800,0);
            glEnd();
            glBegin(GL_QUADS);
                glColor4f(0,0,0,0);
                glVertex2f(0,300);
                glColor4f(0,0,0,1);
                glVertex2f(0,600);
                glVertex2f(800,600);
                glColor4f(0,0,0,0);
                glVertex2f(800,300);
            glEnd();
            glEnable(GL_TEXTURE_2D);

            // On construit la blue team vector et la red team vector puis on tri
            std::vector<Player*> blueTeam;
            std::vector<Player*> redTeam;
            std::vector<Player*> ffaTeam;
            std::vector<Player*> spectatorTeam;
            game->spectatorPing=0;
            game->bluePing = 0;
            game->redPing = 0;
            game->ffaPing = 0;
            int j;
            for (int i=0;i<MAX_PLAYER;++i)
            {
                if (game->players[i])
                {
                    switch (game->players[i]->teamID)
                    {
                    case PLAYER_TEAM_BLUE:
                        for (j=0;j<(int)blueTeam.size();++j)
                        {
                            if (game->players[i]->kills > blueTeam[j]->kills) break;
                        }
                        game->bluePing+= game->players[i]->ping;
                        blueTeam.insert(blueTeam.begin()+j, game->players[i]);
                        break;
                    case PLAYER_TEAM_RED:
                        for (j=0;j<(int)redTeam.size();++j)
                        {
                            if (game->players[i]->kills > redTeam[j]->kills) break;
                        }
                        game->redPing+= game->players[i]->ping;
                        redTeam.insert(redTeam.begin()+j, game->players[i]);
                        break;
                    case PLAYER_TEAM_SPECTATOR:
                        for (j=0;j<(int)spectatorTeam.size();++j)
                        {
                            if (game->players[i]->kills > spectatorTeam[j]->kills) break;
                        }
                        game->spectatorPing+= game->players[i]->ping;
                        spectatorTeam.insert(spectatorTeam.begin()+j, game->players[i]);
                        break;
                    }

                    // On les class tous dans le ffa (sauf spectator)
                    if (game->players[i]->teamID != PLAYER_TEAM_SPECTATOR)
                    {
                        for (j=0;j<(int)ffaTeam.size();++j)
                        {
                            if (game->players[i]->score > ffaTeam[j]->score) break;
                        }
                        game->ffaPing+= game->players[i]->ping;
                        ffaTeam.insert(ffaTeam.begin()+j, game->players[i]);
                    }
                }
            }

            if (blueTeam.size() > 0) game->bluePing /= (int)blueTeam.size();
            if (redTeam.size() > 0) game->redPing /= (int)redTeam.size();
            if (spectatorTeam.size() > 0) game->spectatorPing /= (int)spectatorTeam.size();
            if (ffaTeam.size() > 0) game->ffaPing /= (int)ffaTeam.size();

            // Temporairement juste la liste des joueurs pas trié là pis toute
            dkfBindFont(game->font);
            int vPos = 50;

            // Title [FIX]: Does not use language file


            switch (game->gameType)
            {
            case GAME_TYPE_DM:
                renderStatsSlice(CVector4f(0, 0, 0, .75f), clientVar.lang_playerNameC.s, "Kills", "Death", "Damage", "", "", clientVar.lang_pingC.s, vPos);
                vPos += 10;
                ClientGame_RenderFFA(game, ffaTeam, vPos);
                ClientGame_RenderSpectator(game, spectatorTeam, vPos);
                break;
            case GAME_TYPE_TDM:
                renderStatsSlice(CVector4f(0, 0, 0, .75f), clientVar.lang_playerNameC.s, "Kills", "Death", "Damage", "","", clientVar.lang_pingC.s, vPos);
                vPos += 10;
                if (game->blueScore >= game->redScore)
                {
                    ClientGame_RenderBlueTeam(game, blueTeam, vPos);
                    ClientGame_RenderRedTeam(game, redTeam, vPos);
                }
                else
                {
                    ClientGame_RenderRedTeam(game, redTeam, vPos);
                    ClientGame_RenderBlueTeam(game, blueTeam, vPos);
                }
                ClientGame_RenderSpectator(game, spectatorTeam, vPos);
                break;
            case GAME_TYPE_CTF:
                renderStatsSlice(CVector4f(0, 0, 0, .75f), clientVar.lang_playerNameC.s, "Kills", "Death", "Damage", "Retrn", "Caps", clientVar.lang_pingC.s, vPos);
                vPos += 10;
                if (game->blueWin >= game->redWin)
                {
                    ClientGame_RenderBlueTeam(game, blueTeam, vPos);
                    ClientGame_RenderRedTeam(game, redTeam, vPos);
                }
                else
                {
                    ClientGame_RenderRedTeam(game, redTeam, vPos);
                    ClientGame_RenderBlueTeam(game, blueTeam, vPos);
                }
                ClientGame_RenderSpectator(game, spectatorTeam, vPos);
                break;
            }

            blueTeam.clear();
            redTeam.clear();
            spectatorTeam.clear();
            ffaTeam.clear();
        glPopAttrib();
    dkglPopOrtho();
}

void ClientGame_RenderBlueTeam(ClientGame* game, std::vector<Player*> & teamOrder, int & vPos)
{
    // Blue Team
    renderStatsSlice(CVector4f(0, 0, 1, .75f), clientVar.lang_blueTeamC.s, "","","","",CString("%i", game->blueScore).s, CString(""/*%i", bluePing*33*/).s, vPos);
    for (int j=0;j<(int)teamOrder.size();++j)
    {
        CString showName = teamOrder[j]->name;
        showName.insert("\x1", 0);
        if (teamOrder[j]->status == PLAYER_STATUS_DEAD) showName.insert(CString("(%s) ", clientVar.lang_dead.s).s, 0);

        CString pingStr;
        if (teamOrder[j]->ping*33 < 100) /*"Good"*/
            pingStr = CString("\x2") + teamOrder[j]->ping*33;
        else if (teamOrder[j]->ping*33 < 200) /*"Average"*/
            pingStr = CString("\x9") + teamOrder[j]->ping*33;
        else if (teamOrder[j]->ping*33 < 999) /*"Bad"*/
            pingStr = CString("\x4") + teamOrder[j]->ping*33;
        else
            pingStr = "\x5???";

        renderStatsSlice(   CVector4f(0,0,0,.75f), showName.s,
                            CString("%i",(int)teamOrder[j]->kills).s,
                            CString("%i",(int)teamOrder[j]->deaths).s,
                            CString("%.1f", teamOrder[j]->dmg).s,
                            CString("%i",(int)teamOrder[j]->returns).s,
                            CString("%i",(int)teamOrder[j]->score).s,
                            pingStr.s, vPos);

    }
    vPos += 10;
}

void ClientGame_RenderRedTeam(ClientGame* game, std::vector<Player*> & teamOrder, int & vPos)
{
    // Red Team
    renderStatsSlice(CVector4f(1, 0, 0, .75f), clientVar.lang_redTeamC.s,"","","","", CString("%i", game->redScore).s, CString(""/*%i", redPing*33*/).s, vPos);
    for (int j=0;j<(int)teamOrder.size();++j)
    {
        CString showName = teamOrder[j]->name;
        showName.insert("\x4", 0);
        if (teamOrder[j]->status == PLAYER_STATUS_DEAD) showName.insert(CString("(%s) ", clientVar.lang_dead.s).s, 0);

        CString pingStr;
        if (teamOrder[j]->ping*33 < 100) /*"Good"*/
            pingStr = CString("\x2") + teamOrder[j]->ping*33;
        else if (teamOrder[j]->ping*33 < 200) /*"Average"*/
            pingStr = CString("\x9") + teamOrder[j]->ping*33;
        else if (teamOrder[j]->ping*33 < 999) /*"Bad"*/
            pingStr = CString("\x4") + teamOrder[j]->ping*33;
        else
            pingStr = "\x5???";

        renderStatsSlice(   CVector4f(0,0,0,.75f), showName.s,
                            CString("%i",(int)teamOrder[j]->kills).s,
                            CString("%i",(int)teamOrder[j]->deaths).s,
                            CString("%.1f", teamOrder[j]->dmg).s,
                            CString("%i",(int)teamOrder[j]->returns).s,
                            CString("%i",(int)teamOrder[j]->score).s,
                            pingStr.s, vPos);
    }
    vPos += 10;
}

void ClientGame_RenderFFA(ClientGame* game, std::vector<Player*> & teamOrder, int & vPos)
{

    // All Team
    renderStatsSlice(CVector4f(1, 1, 1, .75f), CString(clientVar.lang_freeForAllC.s, game->redWin + game->blueWin).s,"","","","", CString(""/*%i", blueScore + redScore*/).s, CString("%i", game->ffaPing*33).s, vPos);
    for (int j=0;j<(int)teamOrder.size();++j)
    {
        CString showName = teamOrder[j]->name;
        showName.insert("\x8", 0);
        if (teamOrder[j]->status == PLAYER_STATUS_DEAD) showName.insert((CString("(") + clientVar.lang_dead + ") ").s, 0);
        CString pingStr;
        if (teamOrder[j]->ping*33 < 100) /*"Good"*/
            pingStr = CString("\x2") + teamOrder[j]->ping*33;
        else if (teamOrder[j]->ping*33 < 200) /*"Average"*/
            pingStr = CString("\x9") + teamOrder[j]->ping*33;
        else if (teamOrder[j]->ping*33 < 999) /*"Bad"*/
            pingStr = CString("\x4") + teamOrder[j]->ping*33;
        else
            pingStr = "\x5???";

        renderStatsSlice(CVector4f(0,0,0,.75f), showName.s,CString("%i",(int)teamOrder[j]->score).s, CString("%i", (int)teamOrder[j]->deaths).s, CString("%.1f", teamOrder[j]->dmg).s,"","", pingStr.s, vPos);


    }
    vPos += 10;
}

void ClientGame_RenderSpectator(ClientGame* game, std::vector<Player*> & teamOrder, int & vPos)
{

    // Spectators
    renderStatsSlice(CVector4f(.5f, .5f, .5f, .75f), clientVar.lang_spectatorC.s, "","","","","", CString(""/*%i", spectatorPing*33*/).s, vPos);
    for (int j=0;j<(int)teamOrder.size();++j)
    {
        CString showName = teamOrder[j]->name;
        showName.insert("\x8", 0);
        CString pingStr;
        if (teamOrder[j]->ping*33 < 100) /*"Good"*/
            pingStr = CString("\x2") + teamOrder[j]->ping*33;
        else if (teamOrder[j]->ping*33 < 200) /*"Average"*/
            pingStr = CString("\x9") + teamOrder[j]->ping*33;
        else if (teamOrder[j]->ping*33 < 999) /*"Bad"*/
            pingStr = CString("\x4") + teamOrder[j]->ping*33;
        else
            pingStr = "\x5???";

        renderStatsSlice(   CVector4f(0,0,0,.75f), showName.s,
                            CString("%i",(int)teamOrder[j]->kills).s,
                            CString("%i",(int)teamOrder[j]->deaths).s,
                            CString("%.1f", teamOrder[j]->dmg).s,
                            CString("%i",(int)teamOrder[j]->returns).s,
                            CString("%i",(int)teamOrder[j]->score).s,
                            pingStr.s, vPos);
    }
}

void ClientGame_RenderMiniMap(ClientGame* game)
{
    auto cscene = static_cast<ClientScene*>(scene);
    auto cmap = static_cast<ClientMap*>(game->map);
    CVector2i res(1280, 720);// = dkwGetResolution();

    dkglPushOrtho((float)res[0], (float)res[1]);
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D, cmap->texMap);
    glPushMatrix();
    float scalar = 128.0f / (float)std::max<int>(game->map->size[0], game->map->size[1]);
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
    if(game->thisPlayer)
    {
        if(game->gameType == GAME_TYPE_CTF)
        {
            glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
            glDisable(GL_TEXTURE_2D);
            // render pods, if flags are not on them!
            if(game->map->flagState[0] != -2)
            {
                // rendering blue pod
                glPushMatrix();
                glTranslatef(game->map->flagPodPos[0][0], game->map->flagPodPos[0][1], 0);
                glColor3f(0, 0, 1);
                glBegin(GL_QUADS);
                glVertex2f(-2 / scalar, 2 / scalar);
                glVertex2f(-2 / scalar, -2 / scalar);
                glVertex2f(2 / scalar, -2 / scalar);
                glVertex2f(2 / scalar, 2 / scalar);
                glEnd();
                glPopMatrix();
            }
            if(game->map->flagState[1] != -2)
            {
                // rendering red pod
                glPushMatrix();
                glTranslatef(game->map->flagPodPos[1][0], game->map->flagPodPos[1][1], 0);
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
        glBindTexture(GL_TEXTURE_2D, game->tex_miniMapAllied);
        for(int i = 0; i < MAX_PLAYER; ++i)
        {
            // On render le people sur la map
            if(game->players[i])
            {
                if(game->players[i]->status == PLAYER_STATUS_ALIVE)
                {
                    if((game->players[i]->teamID == game->thisPlayer->teamID || game->thisPlayer->teamID == PLAYER_TEAM_SPECTATOR) && game->gameType != GAME_TYPE_DM || game->thisPlayer == game->players[i])
                    {
                        glPushMatrix();
                        glTranslatef(game->players[i]->currentCF.position[0], game->players[i]->currentCF.position[1], 0);
                        glRotatef(game->players[i]->currentCF.angle, 0, 0, 1);
                        if(game->players[i] == game->thisPlayer)
                        {
                            switch(game->players[i]->teamID)
                            {
                            case PLAYER_TEAM_BLUE:glColor3f(0, 1, 1); break;
                            case PLAYER_TEAM_RED:glColor3f(1, 1, 0); break;
                            }
                        }
                        else
                        {
                            switch(game->players[i]->teamID)
                            {
                            case PLAYER_TEAM_BLUE:glColor3f(game->players[i]->firedShowDelay*.35f, game->players[i]->firedShowDelay*.35f, 1); break;
                            case PLAYER_TEAM_RED:glColor3f(1, game->players[i]->firedShowDelay*.35f, game->players[i]->firedShowDelay*.35f); break;
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
        glBindTexture(GL_TEXTURE_2D, game->tex_miniMapEnemy);
        if(game->thisPlayer->teamID != PLAYER_TEAM_SPECTATOR)
        {
            for(int i = 0; i < MAX_PLAYER; ++i)
            {
                // On render le people sur la map
                if(game->players[i] && game->players[i] != game->thisPlayer)
                {
                    if(game->players[i]->status == PLAYER_STATUS_ALIVE || game->players[i]->firedShowDelay > 0)
                    {
                        if(game->players[i]->teamID != game->thisPlayer->teamID || game->gameType == GAME_TYPE_DM)
                        {
                            glPushMatrix();
                            glTranslatef(game->players[i]->currentCF.position[0], game->players[i]->currentCF.position[1], 0);
                            glRotatef(game->players[i]->currentCF.angle, 0, 0, 1);
                            switch(game->players[i]->teamID)
                            {
                            case PLAYER_TEAM_BLUE:glColor4f(.3f, .3f, 1, game->players[i]->firedShowDelay*.5f); break;
                            case PLAYER_TEAM_RED:glColor4f(1, 0, 0, game->players[i]->firedShowDelay*.5f); break;
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
        if(game->gameType == GAME_TYPE_CTF)
        {
            //      if (map->flagState[0] == -2) map->flagPos[0] = map->flagPodPos[0];
            //      if (map->flagState[1] == -2) map->flagPos[1] = map->flagPodPos[1];
            //      if (map->flagState[0] >= 0) if (players[map->flagState[0]]) map->flagPos[0] = players[map->flagState[0]]->currentCF.position;
            //      if (map->flagState[1] >= 0) if (players[map->flagState[1]]) map->flagPos[1] = players[map->flagState[1]]->currentCF.position;

            if(game->thisPlayer->teamID == PLAYER_TEAM_RED ||
                (game->thisPlayer->teamID == PLAYER_TEAM_BLUE && game->map->flagState[0] < 0))
            {
                glBindTexture(GL_TEXTURE_2D, cscene->client->tex_blueFlag);
                glPushMatrix();
                glTranslatef(game->map->flagPos[0][0], game->map->flagPos[0][1], 0);
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
            if(game->thisPlayer->teamID == PLAYER_TEAM_BLUE ||
                (game->thisPlayer->teamID == PLAYER_TEAM_RED && game->map->flagState[1] < 0))
            {
                glBindTexture(GL_TEXTURE_2D, cscene->client->tex_redFlag);
                glPushMatrix();
                glTranslatef(game->map->flagPos[1][0], game->map->flagPos[1][1], 0);
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


void MultOglMatrix(CMatrix3x3f m)
{
    float Matrix[16] = {
        m.s[0], m.s[1], m.s[2], 0,
        m.s[3], m.s[4], m.s[5], 0,
        m.s[6], m.s[7], m.s[8], 0,
        0,    0,    0,    1 };

    glMultMatrixf(Matrix);
}

void ClientPlayer_Render(ClientPlayer* player)
{
    auto game = player->game;
    auto cgame = static_cast<ClientGame*>(player->game);
    if(player->status == PLAYER_STATUS_ALIVE)
    {
        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT);
        //--- TEMP render path with his bot

            // On render son shadow :)
        if(gameVar.r_playerShadow)
        {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, player->tex_baboShadow);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_LIGHTING);
            glColor4f(1, 1, 1, .75f);
            glDepthMask(GL_FALSE);
            glPushMatrix();
            glTranslatef(player->currentCF.position[0] + .1f, player->currentCF.position[1] - .1f, .025f);
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
        if((game->gameType != GAME_TYPE_DM) && (gameVar.cl_teamIndicatorType == 1 || (gameVar.cl_teamIndicatorType == 2 && player->teamID == cgame->thisPlayer->teamID) || (gameVar.cl_teamIndicatorType > 0 && cgame->thisPlayer->teamID == PLAYER_TEAM_SPECTATOR)))
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
                if(player->teamID == PLAYER_TEAM_RED)
                {
                    glColor3f(1, 0, 0);
                }
                else if(player->teamID == PLAYER_TEAM_BLUE)
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
            glTranslatef(player->currentCF.position[0], player->currentCF.position[1], player->currentCF.position[2]);
            glBindTexture(GL_TEXTURE_2D, player->tex_baboHalo);
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
        glTranslatef(player->currentCF.position[0], player->currentCF.position[1], player->currentCF.position[2]);
        MultOglMatrix(player->matrix);
        glEnable(GL_COLOR_MATERIAL);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, player->tex_skin);
        glColor3f(1, 1, 1);
        glPolygonMode(GL_FRONT, GL_FILL);
        //gluQuadricTexture(qObj, true);
        //gluSphere(qObj, .25f, 16, 16);
        dkglDrawSphere(0.25f, 16, 16, GL_TRIANGLES);

        //--- On pogne la position sur l'clientVar.dkpp_ran
        CVector3f screenPos = dkglProject(CVector3f(0, 0, 0));
        CVector2i res = dkwGetResolution();
        player->onScreenPos[0] = (int)screenPos[0];
        player->onScreenPos[1] = res[1] - (int)screenPos[1];
        glPopMatrix();

        glPushMatrix();
        // Draw the gun
        glPolygonMode(GL_FRONT, GL_FILL);
        glTranslatef(player->currentCF.position[0], player->currentCF.position[1], 0);
        glRotatef(player->currentCF.angle, 0, 0, 1);
        glScalef(0.005f, 0.005f, 0.005f);
        auto cweapon = static_cast<ClientWeapon*>(player->weapon);
        auto cmeleeWeapon = static_cast<ClientWeapon*>(player->meleeWeapon);
        if(cweapon)
        {
            ClientWeapon_Render(cweapon);
        }
        if(cmeleeWeapon)
        {
            ClientWeapon_Render(cmeleeWeapon);
        }
        glRotatef(-90, 0, 0, 1);

        glPopMatrix();

        // Le flag si c'est le cas
        if(game->gameType == GAME_TYPE_CTF)
        {
            if(game->map)
            {
                auto cmap = static_cast<ClientMap*>(game->map);
                if(game->map->flagState[0] == player->playerID)
                {
                    cmap->flagAngle[0] = player->currentCF.angle - 90;
                    //game->map->renderFlag(0);
                    //dkoRender(game->map->dko_flag[0], game->map->flagAnim);
                }
                if(game->map->flagState[1] == player->playerID)
                {
                    cmap->flagAngle[1] = player->currentCF.angle - 90;
                    //game->map->renderFlag(1);
                    //dkoRender(game->map->dko_flag[1], game->map->flagAnim);
                }
            }
        }
        glPopAttrib();
    }
}

void ClientPlayer_RenderName(ClientPlayer* player)
{
    auto game = player->game;
    auto cgame = static_cast<ClientGame*>(player->game);
    auto cscene = static_cast<ClientScene*>(scene);
    if(gameVar.sv_showEnemyTag && cgame->thisPlayer)
    {
        if(!player->isThisPlayer && player->teamID != PLAYER_TEAM_SPECTATOR && player->teamID != cgame->thisPlayer->teamID && game->gameType != GAME_TYPE_DM)
        {
            //--- We don't print it !!!!!!
            return;
        }
    }
    if(player->status == PLAYER_STATUS_ALIVE)
    {
        CVector2i res = dkwGetResolution();
        if(player->onScreenPos[0] > 0 && player->onScreenPos[0] < res[0] &&
            player->onScreenPos[1] > 0 && player->onScreenPos[1] < res[1])
        {
            CString showName;
            if(player->ping > 12 && cscene->client->blink < .25f) showName.insert(CString(" \x5%s", clientVar.lang_laggerC.s).s, 0);
            if(gameVar.r_maxNameLenOverBabo > 0 && player->name.len() > gameVar.r_maxNameLenOverBabo)
            {
                CString sname(CString("%%.%is[...]", gameVar.r_maxNameLenOverBabo).s, player->name.s);
                showName.insert(sname.s, 0);
            }
            else
                showName.insert(player->name.s, 0);
            showName.insert("\x8", 0);
            if(player->ping > 12 && cscene->client->blink < .25f) showName.insert(CString("\x5%s ", clientVar.lang_laggerC.s).s, 0);
            printCenterText((float)player->onScreenPos[0], (float)player->onScreenPos[1] - 28, 28, true, showName);
        }
    }

    //--- The life of this player
    if(cgame->thisPlayer && player->status == PLAYER_STATUS_ALIVE)
    {
        if((!player->isThisPlayer && player->teamID == cgame->thisPlayer->teamID && game->gameType != GAME_TYPE_DM) ||
            player->teamID == PLAYER_TEAM_SPECTATOR)
        {
            glColor3f(1, 1, 1);
            renderTexturedQuad(player->onScreenPos[0] - 15, player->onScreenPos[1] - 8, 30, 7, 0);
            glColor3f(0, 0, 0);
            renderTexturedQuad(player->onScreenPos[0] - 14, player->onScreenPos[1] - 7, 28, 5, 0);
            if(player->life > .25f || cscene->client->blink < .25f)
            {
                glColor3f(1 - player->life, player->life, 0);
                renderTexturedQuad(player->onScreenPos[0] - 14, player->onScreenPos[1] - 7, (int)(player->life*28.0f), 5, 0);
            }
        }
    }
}

void Client_Render(Client* client, float & alphaScope)
{
    auto game = client->game;
    int i;
    CVector2i res = dkwGetResolution();

    if(game->thisPlayer)
    {
        if(game->thisPlayer->status == PLAYER_STATUS_ALIVE)
        {
            if(game->map)
            {
                auto cmap = static_cast<ClientMap*>(game->map);
                cmap->camLookAt = (
                    game->thisPlayer->currentCF.position * 5 +
                    game->thisPlayer->currentCF.mousePosOnMap * 4) / 9.0f;
                if(cmap->camLookAt[0] < 0) cmap->camLookAt[0] = 0;
                if(cmap->camLookAt[1] < -1) cmap->camLookAt[1] = -1;
                if(cmap->camLookAt[0] > (float)game->map->size[0]) cmap->camLookAt[0] = (float)game->map->size[0];
                if(cmap->camLookAt[1] > (float)game->map->size[1] + 1) cmap->camLookAt[1] = (float)game->map->size[1] + 1;
            }
        }
    }

    // C'est côté client qu'on fait ça ;)
    if(client->isConnected) ClientGame_Render(game);

    // LE SNIPER SCOPE
    CVector2i cursor = dkwGetCursorPos_main();
    int xM = (int)(((float)cursor[0] / (float)res[0])*800.0f);
    int yM = (int)(((float)cursor[1] / (float)res[1])*600.0f);
    dkglPushOrtho(800, 600);
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //--- Sniper scope!!
    if(game->thisPlayer && game->thisPlayer->status == PLAYER_STATUS_ALIVE)
    {
        if(game->thisPlayer->weapon && game->thisPlayer->weapon->weaponID == WEAPON_SNIPER)
        {
            auto cmap = static_cast<ClientMap*>(game->map);
            if(cmap && cmap->camPos[2] > 8)
            {
                alphaScope = 10 - (cmap->camPos[2] - 2);
                alphaScope = (alphaScope > 0) ? 1 - (alphaScope / 2) : 1;
                glColor4f(0, 0, 0, alphaScope);
                if(!(menuManager.root && menuManager.root->visible) && !client->showMenu)
                {
                    // render the scope view
                    renderTexturedQuad(xM - 128, yM - 128, 256, 256, clientVar.tex_sniperScope);
                    renderTexturedQuad(0, 0, 800, yM - 128, 0);
                    renderTexturedQuad(0, yM + 128, 800, 600 - (yM + 128), 0);
                    renderTexturedQuad(0, yM - 128, xM - 128, 256, 0);
                    renderTexturedQuad(xM + 128, yM - 128, 800 - (xM + 128), 256, 0);
                }
                else
                {
                    // render black background in the menu
                    renderTexturedQuad(0, 0, 800, 600, 0);
                }
            }
        }
    }

    glPopAttrib();
    dkglPopOrtho();

    // Si on doit spawner on marque dans combient de temps
    if(game->thisPlayer)
    {
        dkglPushOrtho(1280, 720);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // Si on c faite tirer dessus
        if(game->thisPlayer->screenHit > 0)
        {
            glColor4f(1, 1, 1, game->thisPlayer->screenHit * 3);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, client->tex_screenHit);
            glBegin(GL_QUADS);
            glTexCoord2i(0, 1);
            glVertex2i(0, 0);
            glTexCoord2i(0, 0);
            glVertex2i(0, 600);
            glTexCoord2i(1, 0);
            glVertex2i(800, 600);
            glTexCoord2i(1, 1);
            glVertex2i(800, 0);
            glEnd();
        }

#ifdef RENDER_LAYER_TOGGLE
        if(renderToggle >= 16)
#endif
            if(game->thisPlayer->status == PLAYER_STATUS_ALIVE)
            {
                // Quand on reload le gun il faut le marquer
                if(game->thisPlayer->weapon)
                {
                    if(game->thisPlayer->weapon->currentFireDelay > 0 && game->thisPlayer->weapon->fireDelay >= 1.0f)
                    {
                        glDisable(GL_TEXTURE_2D);
                        glColor4f(1, 1, 1, .5f);
                        glBegin(GL_QUADS);
                        glVertex2f(400 - 100 - 5, 440 - 5);
                        glVertex2f(400 - 100 - 5, 454 + 5);
                        glVertex2f(400 - 100 + 200 + 10, 454 + 5);
                        glVertex2f(400 - 100 + 200 + 10, 440 - 5);
                        glEnd();
                        glColor4f(0, 0, 0, .5f);
                        glBegin(GL_QUADS);
                        glVertex2f(400 - 100 - 1, 440 - 1);
                        glVertex2f(400 - 100 - 1, 454 + 1);
                        glVertex2f(400 - 100 + 200 + 2, 454 + 1);
                        glVertex2f(400 - 100 + 200 + 2, 440 - 1);
                        glEnd();
                        glEnable(GL_TEXTURE_2D);
                        glColor4f(.5f, 1, .5f, game->thisPlayer->weapon->currentFireDelay / game->thisPlayer->weapon->fireDelay*.75f + .25f);
                        if(client->blink < .25f) printCenterText(400, 400, 32, true, clientVar.lang_reloading);
                        glDisable(GL_TEXTURE_2D);
                        // La progress bar
                        // La progress bar
                        glBegin(GL_QUADS);
                        glVertex2f(400 - 100, 440);
                        glVertex2f(400 - 100, 454);
                        glVertex2f(400 - 100 + (1 - game->thisPlayer->weapon->currentFireDelay / game->thisPlayer->weapon->fireDelay) * 200, 454);
                        glVertex2f(400 - 100 + (1 - game->thisPlayer->weapon->currentFireDelay / game->thisPlayer->weapon->fireDelay) * 200, 440);
                        glEnd();
                    }
                    else if(game->thisPlayer->grenadeDelay > 0)
                    {
                        glDisable(GL_TEXTURE_2D);
                        glColor4f(1, 1, 1, .5f);
                        glBegin(GL_QUADS);
                        glVertex2f(400 - 100 - 5, 440 - 5);
                        glVertex2f(400 - 100 - 5, 454 + 5);
                        glVertex2f(400 - 100 + 200 + 10, 454 + 5);
                        glVertex2f(400 - 100 + 200 + 10, 440 - 5);
                        glEnd();
                        glColor4f(0, 0, 0, .5f);
                        glBegin(GL_QUADS);
                        glVertex2f(400 - 100 - 1, 440 - 1);
                        glVertex2f(400 - 100 - 1, 454 + 1);
                        glVertex2f(400 - 100 + 200 + 2, 454 + 1);
                        glVertex2f(400 - 100 + 200 + 2, 440 - 1);
                        glEnd();
                        glEnable(GL_TEXTURE_2D);
                        glColor4f(.5f, 1, .5f, game->thisPlayer->grenadeDelay / gameVar.weapons[WEAPON_GRENADE]->fireDelay*.75f + .25f);
                        if(client->blink < .25f) printCenterText(400, 400, 32, true, clientVar.lang_reloading);
                        glDisable(GL_TEXTURE_2D);
                        // La progress bar
                        glBegin(GL_QUADS);
                        glVertex2f(400 - 100, 440);
                        glVertex2f(400 - 100, 454);
                        glVertex2f(400 - 100 + (1 - game->thisPlayer->grenadeDelay / gameVar.weapons[WEAPON_GRENADE]->fireDelay) * 200, 454);
                        glVertex2f(400 - 100 + (1 - game->thisPlayer->grenadeDelay / gameVar.weapons[WEAPON_GRENADE]->fireDelay) * 200, 440);
                        glEnd();
                    }
                    else if(game->thisPlayer->weapon->weaponID == WEAPON_SHOTGUN && game->thisPlayer->weapon->currentFireDelay > 0)
                    {
                        glDisable(GL_TEXTURE_2D);
                        glColor4f(1, 1, 1, .5f);
                        glBegin(GL_QUADS);
                        glVertex2f(400 - 100 - 5, 440 - 5);
                        glVertex2f(400 - 100 - 5, 454 + 5);
                        glVertex2f(400 - 100 + 200 + 10, 454 + 5);
                        glVertex2f(400 - 100 + 200 + 10, 440 - 5);
                        glEnd();
                        glColor4f(0, 0, 0, .5f);
                        glBegin(GL_QUADS);
                        glVertex2f(400 - 100 - 1, 440 - 1);
                        glVertex2f(400 - 100 - 1, 454 + 1);
                        glVertex2f(400 - 100 + 200 + 2, 454 + 1);
                        glVertex2f(400 - 100 + 200 + 2, 440 - 1);
                        glEnd();
                        glEnable(GL_TEXTURE_2D);
                        glColor4f(.5f, 1, .5f, game->thisPlayer->weapon->currentFireDelay / game->thisPlayer->weapon->fireDelay*.75f + .25f);
                        if(client->blink < .25f) printCenterText(400, 400, 32, true, clientVar.lang_reloading);
                        glDisable(GL_TEXTURE_2D);
                        // La progress bar
                        glBegin(GL_QUADS);
                        glVertex2f(400 - 100, 440);
                        glVertex2f(400 - 100, 454);
                        glVertex2f(400 - 100 + (1 - game->thisPlayer->weapon->currentFireDelay / 3) * 200, 454);
                        glVertex2f(400 - 100 + (1 - game->thisPlayer->weapon->currentFireDelay / 3) * 200, 440);
                        glEnd();
                    }
                    else if(game->thisPlayer->meleeDelay > 0)
                    {
                        glDisable(GL_TEXTURE_2D);
                        glColor4f(1, 1, 1, .5f);
                        glBegin(GL_QUADS);
                        glVertex2f(400 - 100 - 5, 440 - 5);
                        glVertex2f(400 - 100 - 5, 454 + 5);
                        glVertex2f(400 - 100 + 200 + 10, 454 + 5);
                        glVertex2f(400 - 100 + 200 + 10, 440 - 5);
                        glEnd();
                        glColor4f(0, 0, 0, .5f);
                        glBegin(GL_QUADS);
                        glVertex2f(400 - 100 - 1, 440 - 1);
                        glVertex2f(400 - 100 - 1, 454 + 1);
                        glVertex2f(400 - 100 + 200 + 2, 454 + 1);
                        glVertex2f(400 - 100 + 200 + 2, 440 - 1);
                        glEnd();
                        glEnable(GL_TEXTURE_2D);
                        glColor4f(.5f, 1, .5f, game->thisPlayer->meleeDelay / game->thisPlayer->meleeWeapon->fireDelay*.75f + .25f);
                        if(client->blink < .25f) printCenterText(400, 400, 32, true, clientVar.lang_reloading);
                        glDisable(GL_TEXTURE_2D);
                        // La progress bar
                        glBegin(GL_QUADS);
                        glVertex2f(400 - 100, 440);
                        glVertex2f(400 - 100, 454);
                        glVertex2f(400 - 100 + (1 - game->thisPlayer->meleeDelay / game->thisPlayer->meleeWeapon->fireDelay) * 200, 454);
                        glVertex2f(400 - 100 + (1 - game->thisPlayer->meleeDelay / game->thisPlayer->meleeWeapon->fireDelay) * 200, 440);
                        glEnd();
                    }
                }

                glDisable(GL_TEXTURE_2D);

                // On affiche sa vie à droite
                glBegin(GL_QUADS);
                glColor3f(1, 1, 1);
                glVertex2f(760, 390);
                glVertex2f(760, 589);
                glVertex2f(789, 589);
                glVertex2f(789, 390);
                glColor3f(0, 0, 0);
                glVertex2f(762, 392);
                glVertex2f(762, 587);
                glVertex2f(787, 587);
                glVertex2f(787, 392);
                // La couleur celon sa vie
                if(game->thisPlayer->life > .25f || client->blink < .25f)
                {
                    glColor3f(1 - game->thisPlayer->life, game->thisPlayer->life, 0);
                    glVertex2f(764, 585 - game->thisPlayer->life * 191);
                    glVertex2f(764, 585);
                    glVertex2f(785, 585);
                    glVertex2f(785, 585 - game->thisPlayer->life * 191);
                }
                glEnd();

                // Le heat du gun (ChainGun | FlameThrower)
                if(game->thisPlayer->weapon)
                {
                    if(game->thisPlayer->weapon->weaponID == WEAPON_CHAIN_GUN)
                    {
                        glBegin(GL_QUADS);
                        glColor4f(1, 1, 1, 1 - game->thisPlayer->weapon->chainOverHeat*.5f);
                        glVertex2f(760, 390 - 200);
                        glVertex2f(760, 589 - 200);
                        glVertex2f(789, 589 - 200);
                        glVertex2f(789, 390 - 200);
                        glColor4f(0, 0, 0, 1 - game->thisPlayer->weapon->chainOverHeat*.5f);
                        glVertex2f(762, 392 - 200);
                        glVertex2f(762, 587 - 200);
                        glVertex2f(787, 587 - 200);
                        glVertex2f(787, 392 - 200);
                        // La couleur celon sa vie
                        if((game->thisPlayer->weapon->overHeated && client->blink < .25f) || !game->thisPlayer->weapon->overHeated)
                        {
                            glColor4f(1 - game->thisPlayer->weapon->chainOverHeat, game->thisPlayer->weapon->chainOverHeat, game->thisPlayer->weapon->chainOverHeat, 1 - game->thisPlayer->weapon->chainOverHeat*.5f);
                            glVertex2f(764, 585 - 200 - game->thisPlayer->weapon->chainOverHeat * 191);
                            glVertex2f(764, 585 - 200);
                            glVertex2f(785, 585 - 200);
                            glVertex2f(785, 585 - 200 - game->thisPlayer->weapon->chainOverHeat * 191);
                        }
                        glEnd();
                    }
                }

                // Le nb de grenade quil lui reste
                if(game->thisPlayer->nbGrenadeLeft > 0)
                {
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, client->tex_grenadeLeft);
                    glPushMatrix();
                    glTranslatef(686 + 32, 526 + 32, 0);
                    if(game->thisPlayer->lastShootWasNade)
                    {
                        glScalef(32 + game->thisPlayer->grenadeDelay * 16, 32 + game->thisPlayer->grenadeDelay * 16, 0);
                    }
                    else
                    {
                        glScalef(32, 32, 0);
                    }
                    glBegin(GL_QUADS);
                    glColor3f(1, 1, 1);
                    glTexCoord2f(0, 1);
                    glVertex2f(-1, -1);
                    glTexCoord2f(0, 0);
                    glVertex2f(-1, 1);
                    glTexCoord2f(1, 0);
                    glVertex2f(1, 1);
                    glTexCoord2f(1, 1);
                    glVertex2f(1, -1);
                    glEnd();
                    glPopMatrix();
                    printCenterText(686 + 32, 526 + 32 - 16, 32, true, CString("%i", game->thisPlayer->nbGrenadeLeft));
                }

                // Le nb de molotov quil lui reste
                if(game->thisPlayer->nbMolotovLeft > 0 && gameVar.sv_enableMolotov)
                {
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, client->tex_molotovLeft);
                    glPushMatrix();
                    glTranslatef(686 + 32, 474 + 32, 0);
                    if(!game->thisPlayer->lastShootWasNade)
                    {
                        glScalef(32 + game->thisPlayer->grenadeDelay * 16, 32 + game->thisPlayer->grenadeDelay * 16, 0);
                    }
                    else
                    {
                        glScalef(32, 32, 0);
                    }
                    glBegin(GL_QUADS);
                    glColor3f(1, 1, 1);
                    glTexCoord2f(0, 1);
                    glVertex2f(-1, -1);
                    glTexCoord2f(0, 0);
                    glVertex2f(-1, 1);
                    glTexCoord2f(1, 0);
                    glVertex2f(1, 1);
                    glTexCoord2f(1, 1);
                    glVertex2f(1, -1);
                    glEnd();
                    glPopMatrix();
                    printCenterText(686 + 32, 474 + 32 - 16, 32, true, CString("%i", game->thisPlayer->nbMolotovLeft));
                }

                // Le nb de balle de shotgun quil lui reste
                if((game->thisPlayer->weapon->weaponID == WEAPON_SHOTGUN) &&
                    (gameVar.sv_enableShotgunReload == true))
                {
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, client->tex_shotgunLeft);
                    glPushMatrix();
                    glTranslatef(686 + 32, 422 + 32, 0);
                    glScalef(32 + game->thisPlayer->weapon->currentFireDelay * 16, 32 + game->thisPlayer->weapon->currentFireDelay * 16, 0);
                    glBegin(GL_QUADS);
                    glColor3f(1, 1, 1);
                    glTexCoord2f(0, 1);
                    glVertex2f(-1, -1);
                    glTexCoord2f(0, 0);
                    glVertex2f(-1, 1);
                    glTexCoord2f(1, 0);
                    glVertex2f(1, 1);
                    glTexCoord2f(1, 1);
                    glVertex2f(1, -1);
                    glEnd();
                    glPopMatrix();
                    printCenterText(686 + 32, 422 + 32 - 16, 32, true, CString("%i", 6 - game->thisPlayer->weapon->shotInc));
                }
            }
            else if((
                game->thisPlayer->teamID == PLAYER_TEAM_BLUE ||
                game->thisPlayer->teamID == PLAYER_TEAM_RED) &&
                game->thisPlayer->status == PLAYER_STATUS_DEAD && !game->thisPlayer->spawnRequested)
            {
                glColor3f(1, 1, 1);
                if(game->thisPlayer->timeToSpawn > 0) printCenterText(400, 200, 64, true, CString(clientVar.lang_spawnIn.s, ((int)game->thisPlayer->timeToSpawn + 1) / 60, ((int)(game->thisPlayer->timeToSpawn + 1) % 60)));
                else if(!gameVar.sv_forceRespawn) printCenterText(400, 200, 64, true, CString("Press shoot key [%s] to respawn", keyManager.getKeyName(clientVar.k_shoot).s));
            }

        //--- Auto balance
        if(gameVar.sv_autoBalance && client->autoBalanceTimer > 0 && client->blink < .25f)
        {
            glColor3f(1, 1, 1);
            printCenterText(400, 0, 64, true, CString("Autobalance in %i seconds", (int)client->autoBalanceTimer));
        }

        /*printCenterText(200,100,20,CString("Time played: %.2f", game->thisPlayer->timePlayedCurGame));
        printCenterText(200,124,20,CString("Flag attempts: %d", game->thisPlayer->flagAttempts));*/

        glPopAttrib();
        dkglPopOrtho();

        if(gameVar.r_showLatency)
        {
            dkglPushOrtho((float)res[0], (float)res[1]);
            glPushAttrib(GL_LINE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
            glEnable(GL_BLEND);
            float left = res[0] - (float)(PING_LOG_SIZE)-15;
            float bottom = 65;
            float height = 40;
            float scaledHeight = height / 1000.0f;
            glDisable(GL_TEXTURE_2D);
            glBegin(GL_QUADS);
            glColor4f(0.7f, 0.7f, 0.7f, 0.4f);
            glVertex2f(left, bottom - height);
            glVertex2f(left, bottom);
            glVertex2f(left + PING_LOG_SIZE, bottom);
            glVertex2f(left + PING_LOG_SIZE, bottom - height);
            glEnd();

            glLineWidth(1.0f);
            glBegin(GL_LINES);

            glColor4f(1.0f, 0.0f, 0.0f, 0.6f);
            glVertex3f(left - 1, bottom, 0.0f);
            glVertex3f(left - 1, bottom - height, 0.0f);

            glColor4f(1.0f, 1.0f, 0.0f, 0.6f);
            glVertex3f(left - 1, bottom, 0.0f);
            glVertex3f(left - 1, bottom - 200 * scaledHeight, 0.0f);

            glColor4f(0.0f, 1.0f, 0.0f, 0.6f);
            glVertex3f(left - 1, bottom, 0.0f);
            glVertex3f(left - 1, bottom - 100 * scaledHeight, 0.0f);

            int j = game->thisPlayer->pingLogID;//, k = 0;
            int ping;
            for(int i = 0; i < PING_LOG_SIZE; i++, j++)//, k += 2)
            {
                if(j >= PING_LOG_SIZE)
                    j = 0;
                ping = game->thisPlayer->pingLog[j] * 33;
                if(ping > 1000 || ping < 0)
                    ping = 1000;
                if(ping <= 100)
                    glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
                else if(ping <= 200)
                    glColor4f(1.0f, 1.0f, 0.0f, 0.5f);
                else
                    glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
                glVertex3f(left + i, bottom, 0.0f);
                glVertex3f(left + i, bottom - 1 - ping * scaledHeight, 0.0f);
            }
            glEnd();
            /*glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
            glBegin(GL_LINES);
                glVertex3f(left, bottom-game->thisPlayer->avgPing*(height/1000.0f), 0.0f);
                glVertex3f(left+PING_LOG_SIZE, bottom-game->thisPlayer->avgPing*(height/1000.0f), 0.0f);
            glEnd();
            glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
            printLeftText(left, bottom + 5, 20, CString("Ang. ping: %d", game->thisPlayer->avgPing));*/
            glPopAttrib();
            dkglPopOrtho();
        }

        // On render les chat message par dessus le jeu
        dkglPushOrtho((float)res[0], (float)res[1]);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Afficher les win des team ou les score si en TDM ou CTF ou SND
        glColor3f(1, 1, 1);
        switch(game->gameType)
        {
        case GAME_TYPE_DM:
            // Total Time left
            printLeftText(5, 5, 64, true, CString("%01i:%02i", (int)((game->gameTimeLeft + 1) / 60), (int)(game->gameTimeLeft + 1) % 60));
            break;
        case GAME_TYPE_TDM:
            // Total Time left
            printLeftText(5, 5, 64, true, CString("%01i:%02i", (int)((game->gameTimeLeft + 1) / 60), (int)(game->gameTimeLeft + 1) % 60));
            // Score left
            if(game->blueScore >= game->redScore)
            {
                renderTexturedQuad(5, 5 + 64, 64, 64, client->tex_blueFlag);
                printLeftText(5 + 64 + 5, 5 + 64, 64, true, CString("%i/%i", game->blueScore, gameVar.sv_scoreLimit));
                renderTexturedQuad(5, 5 + 64 + 64, 64, 64, client->tex_redFlag);
                printLeftText(5 + 64 + 5, 5 + 64 + 64, 64, true, CString("%i/%i", game->redScore, gameVar.sv_scoreLimit));
            }
            else
            {
                renderTexturedQuad(5, 5 + 64, 64, 64, client->tex_redFlag);
                printLeftText(5 + 64 + 5, 5 + 64, 64, true, CString("%i/%i", game->redScore, gameVar.sv_scoreLimit));
                renderTexturedQuad(5, 5 + 64 + 64, 64, 64, client->tex_blueFlag);
                printLeftText(5 + 64 + 5, 5 + 64 + 64, 64, true, CString("%i/%i", game->blueScore, gameVar.sv_scoreLimit));
            }
            break;
        case GAME_TYPE_CTF:
            // Total Time left
            printLeftText(5, 5, 64, true, CString("%01i:%02i", (int)((game->gameTimeLeft + 1) / 60), (int)(game->gameTimeLeft + 1) % 60));
            // win left
            if(game->blueWin >= game->redWin)
            {
                renderTexturedQuad(5, 5 + 64, 64, 64, client->tex_blueFlag);
                printLeftText(5 + 64 + 5, 5 + 64, 64, true, CString("%i/%i", game->blueWin, gameVar.sv_winLimit));
                renderTexturedQuad(5, 5 + 64 + 64, 64, 64, client->tex_redFlag);
                printLeftText(5 + 64 + 5, 5 + 64 + 64, 64, true, CString("%i/%i", game->redWin, gameVar.sv_winLimit));
            }
            else
            {
                renderTexturedQuad(5, 5 + 64, 64, 64, client->tex_redFlag);
                printLeftText(5 + 64 + 5, 5 + 64, 64, true, CString("%i/%i", game->redWin, gameVar.sv_winLimit));
                renderTexturedQuad(5, 5 + 64 + 64, 64, 64, client->tex_blueFlag);
                printLeftText(5 + 64 + 5, 5 + 64 + 64, 64, true, CString("%i/%i", game->blueWin, gameVar.sv_winLimit));
            }
            break;
        }

        float textSize = (float)gameVar.r_chatTextSize;
        float xPos = ((float)res[0] / 800.0f) * 128 + 40;
        float yPos = res[1] - (((float)res[1] / 600.0f) * 128 + 40) - 60;

        for(i = 0; i < (int)client->chatMessages.size(); ++i)
        {
            if(client->chatMessages[i].duration > 1)
            {
                glColor4f(0, 0, 0, .5f);
            }
            else
            {
                glColor4f(0, 0, 0, client->chatMessages[i].duration*.5f);
            }
            glDisable(GL_TEXTURE_2D);
            glBegin(GL_QUADS);

            float chatWidth = dkfGetStringWidth(textSize, client->chatMessages[i].message.s);

            glVertex2f(8, yPos - (float)((client->chatMessages.size() - i - 1) * textSize) + 1);
            glVertex2f(8, yPos - (float)((client->chatMessages.size() - i - 1) * textSize) + textSize - 1);
            glColor4f(0, 0, 0, 0);
            glVertex2f(8 + chatWidth, yPos - (float)((client->chatMessages.size() - i - 1) * textSize) + textSize - 1);
            glVertex2f(8 + chatWidth, yPos - (float)((client->chatMessages.size() - i - 1) * textSize) + 1);

            glEnd();
            if(client->chatMessages[i].duration > 1)
            {
                glColor3f(1, 1, 1);
            }
            else
            {
                glColor4f(1, 1, 1, client->chatMessages[i].duration);
            }
            // On l'écris à peut pret au tier de l'écran à gauche
            glEnable(GL_TEXTURE_2D);

            printLeftText(10, yPos - (float)(client->chatMessages.size() - i - 1) * textSize, textSize, true, client->chatMessages[i].message);
        }

        // Si on est apres chatter
        if(client->chatting.haveFocus())
        {
            glDisable(GL_TEXTURE_2D);
            glBegin(GL_QUADS);
            glColor4f(0, 0, 0, .5f);
            glVertex2f(8, yPos + 32 + 1);
            glVertex2f(8, yPos + 32 + 27);
            glColor4f(0, 0, 0, 0);
            glVertex2f(500, yPos + 32 + 27);
            glVertex2f(500, yPos + 32 + 1);
            glEnd();
            glEnable(GL_TEXTURE_2D);
            glColor3f(1, 1, 1);
            CString chattingTo;
            if(client->isChattingTeam)
                chattingTo = "sayteam : ";
            else
                chattingTo = "say : ";


            float chattingToWidth = dkfGetStringWidth(28, chattingTo.s);
            printLeftText(10, yPos + 32, 28, true, chattingTo);
            client->chatting.print(28, 10 + chattingToWidth, yPos + 32, 0);
        }


        // Les events
        for(i = 0; i < (int)client->eventMessages.size(); ++i)
        {
            if(client->eventMessages[i].duration > 1)
            {
                glColor3f(1, 1, 1);
            }
            else
            {
                glColor4f(1, 1, 1, client->eventMessages[i].duration);
            }
            // On l'écris à peut pret au 2 tier de l'écran à gauche

            float eventTextSize = (float)gameVar.r_eventTextSize;

            if(gameVar.r_showEventText)
            {
                printLeftText(xPos, (float)res[1] - (float)(client->eventMessages.size() - i - 1) * eventTextSize - 20 - eventTextSize, eventTextSize, true, client->eventMessages[i].message);
            }
        }

        glPopAttrib();
        dkglPopOrtho();

        // Les stats
        if(game->showStats)
        {
            ClientGame_RenderStats(game);

            if(client->blink < .25f)
            {
                dkglPushOrtho(800, 600);
                glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
                glEnable(GL_BLEND);
                glColor3f(1, 1, 1);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                switch(game->roundState)
                {
                case GAME_PLAYING: break;
                case GAME_BLUE_WIN:
                    printCenterText(475, 5, 64, true, clientVar.lang_blueTeamWin);
                    break;
                case GAME_RED_WIN:
                    printCenterText(475, 5, 64, true, clientVar.lang_redTeamWin);
                    break;
                case GAME_DRAW:
                    printCenterText(475, 5, 64, true, clientVar.lang_roundDraw);
                    break;
                case GAME_MAP_CHANGE:
                    printCenterText(475, 5, 64, true, clientVar.lang_changingMap);
                    break;
                }
                glPopAttrib();
                dkglPopOrtho();
            }
        }
    }

    // On render le menu si c'est le cas
    if(client->showMenu && client->isConnected)
    {
        dkwClipMouse(false);
        menuManager.render(client->clientRoot);
        //  if (clientRoot) clientRoot->render();

        //  renderMenu();
        dkglPushOrtho(800, 600);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glEnable(GL_BLEND);
        glColor3f(1, 1, 1);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //--- Square around the choosen gun
        glPolygonMode(GL_FRONT, GL_LINE);
        glLineWidth(2);
        glColor3f(0, 1, 0);
        glBegin(GL_QUADS);
        glVertex2i(client->currentGun->pos[0] - 5, client->currentGun->pos[1] - 5);
        glVertex2i(client->currentGun->pos[0] - 5, client->currentGun->pos[1] + client->currentGun->size[1] + 5);
        glVertex2i(client->currentGun->pos[0] + client->currentGun->size[0] + 5, client->currentGun->pos[1] + client->currentGun->size[1] + 5);
        glVertex2i(client->currentGun->pos[0] + client->currentGun->size[0] + 5, client->currentGun->pos[1] - 5);
        glEnd();
        glBegin(GL_QUADS);
        glVertex2i(client->currentMelee->pos[0] - 5, client->currentMelee->pos[1] - 5);
        glVertex2i(client->currentMelee->pos[0] - 5, client->currentMelee->pos[1] + client->currentMelee->size[1] + 5);
        glVertex2i(client->currentMelee->pos[0] + client->currentMelee->size[0] + 5, client->currentMelee->pos[1] + client->currentMelee->size[1] + 5);
        glVertex2i(client->currentMelee->pos[0] + client->currentMelee->size[0] + 5, client->currentMelee->pos[1] - 5);
        glEnd();
        glPolygonMode(GL_FRONT, GL_FILL);
        glColor3f(1, 1, 1);

        printCenterText(400, 5 + 48, 32, true, gameVar.sv_gameName);
        switch(game->gameType)
        {
        case GAME_TYPE_DM:
            printCenterText(400, 5, 64, true, clientVar.lang_deathmatchC);
            printCenterText(400, 5 + 88, 32, true, clientVar.lang_deathmatchD);
            break;
        case GAME_TYPE_TDM:
            printCenterText(400, 5, 64, true, clientVar.lang_teamDeathmatchC);
            printCenterText(400, 5 + 88, 32, true, clientVar.lang_teamDeathmatchD);
            break;
        case GAME_TYPE_CTF:
            printCenterText(400, 5, 64, true, clientVar.lang_captureTheFlagC);
            printCenterText(400, 5 + 88, 32, true, clientVar.lang_captureTheFlagD);
            break;
        }
        CString mapInfo(game->map->mapName);
        auto cmap = static_cast<ClientMap*>(game->map);
        if(cmap->author_name.len() > 0)
            mapInfo.set("%s created by %s", game->map->mapName.s, cmap->author_name.s);
        printCenterText(400, 5 + 64, 32, true, mapInfo);
        glPopAttrib();
        dkglPopOrtho();
    }
    else
    {
        if((menuManager.root) && (menuManager.root->visible))
        {
            dkwClipMouse(false);
        }
        else
        {
            dkwClipMouse(true);
        }
    }

    // Finalement, par dessus tout, le crosshair
//  CVector2i cursor = dkwGetCursorPos_main();
//  int xM = (int)(((float)cursor[0]/(float)res[0])*800.0f);
//  int yM = (int)(((float)cursor[1]/(float)res[1])*600.0f);
    dkglPushOrtho(800, 600);
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1, .2f, .2f, client->hitIndicator);
    renderTexturedQuad(xM - 16, yM - 16, 32, 32, client->tex_crossHit);
    glPopAttrib();
    dkglPopOrtho();

    // On se connecte
    if(!client->isConnected)
    {
        dkglPushOrtho(800, 600);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glEnable(GL_BLEND);
        glColor3f(1, 1, 1);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        if(client->blink > .25f) printCenterText(400, 300 - 32, 64, true, clientVar.lang_connectingC);
        printCenterText(400, 332, 48, true, clientVar.lang_pressF10ToCancel);
        if(dkiGetState(KeyF10) == DKI_DOWN) console->sendCommand("disconnect");
        glPopAttrib();
        dkglPopOrtho();
    }
}

void ClientWeapon_Render(ClientWeapon* weapon)
{
    if (weapon->weaponID == WEAPON_KNIVES)
    {
        glPushAttrib(GL_ENABLE_BIT);
            glEnable(GL_ALPHA_TEST);
            glAlphaFunc(GL_GREATER, 0.3f);
            dkoRender(weapon->dkoModel, weapon->modelAnim);
        glPopAttrib();
    }
    else if (weapon->weaponID == WEAPON_SHIELD)
    {
        dkoRender(weapon->dkoAlternative, weapon->modelAnim);
        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
            glPushMatrix();
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                dkoRender(weapon->dkoModel, weapon->modelAnim);
                glDepthMask(GL_FALSE);
                for (float i=0;i<10;++i)
                {
                //  glScalef(1.05f,1.05f,1.05f);
                    glRotatef(36,0,0,1);
                    dkoRender(weapon->dkoModel, weapon->modelAnim);
                }
            glPopMatrix();
        glPopAttrib();

        if (weapon->modelAnim < 10)
        {
            glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
                glDisable(GL_FOG);
                glDisable(GL_LIGHTING);
                glDepthMask(GL_FALSE);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glPushMatrix();
                    // Shot glow
                    glEnable(GL_TEXTURE_2D);
                    glDisable(GL_DEPTH_TEST);
                    glBindTexture(GL_TEXTURE_2D, clientVar.tex_shotGlow);
                    glColor4f(0,.9f,1,1 - (weapon->modelAnim/10.0f));
                    glBegin(GL_QUADS);
                        glTexCoord2f(0,1);
                        glVertex3f(-25,25,0);
                        glTexCoord2f(0,0);
                        glVertex3f(-25,-25,0);
                        glTexCoord2f(1,0);
                        glVertex3f(25,-25,0);
                        glTexCoord2f(1,1);
                        glVertex3f(25,25,0);
                    glEnd();
                    glEnable(GL_DEPTH_TEST);
                glPopMatrix();
            glPopAttrib();
        }
    }
    else
    {
        dkoRender(weapon->dkoModel);
        for(int i = 0; i < (int)weapon->nuzzleFlashes.size(); ++i)
        {
            NuzzleFlash_Render(weapon->nuzzleFlashes[i]);
        }
    }
}

void NuzzleFlash_Render(NuzzleFlash* nf)
{
    if (nf->delay > 0)
    {
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
            glDisable(GL_FOG);
            glDisable(GL_LIGHTING);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glPushMatrix();
                // Shot glow
                glTranslatef(nf->position[0], nf->position[1], nf->position[2]-(.5f/.005f));
                glEnable(GL_TEXTURE_2D);
                glDisable(GL_DEPTH_TEST);
                glBindTexture(GL_TEXTURE_2D, clientVar.tex_shotGlow);
                glColor4f(1,1,1,(nf->delay/NUZZLE_DELAY)*(nf->delay/NUZZLE_DELAY) * .25f);
                glBegin(GL_QUADS);
                    glTexCoord2f(0,1);
                    glVertex3f(-500,500,0);
                    glTexCoord2f(0,0);
                    glVertex3f(-500,-500,0);
                    glTexCoord2f(1,0);
                    glVertex3f(500,-500,0);
                    glTexCoord2f(1,1);
                    glVertex3f(500,500,0);
                glEnd();
                glEnable(GL_DEPTH_TEST);
            glPopMatrix();
            glPushMatrix();
                glTranslatef(nf->position[0], nf->position[1], nf->position[2]);
                glScalef((1-nf->delay/NUZZLE_DELAY)*2+.5f, (1-nf->delay/NUZZLE_DELAY)*2+.5f, (1-nf->delay/NUZZLE_DELAY)*2+.5f);
                glRotatef(nf->angle, 0, 1, 0);
                glEnable(GL_BLEND);
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, clientVar.tex_nuzzleFlash);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glDisable(GL_CULL_FACE);
                glColor4f(1,1,1,nf->delay/NUZZLE_DELAY);
                glBegin(GL_QUADS);
                    glTexCoord2f(0,1);
                    glVertex3f(-50,100,0);
                    glTexCoord2f(0,0);
                    glVertex3f(-50,0,0);
                    glTexCoord2f(1,0);
                    glVertex3f(50,0,0);
                    glTexCoord2f(1,1);
                    glVertex3f(50,100,0);

                    glTexCoord2f(0,1);
                    glVertex3f(0,100,50);
                    glTexCoord2f(0,0);
                    glVertex3f(0,0,50);
                    glTexCoord2f(1,0);
                    glVertex3f(0,0,-50);
                    glTexCoord2f(1,1);
                    glVertex3f(0,100,-50);
                glEnd();
            glPopMatrix();
        glPopAttrib();
    }
}

void renderBabo(int rect[4], float angle, uint32_t texSkin, uint32_t texShadow)
{
    //--- We render the babo in 3D
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        //  glPushAttrib(GL_ALL_ATTRIB_BITS);
            glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT | GL_VIEWPORT_BIT);

                glDepthMask(GL_TRUE);
                CVector2i   res = dkwGetResolution();
                float       offset = 0;

                glViewport(
                    (GLint)((((float)rect[0]/800.0f) * (float)res[0]) + offset),
                    res[1] - (int)(((float)(rect[1])/600.0f) * (float)res[1]) - (int)(((float)rect[3]/600.0f) * (float)res[1] + 1),
                    (int)(((float)rect[2]/800.0f) * (float)res[0]),
                    (int)(((float)rect[3]/600.0f) * (float)res[1] + 1));

                dkglSetProjection(70, 1, 1000, (float)rect[2], (float)rect[3]);

                // Truc par default à enabeler
                glEnable(GL_DEPTH_TEST);
                glEnable(GL_CULL_FACE);
                glDisable(GL_TEXTURE_2D);
                glColor3f(1,1,1);

                //--- Camera
                dkglLookAt(-4, -12, 7, 0, 0, 5, 0, 0, 1);

                //--- Shadow
                glColor4f(1,1,1,.75f);
                renderTexturedQuad(-8, 8, 16, -16, texShadow);

                glColor3f(1,1,1);

                //--- La boule
                glDisable(GL_BLEND);
                glEnable(GL_CULL_FACE);
                glEnable(GL_LIGHTING);
                glDepthFunc(GL_LEQUAL);

                dkglSetPointLight(1, -1000, -500, 2000, 1, 1, 1);
                glPushMatrix();
                    glTranslatef(0, 0, 5);
                    glRotatef(90, 0, 1, 0);
                    glRotatef(angle, 0, 0, 1);


                    glEnable(GL_COLOR_MATERIAL);
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, texSkin);

                    glColor3f(1,1,1);
                    glPolygonMode(GL_FRONT, GL_FILL);
                    dkglDrawSphere(5, 24, 24, GL_TRIANGLES);
                glPopMatrix();

            glPopAttrib();
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
