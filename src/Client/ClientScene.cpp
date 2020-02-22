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
#include "SceneRender.h"
#include "ClientConsole.h"

ClientScene::ClientScene(dkContext* ctx) : Scene(ctx)
{
    clientVar.loadModels();
    mainTab = 0;
    font = dkfCreateFont("main/fonts/babo.png");
    tex_crosshair = dktCreateTextureFromFile("main/textures/Cross01.png", DKT_FILTER_LINEAR);
    tex_menuCursor = dktCreateTextureFromFile("main/textures/MenuCursor.png", DKT_FILTER_LINEAR);
    //tex_miniHeadGames = dktCreateTextureFromFile("main/textures/miniHeadGames.png", DKT_FILTER_LINEAR);

    client = 0;

    renderLoadingScreen(font);

    FSOUND_SetSFXMasterVolume((int)(255.0f*gameVar.s_masterVolume));

    introDelay = 3;
    tex_rndLogo = dktCreateTextureFromFile("main/textures/RnDLabs.png", DKT_FILTER_LINEAR);
    tex_hgLogo = dktCreateTextureFromFile("main/textures/HeadGames.png", DKT_FILTER_LINEAR);
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
    dktDeleteTexture(&tex_rndLogo);
    dktDeleteTexture(&tex_hgLogo);
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

    if(introDelay > 0)
    {
        introDelay -= delay;
        if(introDelay <= 0)
        {
            if(gameVar.s_inGameMusic) dksPlayMusic("main/sounds/Menu.ogg", -1);
            createMenu();
            menuManager.root->visible = true;
        }
    }
    else
    {
        if(mainTab) mainTab->update(delay);

        // On update le server, três important
        if(server)
        {
            server->update(delay);
            if(server->needToShutDown)
            {
                disconnect();
                return;
            }
        }

        // On update le client, três important aussi
        if(client)
        {
            // On set le volume avec êa :D:D trop hot
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
// Pour créer r, join, disconnecter d'une game
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

    // Voilê maintenant on le join
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

    // Voilê maintenant on le join
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

void ClientScene::disconnect()
{

    if(client && !server) dksvarLoadConfigSVOnly(dk, "main/bv2.cfg");

    bool wrongVersion = false;
    if(client) wrongVersion = client->wrongVersionReason;
    console->add("\x3> Disconnecting from current server");
    if(client) console->add("\x3> Shutdowning Client");
    ZEVEN_SAFE_DELETE(client);

    bool wasServer = false;
    if(server) { console->add("\x3> Shutdowning Server"); wasServer = true; }
    ZEVEN_SAFE_DELETE(server);

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
    menuManager.root->superPanel = true;
    menuManager.root->texture = dktCreateTextureFromFile("main/textures/Smoke2.png", DKT_FILTER_NEAREST);

    //--- Create the main tab container
    mainTab = new CMainTab(menuManager.root);
}
