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
#include "ClientScene.h"
#include "ClientHelper.h"
#include "ClientConsole.h"
#include <glad/glad.h>

ClientScene::ClientScene(dkContext* dk) : Scene(dk)
{
    clientVar.loadModels();
    mainTab = 0;
    font = dkfCreateFont("main/fonts/babo.tga");
    tex_crosshair = dktCreateTextureFromFile("main/textures/Cross01.tga", DKT_FILTER_LINEAR);
    tex_menuCursor = dktCreateTextureFromFile("main/textures/MenuCursor.tga", DKT_FILTER_LINEAR);
    //tex_miniHeadGames = dktCreateTextureFromFile("main/textures/miniHeadGames.tga", DKT_FILTER_LINEAR);

    client = 0;
    editor = 0;

    //-- On print le loading screen! (new)
        // On clear les buffers, on init la camera, etc
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CVector2i res = dkwGetResolution();

    glViewport(0, 0, res[0], res[1]);
    dkglSetProjection(60, 1, 50, (float)res[1] * 1.333f, (float)res[1]);

    // Truc par default � enabeler
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
    printCenterText(400, 268, 64, CString("LOADING"));

    dkglPopOrtho();

    // On swap les buffers
    dkwSwap();

    introScreen = new IntroScreen();

    FSOUND_SetSFXMasterVolume((int)(255.0f*gameVar.s_masterVolume));
    //  dksPlayMusic("main/sounds/menu.ogg", -1);
}

//
// Destructeur
//
ClientScene::~ClientScene()
{
    clientVar.deleteModels();
    dkfDeleteFont(&font);
    dktDeleteTexture(&tex_crosshair);
    dktDeleteTexture(&tex_menuCursor);
    ZEVEN_SAFE_DELETE(introScreen);
    ZEVEN_SAFE_DELETE(mainTab);
}

void ClientScene::update(float delay)
{
    if(!gameVar.s_inGameMusic)
    {
        dksStopMusic();
    }
    frameID++;

    //--- Update master server client
    if(master) master->update(delay);
    if(mainTab)
    {
        if(master->isConnected())
        {
            mainTab->browser->btn_refresh->enable = false;
        }
        else
        {
            mainTab->browser->btn_refresh->enable = true;
        }
    }
    if(introScreen)
    {
        introScreen->update(delay);
        if(introScreen->showDelay <= 0)
        {
            ZEVEN_SAFE_DELETE(introScreen);
            if(gameVar.s_inGameMusic) dksPlayMusic("main/sounds/Menu.ogg", -1);
            createMenu();
            menuManager.root->visible = true;
        }
    }
    else
    {
        if(mainTab) mainTab->update(delay);

        // On update le server, tr�s important
        if(server)
        {
            server->update(delay);
            if(server->needToShutDown)
            {
                disconnect();
                return;
            }
        }

        // On update le client, tr�s important aussi
        if(client)
        {
            // On set le volume avec �a :D:D trop hot
            if(client->game)
            {
                FSOUND_SetSFXMasterVolume((int)((255.0f - client->game->viewShake*100.0f)*gameVar.s_masterVolume));
                //  FSOUND_SetFrequency(FSOUND_ALL, gameVar.s_mixRate+(int)(-client->game->viewShake*(float)gameVar.s_mixRate*.25f));
            }
            else
            {
                FSOUND_SetSFXMasterVolume((int)(255.0f*gameVar.s_masterVolume));
                //  FSOUND_SetFrequency(FSOUND_ALL, gameVar.s_mixRate);
            }

            client->update(delay);
            if(client->needToShutDown)
            {
                disconnect();
                return;
            }
        }
        else
        {
            FSOUND_SetSFXMasterVolume((int)(255.0f*gameVar.s_masterVolume));
            //  FSOUND_SetFrequency(FSOUND_ALL, gameVar.s_mixRate);
        }

        // On update l'editor
        if(editor)
        {
            editor->update(delay);
            if(editor->needToShutDown)
            {
                disconnect();
                return;
            }
        }

        // On update les menu
        if((menuManager.root))
        {
            if(menuManager.root->visible)
            {
                menuManager.update(delay);
            }
        }
        menuManager.updateDialogs(delay);
    }

    clientVar.ro_nbParticle = dkpUpdate(delay);
}

