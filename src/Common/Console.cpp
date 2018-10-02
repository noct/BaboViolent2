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

#include "Console.h"
#include <Zeven/FileIO.h>
#include "CMaster.h"
#include "Scene.h"
#include <algorithm>
#include <string>

#include "md5.h"

// Notre module principal
Console * console;

// La scene, par o nos messages consoles passent
extern Scene * scene;

//
// Constructeurs
//
Console::Console(): m_maxCmdHistorySize(20), m_maxMsgHistorySize(300), m_historyMod(3)
{
    m_outputFilename = "main/console.log";
    FileIO *fileIO = new FileIO(m_outputFilename, "wb");
    ZEVEN_SAFE_DELETE(fileIO);
    m_excludeFromLog.push_back("admin");
    m_excludeFromLog.push_back("cacheban");
    m_excludeFromLog.push_back("cacheunban");
}

Console::Console(CString outputFilename): m_maxCmdHistorySize(20), m_maxMsgHistorySize(300), m_historyMod(3)
{
    m_outputFilename = outputFilename;
    SetDisplayEvents(true);
}

//
// Destructeur
//
Console::~Console()
{
}

//
// Pour ajouter une ligne
//
void Console::add(CString message, bool fromServer, bool isEvent)
{
    // broadcast to potential remote admins
    CString messageStr = textColorLess( message );
    if( master ) master->RA_ConsoleBroadcast( messageStr.s );

    if (isEvent == true)
        m_eventMessages.push_back(message);
    else
        m_chatMessages.push_back(message);

    //--- On Check s'il y a pas un joueur admin.
    //    Si oui on lui envoit tout les messages console
    if (scene && fromServer)
    {
        if (scene->server)
        {
            for (int i=0;i<MAX_PLAYER;++i)
            {
                if (scene->server->game->players[i])
                {
                    if (scene->server->game->players[i]->isAdmin)
                    {
                        message = CString(">> ") + message;
                        bb_serverSend(message.s,message.len() + 1, NET_SVCL_CONSOLE, scene->server->game->players[i]->babonetID);
                    }
                }
            }
        }
    }


    if (gameVar.c_debug)
    {
        FileIO fileIO(m_outputFilename, "a");
        if (fileIO.isValid())
        {
            // avant de printer ? dans le fichier, il faut enlever les couleurs dans le texte
            int len = message.len();
            for (int i=0;i<len;++i)
            {
                if (message[i] >= '\x1' && message[i] <= '\x9')
                {
                    message.remove(i);
                    --i;
                    --len;
                }
            }
            fileIO.putLine(message);
        }
    }
}

void Console::output(CString filename)
{
    FileIO fileIO(filename, "w");

    if (fileIO.isValid())
    {
        for (int i=0;i<(int)m_eventMessages.size();fileIO.putLine(m_eventMessages[i++]));
    }
}

void Console::update(float delay)
{
    while ((int)m_eventMessages.size() > m_maxMsgHistorySize)//((gameVar.c_huge)?40:20))
        m_eventMessages.erase(m_eventMessages.begin()); // Thats it

    while ((int)m_chatMessages.size() > m_maxMsgHistorySize)
        m_chatMessages.erase(m_chatMessages.begin());
}

void Console::reset()
{
    m_eventMessages.clear();
    m_chatMessages.clear();
}

void Console::init()
{
    if (gameVar.c_debug) add("Console created");
}

void Console::svChange(CString command)
{
    // On donne la job ?dksvar pour ?
    CMD_RET cr = dksvarCommand(command.s);
    if (gameVar.c_debug) // We do not output that in non-debug
    {
        if (cr == CR_NOSUCHVAR)
            add(CString("\x4> Unknown variable"));
        else if (cr == CR_INVALIDARGS)
            add(CString("\x4> Invalid arguments"));
        else if (cr == CR_NOTSUPPORTED)
            add(CString("\x4> Unknown command"));
        else
            add(CString("\x3> %s", command.s));
    }
    return;
}

