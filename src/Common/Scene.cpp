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

#include "Scene.h"
#include "GameVar.h"
#include "CMaster.h"
#include "Server.h"
#include <fstream>

extern char* bbNetVersion;

//
// Constructeur
//
Scene::Scene(dkContext* ctx)
{
    dk = ctx;
    frameID = 0;
    server = 0;
    serverInfoDelay = GAME_UPDATE_DELAY;
    masterReady = true;

    // On reset el timer
    dkcJumpToFrame(ctx, 0);
}

//
// Destructeur
//
Scene::~Scene()
{
    disconnect();
}

//
// Update
//
void Scene::update(float delay)
{
    frameID++;

    //--- Update master server client
    if (master) master->update(delay);
    {
        // On update le server, trclientVar.dkpp_s important
        if (server)
        {
            server->update(delay);
            if (server->needToShutDown)
            {
                disconnect();
                return;
            }
        }
    }
}

void Scene::dedicate(CString mapName)
{
    disconnect();
    console->add("\x3> Creating Server");
    server = new Server(new Game(mapName)); // On connect notre jeu au server

    // On start le server
    if(!server->host())
    {
        disconnect();
        console->add("\x4> Error creating server");
    }
    else
    {
        console->add(CString("\x3> Server Created on port %i", gameVar.sv_port));
    }
}

void Scene::disconnect()
{
    bool wasServer = false;
    if(server) { console->add("\x3> Shutdowning Server"); wasServer = true; }
    ZEVEN_SAFE_DELETE(server);
}

// Pour chatter
void Scene::sayall(CString message)
{
    if(server) server->sayall(message);
}


// pour kicker un FUCKING cheateux
void Scene::kick(CString playerName)
{
    if(server)
    {
        for(int i = 0; i < MAX_PLAYER; ++i)
        {
            if(server->game)
            {
                if(server->game->players[i])
                {
                    if(textColorLess(server->game->players[i]->name) == textColorLess(playerName))
                    {
                        if(master) master->RA_DisconnectedPlayer(textColorLess(playerName).s, server->game->players[i]->playerIP, (long)server->game->players[i]->playerID);

                        bb_serverDisconnectClient(server->game->players[i]->babonetID);
                        console->add(CString("\x3> Disconnecting client %s (%s), kicked by server", server->game->players[i]->name.s, server->game->players[i]->playerIP), true);
                        ZEVEN_SAFE_DELETE(server->game->players[i]);
                        net_svcl_player_disconnect playerDisconnect;
                        playerDisconnect.playerID = (char)i;
                        bb_serverSend((char*)&playerDisconnect, sizeof(net_svcl_player_disconnect), NET_SVCL_PLAYER_DISCONNECT, 0);
                        continue;
                    }
                }
            }
        }
    }
}


void Scene::kick(int ID)
{
    if(server)
    {
        if(server->game)
        {
            if(server->game->players[ID])
            {
                if(master) master->RA_DisconnectedPlayer(textColorLess(server->game->players[ID]->name).s, server->game->players[ID]->playerIP, (long)server->game->players[ID]->playerID);
                bb_serverDisconnectClient(server->game->players[ID]->babonetID);
                console->add(CString("\x3> Disconnecting client %s (%s), kicked by server", server->game->players[ID]->name.s, server->game->players[ID]->playerIP), true);
                ZEVEN_SAFE_DELETE(server->game->players[ID]);
                net_svcl_player_disconnect playerDisconnect;
                playerDisconnect.playerID = (char)ID;
                bb_serverSend((char*)&playerDisconnect, sizeof(net_svcl_player_disconnect), NET_SVCL_PLAYER_DISCONNECT, 0);
            }
        }
    }
}

void Scene::ban(CString playerName)
{
    if(server)
    {
        for(int i = 0; i < MAX_PLAYER; ++i)
        {
            if(server->game)
                if(server->game->players[i])
                {
                    if(textColorLess(server->game->players[i]->name) == textColorLess(playerName))
                    {
                        ban(server->game->players[i]->playerID);
                    }
                }
        }
    }
}

void Scene::ban(int ID)
{
    if(server)
        if(server->game)
            if(server->game->players[ID])
            {

                if(master) master->RA_DisconnectedPlayer(textColorLess(server->game->players[ID]->name).s, server->game->players[ID]->playerIP, (long)server->game->players[ID]->playerID);
                bb_serverDisconnectClient(server->game->players[ID]->babonetID);
                server->banList.push_back(std::pair<CString, CString>(server->game->players[ID]->name, server->game->players[ID]->playerIP));

                std::ofstream file("main/banlist", std::ios::app | std::ios::binary);
                CString name = server->game->players[ID]->name;
                name.resize(32);
                file.write(name.s, sizeof(char) * 32);
                file.write(server->game->players[ID]->playerIP, sizeof(char) * 16);

                console->add(CString("\x3> Disconnecting client %s (%s), banned by server", server->game->players[ID]->name.s, server->game->players[ID]->playerIP), true);
                ZEVEN_SAFE_DELETE(server->game->players[ID]);
                net_svcl_player_disconnect playerDisconnect;
                playerDisconnect.playerID = (char)ID;
                bb_serverSend((char*)&playerDisconnect, sizeof(net_svcl_player_disconnect), NET_SVCL_PLAYER_DISCONNECT, 0);
            }
}

void Scene::banIP(CString playerIP)
{
    if(server)
    {
        CString playerName("MANUAL-IP-BAN");
        server->banList.push_back(std::pair<CString, CString>(playerName, playerIP));

        std::ofstream file("main/banlist", std::ios::app | std::ios::binary);
        playerName.resize(32);
        playerIP.resize(16);
        file.write(playerName.s, sizeof(char) * 32);
        file.write(playerIP.s, sizeof(char) * 16);
    }
}

void Scene::unban(int banID)
{
    if(server)
    {
        if(server->game)
        {
            // First, remove entry
            server->banList.erase(server->banList.begin() + banID);

            // Then rewrite file
            std::ofstream file("main/banlist", std::ios::trunc | std::ios::binary);
            CString name;

            for(std::size_t i = 0; i < server->banList.size(); ++i)
            {
                // Ensure name is 32 characters
                name = server->banList[i].first;
                name.resize(32);

                // Write data
                file.write(name.s, sizeof(char) * 32);
                file.write(server->banList[i].second.s, sizeof(char) * 16);
            }
        }
    }
}