//
// Renderer
//
void ClientScene::render()
{
    // On clear les buffers, on init la camera, etc
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CVector2i res = dkwGetResolution();

    glViewport(0, 0, res[0], res[1]);
    dkglSetProjection(60, 1, 50, (float)res[1] * 1.333f, (float)res[1]);

    // Truc par default � enabeler
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1, 1, 1);

    if(introScreen)
    {
        introScreen->render();
    }
    else
    {
        // On render le client
        float alphaScope = 0;
        if(client) client->render(alphaScope);

        // On render l'editor
        if(editor) editor->render();

        // On render les menus
        if((menuManager.root) && (menuManager.root->visible))
        {
            menuManager.render();
            dkwClipMouse(false);
        }
        menuManager.renderDialogs();

        /*      if (menu->isShowing) menu->render();
                else if (menu->soundPlayChannel >= 0)
                {
                    FSOUND_StopSound(menu->soundPlayChannel);
                    menu->soundPlayChannel = -1;
                }*/

                // On afficher le fps
        dkglPushOrtho((float)res[0], (float)res[1]);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        dkfBindFont(font);

        glColor3f(1, 1, 1);
        if(gameVar.r_showStats)
        {
            printRightText((float)res[0], 0, 20, CString("FPS : %i", (int)dkcGetFPS(ctx)));
            //dkfPrint(20,0, 0, 0, CString("FPS : %i", (int)dkcGetFPS()).s);
           //(size,x-width,y,0,text.s);
                //printRightText((float)res[0], 32, 32, CString("NB PARTICLE : %i", gameVar.ro_nbParticle));
            //  unsigned long byteSent = (client)?bb_clientGetBytesSent(client->uniqueClientID) + bb_serverGetBytesSent() + bb_peerGetBytesSent():0;
            //  unsigned long byteRecv = (client)?bb_clientGetBytesReceived(client->uniqueClientID) + bb_serverGetBytesReceived() + bb_peerGetBytesReceived():0;
            //  printRightText((float)res[0], 64, 32, CString("NET BYTE SENT : %i Kb", (int)byteSent / 1024));
            //  printRightText((float)res[0], 96, 32, CString("NET BYTE RECV : %i Kb", (int)byteRecv / 1024));
        }

        // On affiche la version du jeu
        if(!editor && !client)
        {
            if(server)
            {
                printRightText((float)res[0] - 5, (float)res[1] - 32 - 5, 32, CString(clientVar.lang_serverVersion.s, (int)GAME_VERSION_SV / 10000, (int)(GAME_VERSION_SV % 10000) / 100, ((int)GAME_VERSION_SV % 100)));
            }
            else
            {
                printRightText((float)res[0] - 5, (float)res[1] - 32 - 5, 32, CString(clientVar.lang_clientVersion.s, (int)GAME_VERSION_CL / 10000, (int)(GAME_VERSION_CL % 10000) / 100, ((int)GAME_VERSION_CL % 100)));
            }

            //--- Copyrights (replaced by head games logo)
        //  printRightText((float)res[0]-5, (float)res[1]-100-5, 16, CString("Copyright (c) RndLabs Inc. 2006-2007"));
        //  printRightText((float)res[0]-5, (float)res[1]-100-5+16, 16, CString("All Rights Reserved"));

        //  printRightText((float)res[0]-5, (float)res[1]-32-5-32, 32, CString("server version %01i.%02i.%02i", (int)GAME_VERSION_SV/10000, (int)(GAME_VERSION_SV%1000)/100, ((int)GAME_VERSION_SV%100)));
        //  printRightText((float)res[0]-5, (float)res[1]-32-5-64, 32, CString("Babonet version %s", bbNetVersion));
        }
        glPopAttrib();
        dkglPopOrtho();

        // Render la console sur toute
        auto cconsole = static_cast<ClientConsole*>(console);
        cconsole->render();

        // Non, le curseur sur TOUUTEEE
        CVector2i cursor = dkwGetCursorPos_main();
        int xM = (int)(((float)cursor[0] / (float)res[0])*800.0f);
        int yM = (int)(((float)cursor[1] / (float)res[1])*600.0f);
        dkglPushOrtho(800, 600);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if(menuManager.root)
            if(!menuManager.root->visible && client)
            {
                if(client->showMenu)
                {
                    glColor3f(1, 1, 1);
                    //  glColor3f(1,1,.6f);
                    renderTexturedQuad(xM, yM, 32, 32, tex_menuCursor);
                }
                else
                {
                    glColor4f(0, 0, 0, 1 - alphaScope);
                    renderTexturedQuad(xM - 15, yM - 15, 32, 32, tex_crosshair);
                    //  glColor4f(1,1,.6f,1-alphaScope);
                    glColor4f(1, 1, 1, 1 - alphaScope);
                    renderTexturedQuad(xM - 16, yM - 16, 32, 32, tex_crosshair);
                }
            }
            else
            {
                glColor3f(1, 1, 1);
                //  glColor3f(1,1,.6f);
                renderTexturedQuad(xM, yM, 32, 32, tex_menuCursor);
            }
        glPopAttrib();
        dkglPopOrtho();
    }
}

