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

#ifndef SCENE_H
#define SCENE_H

#include <Zeven/Core.h>
//#include "CMaster.h"

#define ICONE_DEDICATED         0x0001
#define ICONE_PASSWORDED        0x0002
#define ICONE_FRIENDLYFIRE      0x0004
#define ICONE_3DVIEW            0x0008

class Server;

// Implemented in main.cpp
void MainLoopForceQuit();

class Scene
{
public:
    // La font
    unsigned int font;

    // Le server
    Server * server;

    // Le delait pour sender les server info (à chaque 20sec)
    float serverInfoDelay;
    bool masterReady;

    long frameID;

public:
    dkContext* ctx;

    // Constructeur
    Scene(dkContext* ctx);

    // Destructeur
    virtual ~Scene();

    // Update
    virtual void update(float delay);

    // Pour créer, join, disconnecter d'une game
    virtual void dedicate(CString mapName);
    virtual void disconnect();
    void kick(CString playerName);
    void kick(int ID);
    void ban(CString playerName);
    void ban(int ID);
    void banIP(CString playerIP);
    void unban(int banID);

    // Pour chatter
    virtual void sayall(CString message);
};

#endif