//
// Pour envoyer une commande ?la console
//
void Console::sendCommand(CString commandLine, bool isAdmin, unsigned long bbnetID)
{
    commandLine.trim('\n');

    CString tokenize = commandLine;

    // On va chercher le premier token, ? va nous donner la commande
    CString command = tokenize.getFirstToken(' ');

    // On fait un if sur toute les commandes possibles
    if (command == "help" || command == "?")
    {
        add("help ? info admin - set quit host dedicate voteon novote");
        add("playerlist maplist addmap removemap changemap connect");
        add("disconnect sayall sayteam edit restart kick kickid");
        add("banlist ban banid banip unban move moveid allwatch");
        return;
    }

    // File script
    if (command == "execute")
    {
        int i;
        FileIO file(CString("main/LaunchScript/%s.cfg", tokenize.s), "r");
        if (file.isValid())
        {
            // Clear previous vote settings
            sendCommand("novote");

            CString line = file.getLine();

            //--- We remove the \n at the end of the string
            line.trim('\n');

            //making sure win32 and linux have the same line feed
            line.trim('\r');

            while (line != "endscript")
            {
                //--- Put the ^color code.
                int len = line.len();
                CString newText;
                for (i=0;i<len;++i)
                {
                    if (line[i] == '^' && i < len - 1)
                    {
                        int number = line[i+1] - '0';
                        switch(number)
                        {
                        case 1:newText += "\x1";break;
                        case 2:newText += "\x2";break;
                        case 3:newText += "\x3";break;
                        case 4:newText += "\x4";break;
                        case 5:newText += "\x5";break;
                        case 6:newText += "\x6";break;
                        case 7:newText += "\x7";break;
                        case 8:newText += "\x8";break;
                        case 9:newText += "\x9";break;
                        }
                        ++i;
                    }
                    else
                    {
                        newText += CString("%c", line[i]);
                    }
                }
                line = newText;

                for (;;)
                {
                    if (strnicmp("//", line.s, 2) == 0) break;
                    if (line.s[0] == ' ' || line.s[0] == '\0' || line.s[0] == '\n') break;

                    //--- Send to consoles
                    if(line.len() > 0)
                    {
                    sendCommand(line);
                    }
                    break;
                }
                line = file.getLine();

                //--- We remove the \n at the end of the string
                line.trim('\n');

                //making sure win32 and linux have the same line feed
                line.trim('\r');
            }
        }
        return;
    }

    if (command == "info")
    {
        if(scene->server && scene->server->game)
        {
            add(CString("\x9[Server Info] Game Type: %d - Port: %d - Name: %s", gameVar.sv_gameType, gameVar.sv_port, gameVar.sv_gameName.s), true);
        }
        return;
    }

    // Le set
    if (command == "set")
    {
        command = tokenize.getFirstToken(' ');

        // only set to true when changing sv_gameTimeLimit
        bool updateTimer = (command == "sv_gameTimeLimit");

        // only set true when changing weather effects setting
        bool reloadWeather = (command == "r_weatherEffects");

        if (strnicmp(command.s, "sv_", 3) == 0)
        {
            // Ensure password is no longer than 15 characters
            CString val = CString(commandLine);
            val.getFirstToken(' ');
            CString svar = val.getFirstToken(' ');

            if(svar == "sv_password" && val.len() > 15)
            {
                val.resize(15);
                commandLine = "set sv_password ";
                commandLine += val;
                console->add(CString("Max password length is 15 characters, password changed to '%s'", val.s), true);
            }

            // Il faut envoyer le changement de variable sur le r?eau
            scene->server->sendSVChange(commandLine);

            if (command == "set sv_port")
            {
            //  bb_serverChangePort(gameVar.sv_port);
            }
        }

        // On donne la job ?dksvar pour ?
        CMD_RET cr = dksvarCommand(commandLine.s);
        if (cr == CR_NOSUCHVAR)
            add(CString("\x4> Unknown variable"));
        else if (cr == CR_INVALIDARGS)
            add(CString("\x4> Invalid arguments"));
        else if (cr == CR_NOTSUPPORTED)
            add(CString("\x4> Unknown command"));
        else
            add(CString("\x3> %s", command.s));

        // Do we need to update current game time?
        if (updateTimer)
        {
            if (scene)
            {
                if (scene->server)
                {
                    if (scene->server->game)
                    {
                        scene->server->game->gameTimeLeft = gameVar.sv_gameTimeLimit;
                    }
                }
            }
        }

        return;
    }

    // Le quit
    if (command == "quit")
    {
        // On quit le jeu de force !!
        MainLoopForceQuit();
        add(CString("\x3> Quitting application..."));
        return;
    }

    // Pour hoster une game
    if (command == "dedicate")
    {
        // If already running, only change map, since if the server recieves an
        if(scene->server && scene->server->isRunning)
             scene->server->changeMap(tokenize);
        else
            scene->dedicate(tokenize);
        return;
    }

    // Allow command to be voted on
    if (command == "voteon")
    {
        // Not case sensitive
        CString command("%s", tokenize.getFirstToken(' ').s);
        command.toLower();

        if(scene->server)
        {
            scene->server->voteList.push_back(command);
            add(CString("\x3> %s can now be voted on", command.s), true);
        }

        return;
    }

    // Remove all commands from vote list
    if (command == "novote")
    {
        if(scene->server)
        {
            // Clear list
            std::vector<CString> temp;
            scene->server->voteList.swap(temp);

            add("\x3> All commands are no longer votable", true);
        }
        return;
    }

    // Pour ajouter une map ?la queue
    if (command == "addmap")
    {
        if (scene->server) scene->server->addmap(tokenize);
        return;
    }

    // Lister les player et leur IP !
    if (command == "playerlist")
    {
        if (scene->server)
        {
            if (scene->server->game)
            {
                for (int i=0;i<MAX_PLAYER;++i)
                {
                    if (scene->server->game->players[i])
                    {
                        add(CString("[%02i] %s - IP:%s", i,
                            scene->server->game->players[i]->name.s,
                            scene->server->game->players[i]->playerIP), true);
                    }
                }
            }
        }
        return;
    }

    // Put player on a specified team
    if (command == "move")
    {
        if (scene->server)
        {
            if (scene->server->game)
            {
                int teamID = tokenize.getFirstToken(' ').toInt();
                int playerID = -1;
                if ((teamID < -1) || (teamID > 1))
                {
                    add(CString("Error: team ID must be one of the following:\n    -1 for spectator, 0 for blue or 1 for red."), true);
                }
                else
                {
                    for (int i = 0; i < MAX_PLAYER; ++i)
                    {
                        if(scene->server->game->players[i] && (textColorLess(tokenize) == textColorLess(scene->server->game->players[i]->name)))
                        {
                            playerID = i;
                            break;
                        }
                    }
                    if (playerID == -1)
                    {
                        add(CString("Error: No players were found with given name"), true);
                    }
                    else
                    {
                        scene->server->game->assignPlayerTeam(playerID, teamID, 0);
                        net_clsv_svcl_team_request teamRequest;
                        teamRequest.playerID = playerID;
                        teamRequest.teamRequested = teamID;
                        bb_serverSend((char*)&teamRequest, sizeof(net_clsv_svcl_team_request), NET_CLSV_SVCL_TEAM_REQUEST, 0);
                    }
                }
            }
        }
        return;
    }

    // Put player on a specified team
    if (command == "moveid")
    {
        if (scene->server)
        {
            if (scene->server->game)
            {
                int teamID = tokenize.getFirstToken(' ').toInt();
                int playerID = tokenize.getNextToken(' ').toInt();
                if ((teamID < -1) || (teamID > 1))
                {
                    add(CString("Error: team ID must be one of the following:\n    -1 for spectator, 0 for blue or 1 for red."), true);
                }
                else if ((playerID < -1) || (playerID >= MAX_PLAYER) || !scene->server->game->players[playerID])
                {
                    add(CString("Error: Bad player ID (use playerlist command to obtain the correct ID)"), true);
                }
                else
                {
                    if( playerID == -1 )
                    {
                        // move everyone to the selected team
                        for( int i=0; i<MAX_PLAYER; i++ )
                        {
                            if( scene->server->game->players[i] )
                            {
                                if(scene->server->game->map->flagState[0] == scene->server->game->players[i]->playerID)
                                {
                                    scene->server->game->map->flagState[0] = -1; // Le server va nous communiquer la position du flag exacte
                                    scene->server->game->map->flagPos[0] =  scene->server->game->players[i]->currentCF.position;
                                    scene->server->game->map->flagPos[0][2] = 0;
                                }
                                if(scene->server->game->map->flagState[1] == scene->server->game->players[i]->playerID)
                                {
                                    scene->server->game->map->flagState[1] = -1; // Le server va nous communiquer la position du flag exacte
                                    scene->server->game->map->flagPos[1] =  scene->server->game->players[i]->currentCF.position;
                                    scene->server->game->map->flagPos[1][2] = 0;
                                }
                                scene->server->game->players[i]->currentCF.position.set(-999,-999,0);
                                scene->server->game->assignPlayerTeam(i, teamID, 0);
                                net_clsv_svcl_team_request teamRequest;
                                teamRequest.playerID = i;
                                teamRequest.teamRequested = teamID;
                                bb_serverSend((char*)&teamRequest, sizeof(net_clsv_svcl_team_request), NET_CLSV_SVCL_TEAM_REQUEST, 0);
                            }
                        }
                    }
                    else
                    {
                        // move only the specified player
                        if( scene->server->game->players[playerID] )
                        {
                            if(scene->server->game->map->flagState[0] == scene->server->game->players[playerID]->playerID)
                            {
                                scene->server->game->map->flagState[0] = -1; // Le server va nous communiquer la position du flag exacte
                                scene->server->game->map->flagPos[0] =  scene->server->game->players[playerID]->currentCF.position;
                                scene->server->game->map->flagPos[0][2] = 0;
                            }
                            if(scene->server->game->map->flagState[1] == scene->server->game->players[playerID]->playerID)
                            {
                                scene->server->game->map->flagState[1] = -1; // Le server va nous communiquer la position du flag exacte
                                scene->server->game->map->flagPos[1] =  scene->server->game->players[playerID]->currentCF.position;
                                scene->server->game->map->flagPos[1][2] = 0;
                            }
                            scene->server->game->players[playerID]->currentCF.position.set(-999,-999,0);
                        }
                        scene->server->game->assignPlayerTeam(playerID, teamID, 0);
                        net_clsv_svcl_team_request teamRequest;
                        teamRequest.playerID = playerID;
                        teamRequest.teamRequested = teamID;
                        bb_serverSend((char*)&teamRequest, sizeof(net_clsv_svcl_team_request), NET_CLSV_SVCL_TEAM_REQUEST, 0);
                    }
                }
            }
        }
        return;
    }

    // Everyone spec!
    if (command == "allwatch")
    {
        if (scene->server)
        {
            if (scene->server->game)
            {
                for(int playerID = 0; playerID < MAX_PLAYER; ++playerID)
                {
                    if(scene->server->game->players[playerID])
                    {
                        scene->server->game->assignPlayerTeam(playerID, PLAYER_TEAM_SPECTATOR, 0);
                        net_clsv_svcl_team_request teamRequest;
                        teamRequest.playerID = playerID;
                        teamRequest.teamRequested = PLAYER_TEAM_SPECTATOR;
                        bb_serverSend((char*)&teamRequest, sizeof(net_clsv_svcl_team_request), NET_CLSV_SVCL_TEAM_REQUEST, 0);
                    }
                }
            }
        }
        return;
    }

    // approve players to join selected or all teams
    if (command == "approveall")
    {
        if (scene->server)
        {
            if (scene->server->game)
            {
                int teamid;
                CString strteamid = tokenize.getFirstToken(' ');
                if (strteamid != "")
                {
                    teamid = atoi(strteamid.s);
                    scene->server->game->approveAll(teamid);
                }
                else
                    scene->server->game->approveAll();
            }
        }
        return;
    }

    if (command == "approveplayer")
    {
        if (scene->server)
        {
            if (scene->server->game)
            {
                int id;
                CString strid = tokenize.getFirstToken(' ');
                int teamid;
                CString strteamid = tokenize.getFirstToken(' ');
                if (strid != "" && strteamid != "")
                {
                    id = atoi(strid.s);
                    teamid = atoi(strteamid.s);
                    if (scene->server->game->approvePlayer(id, teamid) == false)
                        add("\x3> Command failed", true);
                }
                else
                    add("\x3> Invalid arguments, usage: approveplayer userid team", true);
            }
        }
        return;
    }

    // removes player from the list of approved players (of all teams or selected)
    // only works if player was approved to some team before
    if (command == "rejectplayer")
    {
        if (scene->server)
        {
            if (scene->server->game)
            {
                CString strid = tokenize.getFirstToken(' ');
                CString strteamid = tokenize.getFirstToken(' ');
                if (strid != "")
                {
                    if (strteamid == "")
                        scene->server->game->rejectPlayer(atoi(strid.s));
                    else
                        scene->server->game->rejectPlayer(atoi(strid.s), (char)atoi(strteamid.s));
                }
                else
                    add("\x3> Invalid arguments, usage: rejectplayer userid [teamid]", true);
            }
        }
        return;
    }

    // rejects all players from joining a team
    if (command == "rejectallplayers")
    {
        if (scene->server)
            if (scene->server->game)
            {
                scene->server->game->rejectAllPlayers();
                sendCommand("allwatch", true);
            }
        return;
    }

    // printout of approved players
    if (command == "listapprovedplayers")
    {
        if (scene->server)
        {
            if (scene->server->game)
            {
                add("\x3> Approved players:", true);
                int teams[2] = { PLAYER_TEAM_RED, PLAYER_TEAM_BLUE };
                CString strplayer;
                CString strteam;
                for (int i = 0; i < 2; i++)
                {
                    strteam.set("\x3 Team %d", teams[i]);
                    add(strteam, true);
                    if (scene->server->game->teamApproveAll[teams[i]] == true)
                        add("\x3 - All", true);
                    else
                    {
                        const std::vector<int>& userids = scene->server->game->approvedPlayers[teams[i]];
                        for (int j = 0; j < (int)userids.size(); j++)
                        {
                            strplayer.set("\x3 - %d", userids[j]);
                            add(strplayer, true);
                        }
                    }
                }
            }
        }
        return;
    }

    // Pour voir la queue des map
    if (command == "maplist")
    {
        if (scene->server)
        {
            for (int i=0;i<(int)scene->server->mapList.size();++i)
            {
                add(scene->server->mapList[i], false);
            }

            if(scene->server->mapList.size() == 0)
                add("\x3> No maps on list", false);
        }
        return;
    }

    if (command == "maplistall")
    {
        if (scene->server)
        {
            std::vector<CString> maps;
            maps = scene->server->populateMapList(true);
            for (int i=0;i<(int)maps.size();++i)
            {
                add(maps[i], false);
            }

            if(maps.size() == 0)
                add("\x3> No maps on server", false);
        }
        return;
    }

    // Pour enlever une map du queue
    if (command == "removemap")
    {
        if (scene->server) scene->server->removemap(tokenize);
        return;
    }

    // Pour changer la map, si on est server
    if (command == "changemap")
    {
        if (scene->server) scene->server->changeMap(tokenize);
        return;
    }

    // Le quit
    if (command == "disconnect")
    {
        // On disconnect si c'est bien le cas
        scene->disconnect();
        return;
    }

    // Pour chatter
    if (command == "sayall")
    {
        scene->sayall(tokenize);
        return;
    }

    // Say to only one person (Console only)
    if (command == "sayid")
    {
        if (scene->server)
        {
            if (scene->server->game)
            {
                int playerID = tokenize.getFirstToken(' ').toInt();
                if ((0 <= playerID) && (playerID < MAX_PLAYER) && scene->server->game->players[playerID])
                {
                    net_clsv_svcl_chat chat_message;
                    chat_message.teamID = -3;// -3 == private message
                    CString message ("\x08Server: %s", tokenize.s);
                    if(message.len() > 49+80)
                        message.resize(49+80);
                    memset(chat_message.message, 0, sizeof(char) * (message.len() + 1));
                    memcpy(chat_message.message, message.s, sizeof(char) * (message.len() + 1));
                    bb_serverSend((char*)&chat_message, sizeof(net_clsv_svcl_chat), NET_CLSV_SVCL_CHAT, scene->server->game->players[playerID]->babonetID);
                }
                else
                {
                    add(CString("Error: Bad player ID"), true);
                }
            }
        }
        return;
    }

    // output to console infos about a player
    if (command == "playerinfo")
    {
        int playerId = tokenize.toInt();
        if( playerId < MAX_PLAYER )
        {
            if( scene->server->game->players[playerId] )
            {
                CString strInfo = "Player ";
                strInfo += playerId;
                strInfo += " WeaponID:";
                strInfo += (scene->server->game->players[playerId]->weapon ? scene->server->game->players[playerId]->weapon->weaponID : -1);
                strInfo += " SecondaryID:";
                strInfo += (scene->server->game->players[playerId]->meleeWeapon ? scene->server->game->players[playerId]->meleeWeapon->weaponID : -1);
                strInfo += " TeamID:";
                strInfo += scene->server->game->players[playerId]->teamID;
                strInfo += " Position:";
                strInfo += scene->server->game->players[playerId]->currentCF.position.x();
                strInfo += ",";
                strInfo += scene->server->game->players[playerId]->currentCF.position.y();
                add( strInfo );
            }
            else
            {
                add("that player id is not valid");
            }
        }
        else
        {
            add("that player id is not valid");
        }
        return;
    }

    // output to console infos about a player
    if (command == "playersinfo")
    {
        for( int i=0; i<MAX_PLAYER; i++ )
        {
            if( scene->server->game->players[i] )
            {
                CString strInfo = "Player ";
                strInfo += i;
                strInfo += " WeaponID:";
                strInfo += (scene->server->game->players[i]->weapon ? scene->server->game->players[i]->weapon->weaponID : -1);
                strInfo += " SecondaryID:";
                strInfo += (scene->server->game->players[i]->meleeWeapon ? scene->server->game->players[i]->meleeWeapon->weaponID : -1);
                strInfo += " TeamID:";
                strInfo += scene->server->game->players[i]->teamID;
                strInfo += " Position:";
                strInfo += scene->server->game->players[i]->currentCF.position.x();
                strInfo += ",";
                strInfo += scene->server->game->players[i]->currentCF.position.y();
                add( strInfo );
            }
        }
        return;
    }

    if (command == "forceplayerspawn")
    {
        CString strID = tokenize.getFirstToken(' ');
        int playerid = strID.toInt();
        if( playerid < MAX_PLAYER )
        {
            if( scene->server->game->players[playerid] )
            {

                CString strX = tokenize.getFirstToken(' ');
                CString strY = tokenize.getFirstToken(' ');
                float newX = strX.toFloat();
                float newY = strY.toFloat();

                CString newWeapon = tokenize.getFirstToken(' ');
                int newWeaponId = scene->server->game->players[playerid]->weapon ? scene->server->game->players[playerid]->weapon->weaponID : 0;
                if( newWeapon.len() > 0 )
                {
                    newWeaponId = newWeapon.toInt();
                    scene->server->game->players[playerid]->nextSpawnWeapon = newWeaponId;
                }

                CString newSecondary = tokenize.getFirstToken(' ');
                int newSecondaryId = scene->server->game->players[playerid]->meleeWeapon ? scene->server->game->players[playerid]->meleeWeapon->weaponID : 10;
                if( newSecondary.len() > 0 )
                {
                    newSecondaryId = newSecondary.toInt();
                    scene->server->game->players[playerid]->nextMeleeWeapon = newSecondaryId;
                }

                scene->server->game->players[playerid]->spawn( CVector3f( newX, newY, 0 ) );

                net_svcl_player_spawn playerSpawn;
                memcpy(playerSpawn.skin, scene->server->game->players[playerid]->skin.s, 7);
                memcpy(playerSpawn.blueDecal, scene->server->game->players[playerid]->blueDecal.s, 3);
                memcpy(playerSpawn.greenDecal, scene->server->game->players[playerid]->greenDecal.s, 3);
                memcpy(playerSpawn.redDecal, scene->server->game->players[playerid]->redDecal.s, 3);
                playerSpawn.weaponID = newWeaponId;
                playerSpawn.meleeID = newSecondaryId;
                playerSpawn.playerID = scene->server->game->players[playerid]->playerID;
                playerSpawn.position[0] = (short)(newX*10);
                playerSpawn.position[1] = (short)(newY*10);
                playerSpawn.position[2] = (short)(scene->server->game->players[playerid]->currentCF.position[2]*10);
                bb_serverSend((char*)&playerSpawn, sizeof(net_svcl_player_spawn), NET_SVCL_PLAYER_SPAWN, 0);
            }
        }
        return;
    }

    if( command == "blueTeamScore" )
    {
        int playerId = tokenize.toInt();
        if( playerId < MAX_PLAYER )
        {
            if( scene->server->game->players[playerId] )
            {
                scene->server->game->map->flagState[1] = -2;

                // On le dis au autres
                net_svcl_change_flag_state flagState;
                flagState.flagID = 1;
                flagState.newFlagState = -3;
                flagState.playerID = playerId;
                bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);
                flagState.newFlagState = -2;
                bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);

                //CString message("\x03> \x01%s \x08scores for the Blue team! ID:", scene->server->game->players[playerId]->name.s,playerId);
                //console->add(message);
                scene->server->game->players[playerId]->score++;
                scene->server->game->blueWin++;
                scene->server->game->blueScore = scene->server->game->blueWin;
            }
        }
        return;
    }

    if( command == "redTeamScore" )
    {
        int playerId = tokenize.toInt();
        if( playerId < MAX_PLAYER )
        {
            if( scene->server->game->players[playerId] )
            {
                scene->server->game->map->flagState[0] = -2;

                // On le dis au autres
                net_svcl_change_flag_state flagState;
                flagState.flagID = 0;
                flagState.newFlagState = -3;
                flagState.playerID = playerId;
                bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);
                flagState.newFlagState = -2;
                bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);

                //CString message("\x03> \x01%s \x08scores for the Red team! ID:%i", scene->server->game->players[playerId]->name.s,playerId);
                //console->add(message);
                scene->server->game->players[playerId]->score++;
                scene->server->game->redWin++;
                scene->server->game->redScore = scene->server->game->redWin;
            }
        }
        return;
    }

    if( command == "redFlagReturn" )
    {
        int playerId = tokenize.toInt();
        if( playerId < MAX_PLAYER )
        {
            if( scene->server->game->players[playerId] )
            {
                scene->server->game->map->flagState[1] = -2;

                // On le dis au autres
                net_svcl_change_flag_state flagState;
                flagState.flagID = 1;
                flagState.newFlagState = -1;
                flagState.playerID = playerId;
                bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);
                flagState.newFlagState = -2;
                bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);

                //CString message("\x03> \x01%s \x08 returned the blue flag ID:%i", scene->server->game->players[playerId]->name.s,playerId);
                //console->add(message);
                //scene->server->game->players[playerId]->score++;
                //scene->server->game->redWin++;
                //scene->server->game->redScore = scene->server->game->blueWin;
            }
        }
        return;
    }

    if( command == "blueFlagReturn" )
    {
        int playerId = tokenize.toInt();
        if( playerId < MAX_PLAYER )
        {
            if( scene->server->game->players[playerId] )
            {
                scene->server->game->map->flagState[0] = -2;

                // On le dis au autres
                net_svcl_change_flag_state flagState;
                flagState.flagID = 0;
                flagState.newFlagState = -1;
                flagState.playerID = playerId;
                bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);
                flagState.newFlagState = -2;
                bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);

                //CString message("\x03> \x01%s \x08 returned the blue flag ID:%i", scene->server->game->players[playerId]->name.s,playerId);
                //console->add(message);
                //scene->server->game->players[playerId]->score++;
                //scene->server->game->redWin++;
                //scene->server->game->redScore = scene->server->game->blueWin;
            }
        }
        return;
    }

    if( command == "mapinfos" )
    {
        if( !scene ) return;
        if( !scene->server ) return;
        if( !scene->server->game ) return;
        if( !scene->server->game->map ) return;

        // list all map infos we have
        CString str = "Map Infos, name:";
        str += scene->server->game->map->mapName;
        str += " size:";
        str += scene->server->game->map->size.x();
        str += ",";
        str += scene->server->game->map->size.y();
        str += " nbSpawn:";
        str += (int)scene->server->game->map->dm_spawns.size();

        add( str );

        // list spawn points
        for( unsigned int i=0; i<scene->server->game->map->dm_spawns.size(); i++ )
        {
            str = "Spawn ";
            str += (int)i;
            str += ":";
            str += scene->server->game->map->dm_spawns[i].x();
            str += ",";
            str += scene->server->game->map->dm_spawns[i].y();
            add( str );
        }


        return;
    }

    if( command == "listbluespawns" )
    {
        if( !scene ) return;
        if( !scene->server ) return;
        if( !scene->server->game ) return;
        if( !scene->server->game->map ) return;

        // list all map infos we have
        CString str;

        // list blue spawn points
        for( unsigned int i=0; i<scene->server->game->map->blue_spawns.size(); i++ )
        {
            str = "Blue Spawn #";
            str += (int)i;
            str += ":";
            str += scene->server->game->map->blue_spawns[i].x();
            str += ",";
            str += scene->server->game->map->blue_spawns[i].y();
            add( str );
        }
        return;
    }

    if( command == "listredspawns" )
    {
        if( !scene ) return;
        if( !scene->server ) return;
        if( !scene->server->game ) return;
        if( !scene->server->game->map ) return;

        // list all map infos we have
        CString str;

        // list blue spawn points
        for( unsigned int i=0; i<scene->server->game->map->red_spawns.size(); i++ )
        {
            str = "Red Spawn #";
            str += (int)i;
            str += ":";
            str += scene->server->game->map->red_spawns[i].x();
            str += ",";
            str += scene->server->game->map->red_spawns[i].y();
            add( str );
        }
        return;
    }

    if( command == "allplayerpos" )
    {
        if( !scene ) return;
        if( !scene->server ) return;
        if( !scene->server->game ) return;
        if( !master ) return;


        // send to remote admin all player positions (compressed to shorts)
        for( unsigned int i=0; i<MAX_PLAYER; i++ )
        {
            if( scene->server->game->players[i] )
            {
                if( scene->server->game->players[i]->teamID > -1 )
                {
                    long id = (long)i;
                    short x = (short)(scene->server->game->players[i]->currentCF.position.x() * 100);
                    short y = (short)(scene->server->game->players[i]->currentCF.position.y() * 100);
                    master->RA_PositionBroadcast( id, x, y );
                }
            }
        }
        return;
    }

    // Pour carr?ent restarter toute la patente
    if (command == "restart")
    {
        dkContext* ctx = scene->ctx;
        ZEVEN_SAFE_DELETE(scene);
        scene = new Scene(ctx);
        return;
    }

    // KICK UN CRISS DE CHEATEUX ?MARDE
    if (command == "kick")
    {
        scene->kick(tokenize);
        return;
    }

    // KICK LE CHEATER PAR PLAYER ID
    if (command == "kickid")
    {
        int playerID = tokenize.toInt();
        if (playerID >= 0 && playerID < MAX_PLAYER)
        {
            scene->kick(playerID);
        }
        return;
    }

    // List bans
    if (command == "banlist")
    {
        if (scene->server)
        {
            for (int i=0;i<(int)scene->server->banList.size();++i)
            {
                add(CString("[%02i] %s \x8- %s", i,
                    scene->server->banList[i].first.s,
                    scene->server->banList[i].second.s), true);
            }
        }
        return;
    }

    // Ban by name
    if (command == "ban")
    {
        scene->ban(tokenize);
        return;
    }

    // Ban by IP
    if (command == "banip")
    {
        scene->banIP(tokenize);
        return;
    }

    // Ban by player ID
    if (command == "banid")
    {
        int playerID = tokenize.toInt();
        if (playerID >= 0 && playerID < MAX_PLAYER)
        {
            scene->ban(playerID);
        }
        return;
    }

    // Unban
    if(command == "unban") {
        unsigned int banID = tokenize.toInt();
        if(scene->server) {
            if(banID >= 0 && banID < scene->server->banList.size())
                scene->unban(banID);
        }
        return;
    }

    // List Cached players
    if (command == "cachelist")
    {
        if (scene->server)
        {
            for (int i=0;i<50;i++)
            {
                if( scene->server->CachedPlayers[i].Valid )
                {
                    //if a parameter was entered after the list, onlyshow those who fits
                    if( tokenize != "" )
                    {
                        tokenize.toLower();
                        CString name( scene->server->CachedPlayers[i].NickName );
                        name.toLower();

                        if( strstr( name.s , tokenize.s ) )
                        {
                            add(CString("[%02i] %s \x8- %s  %s", i,
                                scene->server->CachedPlayers[i].NickName,
                                scene->server->CachedPlayers[i].IP,
                                scene->server->CachedPlayers[i].macAddr),
                                true);
                        }
                        else if( strstr( scene->server->CachedPlayers[i].IP , tokenize.s ) )
                        {
                            add(CString("[%02i] %s \x8- %s  %s", i,
                                scene->server->CachedPlayers[i].NickName,
                                scene->server->CachedPlayers[i].IP,
                                scene->server->CachedPlayers[i].macAddr),
                                true);
                        }
                    }
                    else
                    {
                        add(CString("[%02i] %s \x8- %s  %s", i,
                            scene->server->CachedPlayers[i].NickName,
                            scene->server->CachedPlayers[i].IP,
                            scene->server->CachedPlayers[i].macAddr),
                            true);
                    }
                }
            }
        }
        return;
    }

    // ban a cached player
    // parameter are : password Id DurationInDays( 0 = unlimited ban )
    if (command == "cacheban")
    {
        if (scene->server)
        {
            CString strPass = tokenize.getFirstToken(' ');
//          if( strPass != "roxbabo" )
//          {
//              return;
//          }

            CString strID = tokenize.getFirstToken(' ');
            if( strID == "" )
            {
                return;
            }

            int ID = strID.toInt();
            if( ID >= 50 || ID < 0 || !scene->server->CachedPlayers[ID].Valid )
            {
                return;
            }

            CString strDuration = tokenize.getFirstToken(' ');
            int duration = 0;
            if( strDuration != "" )
            {
                duration = strDuration.toInt();
                if( duration < 0 ) duration = 1;
            }

            stCacheBan cb;
            cb.Duration     =   duration;
            cb.ID           =   ID;
            sprintf( cb.IP , "%s", scene->server->CachedPlayers[ID].IP );
            sprintf( cb.MAC , "%s", scene->server->CachedPlayers[ID].macAddr );
            sprintf( cb.Nick , "%s", scene->server->CachedPlayers[ID].NickName );
            memcpy( cb.Pass, strPass.s, 7 );
            cb.Pass[7] = '\0';

            // we have everything, tell the master server to ban him!
            master->sendPacket( (char*)&cb , sizeof(stCacheBan) , CACHE_BAN );

        }

        return;
    }

    // see who was cachebanned by the master server
    // parameter is a filter if we want to only list those containing some characters in the nick name
    if (command == "cachebanned")
    {
        if (scene->server)
        {
            CString strFilter = tokenize.getFirstToken(' ');
            stCacheList cl;
            sprintf( cl.Filter , "%s", strFilter.s );

            // we have everything, tell the master server to send us the cachebanned
            master->sendPacket( (char*)&cl , sizeof(stCacheList) , CACHE_BAN_LIST );

        }

        return;
    }


    // unban someone from master server
    // parameter is the pass + ID of the guy
    if (command == "cacheunban")
    {
        if (scene->server)
        {
            CString strPass = tokenize.getFirstToken(' ');
            if( strPass == "" )
            {
                return;
            }

            CString strID = tokenize.getFirstToken(' ');
            if( strID == "" )
            {
                return;
            }
            short ID = strID.toInt();

            stCacheUnban cu;
            cu.ID   =   ID;
            memcpy( cu.Pass, strPass.s, 7 );
            cu.Pass[7] = '\0';

            // we have everything, tell the master server to unban this guy
            master->sendPacket( (char*)&cu , sizeof(stCacheUnban) , CACHE_UNBAN );

        }

        return;
    }

    // tell master to send us cache list of remote server (no need to have admin on that server)
    if (command == "cachelistremote")
    {
        if (scene->server && bbnetID != -1)
        {
            CString serverIP = tokenize.getFirstToken(' ');
            CString serverPort = tokenize.getFirstToken(' ');
            CString strFilter = tokenize.getFirstToken(' ');
            add(CString("req from %i ", bbnetID));
            master->requestRemoteCacheList(strFilter, serverIP, serverPort, bbnetID);
            return;
        }

        //return;
        // "Unkown command" ;)
    }

    // ban remotely cached player (id from the result of last cachelistremote call)
    // parameter are : password Id DurationInDays( 0 = unlimited ban )
    if (command == "cachebanremote")
    {
        if (scene->server)
        {
            CString strPass = tokenize.getFirstToken(' ');
            if( strPass == "" ) return;

            CString strID = tokenize.getFirstToken(' ');
            if( strID == "" ) return;
            int ID = strID.toInt();
            if( ID >= 50 || ID < 0 || !master->CachedPlayersRemote[ID].Valid )
            {
                add("\x3> Invalid ID of player");
                return;
            }

            CString strDuration = tokenize.getFirstToken(' ');
            int duration = 0;
            if( strDuration != "" )
            {
                duration = strDuration.toInt();
                if( duration < 0 ) duration = 1;
            }

            stCacheBan cb;
            cb.Duration     =   duration;
            cb.ID           =   ID;
            sprintf( cb.IP , "%s", master->CachedPlayersRemote[ID].IP );
            sprintf( cb.MAC , "%s", master->CachedPlayersRemote[ID].macAddr );
            sprintf( cb.Nick , "%s", master->CachedPlayersRemote[ID].NickName );
            memcpy( cb.Pass, strPass.s, 7 );
            cb.Pass[7] = '\0';

            // we have everything, tell the master server to ban him!
            master->sendPacket( (char*)&cb , sizeof(stCacheBan) , CACHE_BAN );
        }

        //return;
        // "Unkown command" ;)
    }

    // getinvalidchecksums [offsetFromEnd=50 [number=50]]
    // request number(max 50) of entries from BadChecksums starting from number of entries-offsetFromEnd
    if (command == "getinvalidchecksums")
    {
        if (scene && scene->server)
        {
            int num = -1, offsetFromEnd = 50;
            CString offsetFromEndStr = tokenize.getFirstToken(' ');
            if (offsetFromEndStr != "")
                offsetFromEnd = offsetFromEndStr.toInt();
            CString numStr = tokenize.getFirstToken(' ');
            if (numStr != "")
                num = numStr.toInt();
            //scene->server->sendInvalidChecksums(bbnetID, num, offsetFromEnd);
            std::vector<invalidChecksumEntity> list = scene->server->getInvalidChecksums(bbnetID, num, offsetFromEnd);
            for (int i = 0; i < (int)list.size(); i++)
            {
                net_svcl_bad_checksum_entity bce;
                memset(&bce, 0, sizeof(net_svcl_bad_checksum_entity));
                strcpy(bce.name, list[i].name);
                strcpy(bce.playerIP, list[i].playerIP);
                bce.id = list[i].id;
                bb_serverSend((char*)&bce, sizeof(net_svcl_bad_checksum_entity), NET_SVCL_BAD_CHECKSUM_ENTITY, bbnetID);
            }
        }
        return;
    }

    if (command == "deleteinvalidchecksums")
    {
        if (scene && scene->server)
            scene->server->deleteInvalidChecksums();
        return;
    }

    if (command == "invalidchecksumsinfo")
    {
        if (scene && scene->server)
        {
            net_svcl_bad_checksum_info bci;
            bci.number = scene->server->getNumberOfInvalidChecksums();
            bb_serverSend((char*)&bci, sizeof(net_svcl_bad_checksum_info), NET_SVCL_BAD_CHECKSUM_INFO, bbnetID);
        }
        return;
    }

    add(CString("\x3> Unkown command : \"%s\"", command.s));
    add(CString("\x3> Type \"?\" for commands list", command.s));
}

void Console::SetDisplayEvents(bool b)
{
    if (displayEvents != b)
    {
        displayEvents = b;
        m_visibleMsgOffset = 0;
    }
}

const std::vector<CString>& Console::GetActiveMessages()
{
    if (displayEvents == true)
        return m_eventMessages;
    else
        return m_chatMessages;
}