//
// Pour cr�r, join, disconnecter d'une game
//
void ClientScene::host(CString mapName)
{
    disconnect();
    console->add("\x3> Creating Server");
    server = new Server(new Game(mapName)); // On connect notre jeu au server
    console->add("\x3> Creating Client");
    client = new Client(new ClientGame()); // On connect notre jeu au client
    client->isServer = true;

    // On start le server
    if(!server->host())
    {
        disconnect();
        console->add("\x4> Error creating server");
        //  menu->show();
        menuManager.root->visible = true;
        return;
    }
    else
    {
        console->add(CString("\x3> Server Created on port %i", gameVar.sv_port));
    }

    // Voil� maintenant on le join
    if(!client->join("127.0.0.1", gameVar.sv_port))
    {
        disconnect();
        console->add("\x4> Error connecting to server. Verify the IP adress");
        //  menu->show();
        menuManager.root->visible = true;
        return;
    }
    else
    {
        //  console->add("\x3> Connection successfull");
        //  menu->hide();
        menuManager.root->visible = false;
    }
}

void ClientScene::dedicate(CString mapName)
{
    disconnect();
    console->add("\x3> Creating Server");
    server = new Server(new Game(mapName)); // On connect notre jeu au server

    // On start le server
    if(!server->host())
    {
        disconnect();
        console->add("\x4> Error creating server");
        menuManager.root->visible = true;
    }
    else
    {
        console->add(CString("\x3> Server Created on port %i", gameVar.sv_port));
        menuManager.root->visible = false;
    }
}

void ClientScene::join(CString IPAddress, int port, CString password/*, CString adminRequest*/)
{
    disconnect();
    console->add("\x3> Creating Client");
    client = new Client(new ClientGame()); // On connect notre jeu au client

/*  if (!adminRequest.isNull())
    {
        client->adminUserPassRequest = adminRequest;
        client->requestedAdmin = true;
    }*/

    // Voil� maintenant on le join
    if(!client->join(IPAddress, port, password))
    {
        disconnect();
        console->add("\x4> Error connecting to server. Verify the IP address");
        //  menu->show();
        menuManager.root->visible = true;
    }
    else
    {
        //  console->add("\x3> Connection successfull");
        //  menu->hide();
        menuManager.root->visible = false;
    }
}

void ClientScene::edit(CString command)
{
    CString mapName = command.getFirstToken(' ');
    int w, h;
    CString token = command.getFirstToken(' ');
    if(token.isNull()) w = 0;
    else w = token.toInt();
    token = command.getFirstToken(' ');
    if(token.isNull()) h = 0;
    else h = token.toInt();
    disconnect();
    console->add("\x3> Creating Editor");
    //  ZEVEN_SAFE_DELETE(menuManager.root);
    editor = new Editor(mapName, font, w, h); // On connect notre jeu au client
//  menu->hide();
    menuManager.root->visible = false;
}

void ClientScene::disconnect()
{

    if(client && !server) dksvarLoadConfigSVOnly("main/bv2.cfg");

    bool wrongVersion = false;
    if(client) wrongVersion = client->wrongVersionReason;
    console->add("\x3> Disconnecting from current server");
    if(client) console->add("\x3> Shutdowning Client");
    ZEVEN_SAFE_DELETE(client);

    bool wasServer = false;
    if(server) { console->add("\x3> Shutdowning Server"); wasServer = true; }
    ZEVEN_SAFE_DELETE(server);

    if(editor) console->add("\x3> Shutdowning Editor");
    ZEVEN_SAFE_DELETE(editor);
    auto cconsole = static_cast<ClientConsole*>(console);
    cconsole->unlock(); // Petit bug quand on chattait ;)
//  menu->show();
    if(menuManager.root) menuManager.root->visible = true;
    //  if (wrongVersion) menu->currentMenu = MENU_WRONG_VERSION;

}

void ClientScene::sayteam(CString message)
{
    if(client) client->sayteam(message);
}

void ClientScene::sayall(CString message)
{
    if(client)
        client->sayall(message);
    else
        if(server) server->sayall(message);
}

// Creating menu
void ClientScene::createMenu()
{
    //--- Overall control
    menuManager.root = new CControl();
    menuManager.root->font = font;
    menuManager.root->size.set(800, 600);
    menuManager.root->backColor.set(.3f, .5f, .8f);
    menuManager.root->foreColor.set(1, 1, 1);
    menuManager.root->textShadow = true;
    menuManager.root->noFill = true;
    if(gameVar.r_animatedMenu)
    {
        menuManager.root->superPanel = true;
        menuManager.root->texture = dktCreateTextureFromFile("main/textures/Smoke2.tga", DKT_FILTER_NEAREST);
    }

    //--- Create the main tab container
    mainTab = new CMainTab(menuManager.root);
}
