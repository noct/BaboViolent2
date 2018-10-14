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

#ifndef CLIENT_SCENE_H
#define CLIENT_SCENE_H

#include "Scene.h"
#include "Client.h"
#include "CMainTab.h"
#include "CMenuManager.h"
#include "IntroScreen.h"

struct ClientScene : public Scene
{
    // Le client
    Client * client;

    // Le menu
//  Menu * menu;
    CMainTab * mainTab;

    // Notre intro screen (quand que lui n'est pas fini, on update ni affiche rien d'autre)
    IntroScreen * introScreen;

    // Notre curseur
    unsigned int tex_crosshair;
    unsigned int tex_menuCursor;

    ClientScene(dkContext* ctx);

    // Destructeur
    virtual ~ClientScene();

    void update(float delay);

    // Creating menu
    void createMenu();

    void host(CString mapName);
    void dedicate(CString mapName);
    void join(CString IPAddress, int port, CString password = "");
    void sayteam(CString message);
    void disconnect();
    void sayall(CString message);
};

#endif

