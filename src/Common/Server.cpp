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

#include "Server.h"
#include "Console.h"
#include "netPacket.h"
#include "RemoteAdminPackets.h"
#include "CCurl.h"
#include <time.h>
#include <fstream>
#include <algorithm>
#include "Scene.h"

#include "md5.h"
#ifdef WIN32
#include <direct.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif


extern Scene* scene;

//
// Constructeur
//
Server::Server(Game * pGame) : maxTimeOverMaxPing(5.0f)//, maxIdleTime(180.0f)
{
    game = pGame;
    game->isServerGame = true;
    needToShutDown = false;
    pingDelay = 0;
    changeMapDelay = 0;
    frameID = 0;
    autoBalanceTimer = 0;
    infoSendDelay = 15;

    // reset cached users
    CachedIndex = 0; // what index are we going to use for next client
    for(int i = 0; i < 50; i++)
    {
        CachedPlayers[i].Valid = false;
        CachedPlayers[i].macAddr[0] = '\0';
        CachedPlayers[i].NickName[0] = '\0';
        CachedPlayers[i].IP[0] = '\0';
    }

    // Load banlist
    std::ifstream file("main/banlist", std::ios::binary);
    char name[32], ip[16];

    while(file.is_open() && !file.eof())
    {
        file.read(name, sizeof(char) * 32);
        file.read(ip, sizeof(char) * 16);

        // Will not hit eof until after first read
        if(file.eof()) break;

        // Add to ban list
        banList.push_back(std::pair<CString, CString>(name, ip));

    }
    //reportUploadURLs.push_back("http://localhost/index.php");
}



#include "CMaster.h"
//
// Destructeur
//
Server::~Server()
{
    for(unsigned int i = 0; i < m_checksumQueries.size(); i++)
    {
        delete m_checksumQueries[i];
    }
    m_checksumQueries.clear();

    if(master) master->RunningServer = 0;

    mapList.clear();
    banList.clear();

    bb_serverShutdown();
    ZEVEN_SAFE_DELETE(game);

    stKillServ killServ;
    killServ.Port = gameVar.sv_port;
    if(master) master->sendPacket((char*)(&killServ), sizeof(stKillServ), KILL_SERV);
}



//
// Le voting validity
//
bool Server::validateVote(CString vote)
{
    CString com = vote;
    CString command = com.getFirstToken(' ');
    CString var = com.getFirstToken(' ');

    // If setting a var, remove the 'set' since it is not included
    // in the list.
    if(strnicmp("set sv_", vote.s, 7) == 0)
        command = var;

    // Not case sensitive
    command.toLower();

    // Compare against allowed commands
    for(std::vector<CString>::iterator i = voteList.begin(); i != voteList.end(); ++i)
        if(*i == command)
            return true;

    return false;
}



//
// Pour starter le server
//
int Server::host()
{
    //permet de spawner un serveur
    if(bb_serverCreate(false, MAX_PLAYER, gameVar.sv_port))
    {
        isRunning = false;
        return 0;
    }
    else
    {
        srand((unsigned int)time(0));
        game->mapSeed = rand() % 1000000; // Quin, 1000000 maps, c tu assez clientVar.dkpp_?
        game->createMap();
        nextMap = game->mapName;
        mapList.push_back(game->mapName);
        if(!game->map)
        {
            console->add("\x4> Map not loaded", true);
            needToShutDown = true;
            isRunning = false;
            return 0;
        }
        if(!IsMapValid(*game->map, game->gameType))
        {
            console->add("\x4> Map is missing some entities, server can not be started", true);
            needToShutDown = true;
            isRunning = false;
            return 0;
        }
        isRunning = true;

        // La game roule!
        // On ouvre notre port UDP
        if(bb_peerBindPort(gameVar.sv_port) == 1)
        {
            console->add(CString("\x4> Error binding port on %i.", gameVar.sv_port/* + 1*/), true);
        }

        //"register" the server with our master server ( for udp broadcast ... kind of an hack )
        master->RunningServer = this;

        return 1;
    }
}



//
// Pour changer la map
//
void Server::changeMap(CString & mapName)
{
    if(isRunning)
    {
        if(game)
        {
            // if map name is not given - get the next map from the servers map list
            mapName.trim(' ');
            if(mapName == "")
            {
                mapName = queryNextMap();
            }
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
            clearStatsCache();
            nextMap = mapName;
            changeMapDelay = 10;
            net_svcl_round_state roundState;
            game->roundState = GAME_MAP_CHANGE;
            roundState.newState = game->roundState;
            roundState.reInit = false;
            bb_serverSend((char*)&roundState, sizeof(net_svcl_round_state), NET_SVCL_GAME_STATE, 0);
        }
    }
}

void Server::addmap(CString & mapName)
{
    //--- Check first is that map exist.
    CString filename("main/maps/%s.bvm", mapName.s);
    FILE* fic = fopen(filename.s, "rb");
    if(!fic)
    {
        console->add(CString("\x9> Warning, map not found %s", mapName.s), true);
        return;
    }
    else
    {
        fclose(fic);
    }

    // Un map ne peut pas clientVar.dkpp_re lclientVar.dkpp_2 fois (c poche mais c hot)
    for(int i = 0; i < (int)mapList.size(); ++i)
    {
        if(mapList[i] == mapName) return;
    }
    mapList.push_back(mapName);

    console->add(CString("\x9> %s added", mapName.s), true);
}

void Server::removemap(CString & mapName)
{
    if(mapName == game->mapName) return;
    for(int i = 0; i < (int)mapList.size(); ++i)
    {
        if(mapList[i] == mapName)
        {
            console->add(CString("\x9> %s removed", mapName.s), true);
            mapList.erase(mapList.begin() + i);
            for(int i = 0; i < (int)mapInfoList.size(); ++i)
            {//removes from the info list
                if(mapInfoList[i].mapName == mapName)
                {
                    mapInfoList.erase(mapInfoList.begin() + i);
                    return;
                }
            }
            return;
        }
    }
}

std::vector<CString> Server::populateMapList(bool all)
{
    std::vector<CString> maps;
    if(all == false)
        maps = mapList;
    else
    {
#ifdef WIN32
        WIN32_FIND_DATA FindFileData;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        char DirSpec[MAX_PATH]; // directory specification
        DWORD dwError;
        char appPath[_MAX_PATH];

        // Chercher le path du "current working directory".
        _getcwd(appPath, _MAX_PATH);

        strncpy(DirSpec, appPath, strlen(appPath) + 1);
        strncat(DirSpec, "\\main\\maps\\*.bvm", strlen("\\main\\maps\\*.bvm") + 1);

        hFind = FindFirstFile(DirSpec, &FindFileData);

        if(hFind == INVALID_HANDLE_VALUE)
        {
            // Si on ne trouve pas le rA©pertoire dA©sirA©.
        }
        else
        {
            CString filename = CString(FindFileData.cFileName);
            filename.resize(filename.len() - 4);
            maps.push_back(filename);

            while(FindNextFile(hFind, &FindFileData) != 0)
            {
                if(!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    CString filename = CString(FindFileData.cFileName);
                    filename.resize(filename.len() - 4);
                    maps.push_back(filename);
                }
            }

            dwError = GetLastError();
            FindClose(hFind);
            if(dwError != ERROR_NO_MORE_FILES)
            {
                // Si il y a une error en dA©tectant qu'il n'y a plus de fichiers.
            }
        }
#else
        dirent* file;
        DIR* hFile;
        char dirspec[256];
        getcwd(dirspec, sizeof(dirspec));
        strcat(dirspec, "/main/maps");
        hFile = opendir(dirspec);
        if(hFile)
        {
            while(file = readdir(hFile))
            {
                char *extension = strrchr(file->d_name, '.');
                if(extension)
                {
                    if(!strcasecmp(extension, ".bvm"))
                        maps.push_back(file->d_name);
                }
            }
            closedir(hFile);
        }

#endif
    }
    return maps;
}



//
// Pour aller chercher la prochaine map clientVar.dkpp_loader
//
CString Server::queryNextMap()
{
    CString currentName = game->mapName;
    int lengthDif = mapList.size() - mapInfoList.size();
    for(int i = (int)mapList.size() - lengthDif; i < (int)mapList.size(); i++)
    {//because generating the map info requires creating the map and we dont want to do this midgame with the addmap command for fear of bugs, this is kept separately in map list
        game->mapName = mapList[i];
        game->createMap();//we just create and add any maps in maplist that arent also in the info list
        int mapArea = 0;
        for(int j = 1; j < game->map->size[0] - 1; j++)
            for(int k = 1; k < game->map->size[1] - 1; k++)
            {//get the total area
                if(game->map->cells[k*game->map->size[0] + j].passable)
                {
                    mapArea++;
                }
            }
        mapInfo mInfo = { game->mapName, mapArea, 1000000000 };
        mapInfoList.push_back(mInfo);
    }
    if(lengthDif > 0)
    {
        game->mapName = currentName;
        game->createMap();//since we're done with the game's map and other functions might not be, we put it back the way we found it, who needs proper OOP when you've got manners anyways?
    }
    int indexOfMax = -1;
    for(int i = 0; i < (int)mapInfoList.size(); ++i)
    {
        if(mapInfoList[i].mapName == currentName)
        {
            mapInfoList[i].lastPlayed = 1;
        }
        else
        {
            mapInfoList[i].lastPlayed++;
        }
        bool suitable = filterMapFromRotation(mapInfoList[i]);
        if(suitable)//Only select indices of suitable maps
        {
            if(indexOfMax == -1)
            {
                indexOfMax = i;
            }
            else if(mapInfoList[i].lastPlayed > mapInfoList[indexOfMax].lastPlayed)
            {
                indexOfMax = i;
            }
        }
    }
    if(indexOfMax >= 0)
    {
        CString infoOutput = mapInfoList[indexOfMax].mapName;
        infoOutput += ": ";
        infoOutput += mapInfoList[indexOfMax].mapArea;
        console->add(infoOutput, true, true);
        mapInfoList[indexOfMax].lastPlayed = 0;
        return mapInfoList[indexOfMax].mapName;
    }
    else
    {
        int n = (int)mapInfoList.size() - 1;
        int index = rand(0, n);
        mapInfoList[index].lastPlayed = 0;
        return mapInfoList[index].mapName;
    }
    return "";
}



//
// Update de babonet
//
void Server::updateNet(float delay, bool send)
{
    // On update le server
    char IPDuGars[16];
    int clientID = bb_serverUpdate(delay, UPDATE_SEND_RECV, IPDuGars);
    int playerID = -1;
    if(clientID > 0)
    {
        // On a un nouveu client!
        console->add(CString("\x3> A client has connected. Client ID : %i", clientID), true);

        // Check against ban list
        for(std::size_t i = 0; i < banList.size(); ++i)
        {
            if(banList[i].second == IPDuGars)
            {
                bb_serverDisconnectClient(clientID);
                console->add(CString("\x3> Disconnecting banned client, %s. IP: %s",
                    banList[i].first.s, banList[i].second.s), true);
                return;
            }
        }

        // On le crclientVar.dkpp_A
        playerID = game->createNewPlayerSV(clientID);
        if(playerID == -1)
        {
            // Oups!! Y a pus de place, on le canne
            bb_serverDisconnectClient(clientID);
            console->add("\x3> Disconnecting client, server is full", true);
        }
        else
            strcpy(game->players[playerID]->playerIP, IPDuGars);
        /*PlayerStats* ps = getStatsFromCache(game->players[playerID]->userID);
        if (ps != 0)
            ps->MergeStats(game->players[playerID]);*/
    }
    else if(clientID == BBNET_ERROR)
    {
        // Une erreur !!! On arrclientVar.dkpp_e tout !!!
        console->add(CString("\x3> Error : %s", bb_serverGetLastError()), true);
        needToShutDown = true;
    }
    else if(clientID < 0)
    {
        // On client a disconnectclientVar.dkpp_
        for(int i = 0; i < MAX_PLAYER; ++i)
        {
            if(game->players[i])
            {
                if(game->players[i]->babonetID == -clientID)
                {
                    // Save stats to cache
                    if(game->players[i]->timePlayedCurGame > EPSILON)
                        cacheStats(game->players[i]);
                    // On cancel le vote si on pensait le kicker!
                    if(game->voting.votingInProgress)
                    {
                        CString com = game->voting.votingWhat;
                        CString command = com.getFirstToken(' ');
                        if(command == "kick" || command == "ban")
                        {
                            if(com == textColorLess(game->players[i]->name))
                            {
                                //--- CANCEL THE VOTE!
                                net_svcl_vote_result voteResult;
                                voteResult.passed = false;
                                bb_serverSend((char*)(&voteResult), sizeof(net_svcl_vote_result), NET_SVCL_VOTE_RESULT);
                            }
                        }
                        else if(command == "kickid" || command == "banid")
                        {
                            if(com.toInt() == i)
                            {
                                //--- CANCEL THE VOTE!
                                net_svcl_vote_result voteResult;
                                voteResult.passed = false;
                                bb_serverSend((char*)(&voteResult), sizeof(net_svcl_vote_result), NET_SVCL_VOTE_RESULT);
                            }
                        }
                    }
                    // On le disconnect !!
                    console->add(CString("\x3> Player disconnected : %s ID:%i", game->players[i]->name.s, i), true);
                    // broadcast to potential remote admins
                    if(master) master->RA_DisconnectedPlayer(textColorLess(game->players[i]->name).s, game->players[i]->playerIP, (long)game->players[i]->playerID);
                    ZEVEN_SAFE_DELETE(game->players[i]);
                    net_svcl_player_disconnect playerDisconnect;
                    playerDisconnect.playerID = (char)i;
                    bb_serverSend((char*)&playerDisconnect, sizeof(net_svcl_player_disconnect), NET_SVCL_PLAYER_DISCONNECT, 0);
                    break;
                }
            }
        }
    }
}



#include "CMaster.h"
//
// On envoit les info au master
//
void Server::sendServerInfo()
{
    if(master && gameVar.sv_gamePublic)
        master->sendGameInfo(this);
}

//void Server::SendGlobalBan( CString nick , CString IP , int duration )
//{
//	// lets tell master server we want to send him a global ban
//	if(master)
//	{
//
//		// build ou packet
//
//		master->sendPacket(
//	}
//}



//
// Pour l'updater
//
void Server::update(float delay)
{
    if(game && isRunning)
    {
        /*	if (infoSendDelay == 0)
            {
                infoSendDelay = 0;
            }*/
        infoSendDelay += delay;
        if(infoSendDelay > 20)
        {
            sendServerInfo();
            infoSendDelay = 0;
        }

        // Check to see if we have auth request responses
        if(authRequests.size() > 0)
        {
            // Since deleting from a vector invaldates the iterator, we'll add to another
            // vector and swap.
            std::vector<CCurl*> stillRunning;

            // Iterate through requests
            for(std::vector<CCurl*>::iterator i = authRequests.begin(); i != authRequests.end(); ++i)
            {
                CCurl* request = (*i);
                if(!request->isRunning())
                {
                    console->add(CString("[Auth] Response (%i bytes) recieved", request->recieved()));

                    // Get player id
                    int id = *((int*)request->userData());
                    delete (int*)request->userData();

                    // Get user id
                    if(request->recieved() > 0 && game->players[id] && request->recieved() < MAX_CARAC - 1)
                    {
                        CString temp = CString("%s", request->response().c_str());
                        int userID = temp.toInt();

                        // Set player's id
                        if(userID > 0)
                        {
                            game->players[id]->userID = userID;
                            console->add(CString("[Auth] %s authenticated with id %i", game->players[id]->name.s, userID), true);
                            /*PlayerStats* ps = getStatsFromCache(game->players[id]->userID);
                            if (ps != 0)
                            {
                                ps->MergeStats(game->players[id]);

                                net_svcl_player_update_stats playerStats;
                                playerStats.playerID = (char)id;
                                playerStats.kills = (short)game->players[id]->kills;
                                playerStats.deaths = (short)game->players[id]->deaths;
                                playerStats.score = (short)game->players[id]->score;
                                playerStats.returns = (short)game->players[id]->returns;
                                playerStats.flagAttempts = (short)game->players[id]->flagAttempts;
                                playerStats.timePlayedCurGame = game->players[id]->timePlayedCurGame;

                                for (int i=0;i<MAX_PLAYER;++i)
                                {
                                    if (game->players[i])
                                        bb_serverSend((char*)&playerStats, sizeof(net_svcl_player_update_stats), NET_SVCL_PLAYER_UPDATE_STATS, game->players[i]->babonetID);
                                }
                            }*/
                        }
                        else
                        {
                            console->add(CString("[Auth] %s failed authentication", game->players[id]->name.s), true);
                            //// If this is a match server, kick unauthorized players
                            //bb_serverDisconnectClient(game->players[id]->babonetID);
                            //ZEVEN_SAFE_DELETE(game->players[id]);
                            //net_svcl_player_disconnect playerDisconnect;
                            //playerDisconnect.playerID = (char)id;
                            //bb_serverSend((char*)&playerDisconnect,sizeof(net_svcl_player_disconnect),NET_SVCL_PLAYER_DISCONNECT,0);
                        }

                    }

                    // We're done, delete it
                    delete *i;
                }
                else
                    stillRunning.push_back(*i);
            }

            authRequests.swap(stillRunning);
        }

        if(reportUploads.size() > 0)
        {
            std::vector<CCurl*>::iterator it = reportUploads.begin();
            for(; it != reportUploads.end(); )
            {
                if((*it)->isRunning() == false)
                {
                    std::string response = (*it)->response();
                    if(!response.empty() && response.size() <= 480)
                        console->add(CString("\x2Report Sent: %s", response.c_str()), true);
                    else
                        console->add("\x2Report Failure: Sent, but no response recieved.", true);

                    delete (*it);
                    it = reportUploads.erase(it);
                }
                else
                    it++;
            }
        }

        if(gameVar.sv_gamePublic && master)
        {
            //check to see if we have answers from our banned query
            for(int i = 0; i < 64; i++)
            {
                if(master->BannedAnswers[i].ID != -1)
                {
                    //the guy is banned
                    if(master->BannedAnswers[i].Answer)
                    {
                        // kick
                        if(master) master->RA_DisconnectedPlayer(textColorLess(game->players[master->BannedAnswers[i].ID]->name).s, game->players[master->BannedAnswers[i].ID]->playerIP, (long)game->players[master->BannedAnswers[i].ID]->playerID);
                        bb_serverDisconnectClient(game->players[master->BannedAnswers[i].ID]->babonetID);
                        ZEVEN_SAFE_DELETE(game->players[master->BannedAnswers[i].ID]);
                        net_svcl_player_disconnect playerDisconnect;
                        playerDisconnect.playerID = (char)master->BannedAnswers[i].ID;
                        bb_serverSend((char*)&playerDisconnect, sizeof(net_svcl_player_disconnect), NET_SVCL_PLAYER_DISCONNECT, 0);
                    }
                    master->BannedAnswers[i].ID = -1;
                    master->BannedAnswers[i].Answer = -1;
                }
            }
        }

        // update current checksum queries
        for(unsigned int nn = 0; nn < m_checksumQueries.size(); nn++)
        {
            if(!m_checksumQueries[nn]->Update(delay))
            {
                // we need to get rid of the client associated with that query, because we didnt receive the checksum in the right amount of time
                // kick
                if(game->players[m_checksumQueries[nn]->GetID()])
                {
                    if(master) master->RA_DisconnectedPlayer(textColorLess(game->players[m_checksumQueries[nn]->GetID()]->name).s, game->players[m_checksumQueries[nn]->GetID()]->playerIP, (long)game->players[m_checksumQueries[nn]->GetID()]->playerID);
                    bb_serverDisconnectClient(game->players[m_checksumQueries[nn]->GetID()]->babonetID);
                    ZEVEN_SAFE_DELETE(game->players[m_checksumQueries[nn]->GetID()]);
                    net_svcl_player_disconnect playerDisconnect;
                    playerDisconnect.playerID = (char)m_checksumQueries[nn]->GetID();
                    bb_serverSend((char*)&playerDisconnect, sizeof(net_svcl_player_disconnect), NET_SVCL_PLAYER_DISCONNECT, 0);
                }
                delete m_checksumQueries[nn];
                m_checksumQueries.erase(m_checksumQueries.begin() + nn);
                nn--;
            }
        }

        DelayedKicksMap::iterator it = delayedKicks.begin(), itTmp;
        for(; it != delayedKicks.end(); )
        {
            it->second.timeToKick -= delay;
            if(it->second.timeToKick < 0.0)
            {
                // kick
                if(game->players[it->second.playerID] &&
                    game->players[it->second.playerID]->babonetID == it->second.babonetID)
                {
                    if(master) master->RA_DisconnectedPlayer(textColorLess(game->players[it->second.playerID]->name).s, game->players[it->second.playerID]->playerIP, (long)game->players[it->second.playerID]->playerID);
                    bb_serverDisconnectClient(it->second.babonetID);
                    ZEVEN_SAFE_DELETE(game->players[it->second.playerID]);
                    net_svcl_player_disconnect playerDisconnect;
                    playerDisconnect.playerID = (char)it->second.playerID;
                    bb_serverSend((char*)&playerDisconnect, sizeof(net_svcl_player_disconnect), NET_SVCL_PLAYER_DISCONNECT, 0);
                }
                itTmp = it;
                it++;
                delayedKicks.erase(itTmp);
            }
            else
                it++;
        }

        // On update le server
        updateNet(delay, false);

        // On check si on change pas de map
        if(changeMapDelay > 0)
        {
            changeMapDelay -= delay;
            if(changeMapDelay <= 0)
            {
                changeMapDelay = 0;
                game->resetGameType(gameVar.sv_gameType);
                // On load la new map
                CString lastMap = game->mapName;
                game->mapName = nextMap;
                game->createMap();
                while((!game->map || !IsMapValid(*game->map, game->gameType)) && mapList.size() > 0)
                {
                    console->add("\x4> Map is missing some entities, removing it from the list");
                    needToShutDown = false;
                    isRunning = true;
                    CString mapNameTemp("%s", game->mapName.s);
                    game->mapName = "";
                    removemap(mapNameTemp);
                    game->mapName = mapNameTemp;
                    game->mapName = queryNextMap();
                    game->createMap();
                }
                if(!game->map)
                {
                    needToShutDown = true;
                    isRunning = false;
                    return;
                }
                else
                {
                    // On le dit au autres
                    net_svcl_map_change mapChange;
                    memcpy(mapChange.mapName, game->map->mapName.s, strlen(game->map->mapName.s) + 1);
                    mapChange.gameType = gameVar.sv_gameType;
                    bb_serverSend((char*)&mapChange, sizeof(net_svcl_map_change), NET_SVCL_MAP_CHANGE, 0);

                    // On change le round clientVar.dkpp_playing
                    net_svcl_round_state roundState;
                    game->roundState = GAME_PLAYING;
                    roundState.newState = game->roundState;
                    roundState.reInit = true; // On remets tout clientVar.dkpp_0, les scores etc
                    bb_serverSend((char*)&roundState, sizeof(net_svcl_round_state), NET_SVCL_GAME_STATE, 0);
                }
            }
        }

        // On change le type de game??
        if(gameVar.sv_gameType != game->gameType)
        {
            game->resetGameType(gameVar.sv_gameType);
        }

        // On update le timing de la game
        if(game->gameTimeLeft > 0) game->gameTimeLeft -= delay;
        if(game->roundTimeLeft > 0) game->roundTimeLeft -= delay;
        if(game->gameTimeLeft < 0)
        {
            game->gameTimeLeft = 0;

            // On a fini la partie! Next map
        }
        if(game->roundTimeLeft < 0)
        {
            game->roundTimeLeft = 0;

            // On a fini le round! Next round
        }

        // On check si on a pas fini un round ou une game
        if(game->roundState == GAME_PLAYING)
        {
            bool changeRoundState = false;

            // clientVar.dkpp_ c le plus simple
            if(game->gameType == GAME_TYPE_DM)
            {
                if(game->gameTimeLeft == 0 && gameVar.sv_gameTimeLimit > 0)
                {
                    game->roundState = GAME_DONT_SHOW;
                    changeRoundState = true;
                } // Sinon c'est temps illimitclientVar.dkpp_

                // On check si un joueur atteint pas le score max
                for(int i = 0; i < MAX_PLAYER; ++i)
                {
                    if(game->players[i])
                    {
                        if(game->players[i]->score >= gameVar.sv_scoreLimit && gameVar.sv_scoreLimit > 0)
                        {
                            game->roundState = GAME_DONT_SHOW;
                            changeRoundState = true;
                            break;
                        }
                    }
                }
            }

            // TDM
            if(game->gameType == GAME_TYPE_TDM)
            {
                // On check si on attein pas les max de score
                if(game->blueScore == game->redScore && game->redScore >= gameVar.sv_scoreLimit && gameVar.sv_scoreLimit > 0)
                {
                    game->roundState = GAME_DRAW;
                    changeRoundState = true;
                }
                else if(game->blueScore >= gameVar.sv_scoreLimit && gameVar.sv_scoreLimit > 0)
                {
                    game->roundState = GAME_BLUE_WIN;
                    changeRoundState = true;
                }
                else if(game->redScore >= gameVar.sv_scoreLimit && gameVar.sv_scoreLimit > 0)
                {
                    game->roundState = GAME_RED_WIN;
                    changeRoundState = true;
                }

                if(game->gameTimeLeft == 0 && gameVar.sv_gameTimeLimit > 0)
                {
                    game->roundState = GAME_DONT_SHOW;
                    changeRoundState = true;
                }
            }

            // Le mode CTF, assez simple aussi
            if(game->gameType == GAME_TYPE_CTF)
            {
                // On check si on attein pas les max de score
                if(game->blueWin == game->redWin && game->redWin >= gameVar.sv_winLimit && gameVar.sv_winLimit > 0)
                {
                    game->roundState = GAME_DRAW;
                    changeRoundState = true;
                }
                else if(game->blueWin >= gameVar.sv_winLimit && gameVar.sv_winLimit > 0)
                {
                    game->roundState = GAME_BLUE_WIN;
                    changeRoundState = true;
                }
                else if(game->redWin >= gameVar.sv_winLimit && gameVar.sv_winLimit > 0)
                {
                    game->roundState = GAME_RED_WIN;
                    changeRoundState = true;
                }

                // On check si le temps est clientVar.dkpp_oulclientVar.dkpp_
                if(game->gameTimeLeft == 0 && gameVar.sv_gameTimeLimit > 0)
                {
                    if(game->blueWin == game->redWin)
                    {
                        game->roundState = GAME_DRAW;
                        changeRoundState = true;
                    }
                    else if(game->blueWin > game->redWin)
                    {
                        game->roundState = GAME_BLUE_WIN;
                        changeRoundState = true;
                    }
                    else
                    {
                        game->roundState = GAME_RED_WIN;
                        changeRoundState = true;
                    }
                }
            }

            if(changeRoundState)
            {
                if(gameVar.sv_report && (game->roundState == GAME_RED_WIN ||
                    game->roundState == GAME_BLUE_WIN || game->roundState == GAME_DRAW))
                {
                    console->add("\x2Updating Stats Cache", true);
                    updateStatsCache();
                }
                clearStatsCache();

                changeMapDelay = 10; // Pour dire qu'on change de map apres le round
                nextMap = queryNextMap();
                net_svcl_round_state roundState;
                roundState.newState = game->roundState;
                roundState.reInit = false;
                bb_serverSend((char*)&roundState, sizeof(net_svcl_round_state), NET_SVCL_GAME_STATE, 0);
            }
        }



        // Est-ce que la babonet veut nous dire de quoi?
    /*	CString message = bb_serverGetLastMessage();
        if (!message.isNull())
        {
            console->add(CString("\x9> babonet : %s", message.s), true);
        }*/

        // On recv les messages
        char * buffer;
        int messageID;
        uint32_t babonetID;
        while(buffer = bb_serverReceive(babonetID, messageID))
        {
            // On gclientVar.dkpp_e les messages reclientVar.dkpp_the
            recvPacket(buffer, messageID, babonetID);
        }

        int nbPlayers = 0;
        // On update et send les ping
        for(int i = 0; i < MAX_PLAYER; ++i)
        {
            if(game->players[i])
            {
                nbPlayers++;

                if(!game->players[i]->waitForPong)
                {
                    // On est pret clientVar.dkpp_lui envoyer un ping?
                    if(game->players[i]->currentPingFrame >= 30)
                    {
                        game->players[i]->currentPingFrame = 0;
                        game->players[i]->waitForPong = true;
                        game->players[i]->connectionInterrupted = false;

                        // On lui send son pingdlidou
                        net_svcl_ping ping;
                        ping.playerID = char(i); // Ici on s'en occupe pas du ID
                        bb_serverSend((char*)&ping, sizeof(net_svcl_ping), NET_SVCL_PING, game->players[i]->babonetID);
                        continue;
                    }
                }
                else
                {
                    if(game->players[i]->currentPingFrame > game->players[i]->ping)
                    {
                        // On a dclientVar.dkpp_assclientVar.dkpp_l'encient ping, on le met clientVar.dkpp_jour (pour les updates lclientVar.dkpp_
                        game->players[i]->ping = game->players[i]->currentPingFrame;
                    }
                    if(game->players[i]->currentPingFrame > 30)
                    {
                        long saveFrame = game->players[i]->currentCF.frameID;
                        game->players[i]->currentCF = game->players[i]->netCF1; // Pour les autres joueurs
                        game->players[i]->currentCF.frameID = saveFrame;
                        game->players[i]->connectionInterrupted = true;
                        game->players[i]->sendPosFrame = 0; // On interrupt sont envoit de data non important (coordFrame), pour alclientVar.dkpp_er le tout
                    }

                    // Est-ce que clientVar.dkpp_ fait trop longtemps qu'on attends lclientVar.dkpp_??
                    if(game->players[i]->currentPingFrame > 300) // ouf clientVar.dkpp_ fait 3sec lclientVar.dkpp_ on est assez tolerant:P
                    {
                        // Save stats to cache
                        if(game->players[i]->timePlayedCurGame > EPSILON)
                            cacheStats(game->players[i]);
                        // Connection interrupted (on le disconnect, on est sclientVar.dkpp_clientVar.dkpp_e)
                        if(master) master->RA_DisconnectedPlayer(textColorLess(game->players[i]->name).s, game->players[i]->playerIP, (long)game->players[i]->playerID);
                        bb_serverDisconnectClient(game->players[i]->babonetID);
                        console->add("\x3> Disconnecting client, no respond since 3sec", true);
                        ZEVEN_SAFE_DELETE(game->players[i]);
                        net_svcl_player_disconnect playerDisconnect;
                        playerDisconnect.playerID = (char)i;
                        bb_serverSend((char*)&playerDisconnect, sizeof(net_svcl_player_disconnect), NET_SVCL_PLAYER_DISCONNECT, 0);
                        continue;
                        //						connectionInterrupted = true;
                        //						game->players[i]->currentCF = game->players[i]->netCF1; // Pour les autres joueurs
                    }
                }
                // On incrclientVar.dkpp_ente son ping
                game->players[i]->currentPingFrame++;
            }
        }

        for(int i = 0; i < MAX_PLAYER; ++i)
        {
            if(game->players[i])
            {
                if(game->players[i]->babySitTime <= EPSILON && game->players[i]->ping * 33 > gameVar.sv_maxPing && gameVar.sv_maxPing != 0)
                {
                    game->players[i]->pingOverMax += delay;
                    if(game->players[i]->pingOverMax > maxTimeOverMaxPing)
                    {
                        //console->add(CString("\x3> Client exceeded max ping for over %.0f seconds", maxTimeOverMaxPing), true);
                        console->sendCommand(CString("sayid %d Maximum ping exceeded, moving to spectator", game->players[i]->playerID));
                        console->sendCommand(CString("moveid %d %d", PLAYER_TEAM_SPECTATOR,
                            game->players[i]->playerID));

                        game->players[i]->pingOverMax = 0;
                    }
                }
                else
                    game->players[i]->pingOverMax = 0.0f;

                if(gameVar.sv_autoSpectateWhenIdle && game->players[i] &&
                    (game->players[i]->timeIdle > gameVar.sv_autoSpectateIdleMaxTime) &&
                    (game->players[i]->teamID == PLAYER_TEAM_RED ||
                        game->players[i]->teamID == PLAYER_TEAM_BLUE))
                {
                    console->add(CString("\x3> Too much idling, not enough playing"), true);
                    //console->sendCommand(CString("kickid %d", game->players[i]->playerID));
                    console->sendCommand(CString("moveid %d %d", PLAYER_TEAM_SPECTATOR,
                        game->players[i]->playerID));
                }
                if(gameVar.sv_sendJoinMessage && game->players[i]->timeInServer > 1.0f && game->players[i]->timeInServer < 1.0f + delay)
                {
                    CString sendPlayerJoinMessage("sayid %d ", game->players[i]->playerID);
                    sendPlayerJoinMessage += gameVar.sv_joinMessage;
                    sendPlayerJoinMessage += "\0";
                    console->sendCommand(sendPlayerJoinMessage);
                }
            }
        }

        // On update le jeu (clientVar.dkpp_ c'est autant client que server side)
        game->update(delay);

        // On check pour sender les coordframes des players au autres players s'il en ont le temps
        if(game->roundState == GAME_PLAYING)
        {
            net_clsv_svcl_player_coord_frame playerCoordFrame;
            for(int i = 0; i < MAX_PLAYER; i++)
            {
                if(game->players[i])
                {
                    game->players[i]->sendPosFrame++;
                    if(game->players[i]->sendPosFrame >= game->players[i]->avgPing && game->players[i]->sendPosFrame >= gameVar.sv_minSendInterval + nbPlayers / 8)
                    {
                        game->players[i]->sendPosFrame = 0;

                        // Lui il est ready clientVar.dkpp_se faire envoyer les coordFrames
                        for(int j = 0; j < MAX_PLAYER; ++j)
                        {
                            if(game->players[j])
                            {
                                if(j != i && game->players[j]->status == PLAYER_STATUS_ALIVE)
                                {
                                    playerCoordFrame.playerID = (char)j;
                                    playerCoordFrame.babonetID = game->players[j]->babonetID;
                                    playerCoordFrame.frameID = game->players[j]->currentCF.frameID;
                                    playerCoordFrame.mousePos[0] = (short)(game->players[j]->currentCF.mousePosOnMap[0] * 100);
                                    playerCoordFrame.mousePos[1] = (short)(game->players[j]->currentCF.mousePosOnMap[1] * 100);
                                    playerCoordFrame.mousePos[2] = (short)(game->players[j]->currentCF.mousePosOnMap[2] * 100);
                                    playerCoordFrame.position[0] = (short)(game->players[j]->currentCF.position[0] * 100);
                                    playerCoordFrame.position[1] = (short)(game->players[j]->currentCF.position[1] * 100);
                                    playerCoordFrame.position[2] = (short)(game->players[j]->currentCF.position[2] * 100);
                                    playerCoordFrame.vel[0] = (char)(game->players[j]->currentCF.vel[0] * 10);
                                    playerCoordFrame.vel[1] = (char)(game->players[j]->currentCF.vel[1] * 10);
                                    playerCoordFrame.vel[2] = (char)(game->players[j]->currentCF.vel[2] * 10);
                                    playerCoordFrame.camPosZ = 0;
                                    bb_serverSend((char*)&playerCoordFrame, sizeof(net_clsv_svcl_player_coord_frame), NET_CLSV_SVCL_PLAYER_COORD_FRAME, game->players[i]->babonetID, NET_UDP);
                                }

                                // On shoot aussi le ping de ce joueur
                                net_svcl_player_ping playerPing;
                                playerPing.playerID = (char)j;
                                playerPing.ping = (short)game->players[j]->ping;
                                bb_serverSend((char*)&playerPing, sizeof(net_svcl_player_ping), NET_SVCL_PLAYER_PING, game->players[i]->babonetID, NET_UDP);
                            }
                        }

                        /*		for (j=0;j<(int)game->projectiles.size();++j)
                                {
                                    Projectile * projectile = game->projectiles[j];

                                    if (projectile->projectileID >= (int)game->projectiles.size() || projectile->projectileID < 0)
                                    {
                                        console->add(CString("\x9> Server warning, trying to send a projectile with an out of range index : %i", j));
                                        continue;
                                    }

                                    net_svcl_projectile_coord_frame projectileCoordFrame;
                                    projectileCoordFrame.frameID = projectile->currentCF.frameID;
                                    projectileCoordFrame.projectileID = projectile->projectileID;
                                    projectileCoordFrame.position[0] = (short)(projectile->currentCF.position[0] * 100);
                                    projectileCoordFrame.position[1] = (short)(projectile->currentCF.position[1] * 100);
                                    projectileCoordFrame.position[2] = (short)(projectile->currentCF.position[2] * 100);
                                    projectileCoordFrame.vel[0] = (char)(projectile->currentCF.vel[0] * 10);
                                    projectileCoordFrame.vel[1] = (char)(projectile->currentCF.vel[1] * 10);
                                    projectileCoordFrame.vel[2] = (char)(projectile->currentCF.vel[2] * 10);
                                    projectileCoordFrame.uniqueID = projectile->uniqueID;
                                    bb_serverSend((char*)&projectileCoordFrame, sizeof(net_svcl_projectile_coord_frame), NET_SVCL_PROJECTILE_COORD_FRAME, game->players[i]->babonetID, NET_UDP);
                                }*/

                                // On shoot les info sur les horloges du server
                        net_svcl_synchronize_timer synchronizeTimer;
                        synchronizeTimer.frameID = frameID;
                        synchronizeTimer.gameTimeLeft = game->gameTimeLeft;
                        synchronizeTimer.roundTimeLeft = game->roundTimeLeft;
                        bb_serverSend((char*)&synchronizeTimer, sizeof(net_svcl_synchronize_timer), NET_SVCL_SYNCHRONIZE_TIMER, game->players[i]->babonetID, NET_UDP);
                    }
                }
            }

            // On check pour les flag
            if(game)
            {
                //--- Run the auto balance au 2mins
                if((game->gameType == GAME_TYPE_TDM || game->gameType == GAME_TYPE_CTF) &&
                    gameVar.sv_autoBalance)
                {
                    if(autoBalanceTimer == 0)
                    {
                        //--- Count how much player in each team
                        std::vector<Player*> reds;
                        std::vector<Player*> blues;
                        for(int i = 0; i < MAX_PLAYER; ++i)
                        {
                            if(game->players[i])
                            {
                                if(game->players[i]->teamID == PLAYER_TEAM_RED)
                                {
                                    reds.push_back(game->players[i]);
                                }
                                else if(game->players[i]->teamID == PLAYER_TEAM_BLUE)
                                {
                                    blues.push_back(game->players[i]);
                                }
                            }
                        }

                        //--- Check if they are even
                        if((int)reds.size() < (int)blues.size() - 1 ||
                            (int)blues.size() < (int)reds.size() - 1)
                        {
                            //--- Send autobalance notification
                            autoBalanceTimer = (float)gameVar.sv_autoBalanceTime;
                            bb_serverSend(0, 0, NET_SVCL_AUTOBALANCE, 0);
                        }
                    }
                    else
                    {
                        if(autoBalanceTimer > 0)
                        {
                            autoBalanceTimer -= delay;
                            if(autoBalanceTimer < 0)
                            {
                                autoBalanceTimer = 0;
                                autoBalance();
                            }
                        }
                    }
                }
                else
                {
                    autoBalanceTimer = 0;
                }

                //--- Run game type specific update
                if(game->map && game->gameType == GAME_TYPE_DM)
                {
                    //...
                }
                else if(game->map && game->gameType == GAME_TYPE_TDM)
                {
                    //...
                }
                else if(game->map && game->gameType == GAME_TYPE_CTF)
                {
                    updateCTF(delay);
                }
            }
        }

        // On update le server
        updateNet(delay, true);
    }

    // Transfer maps
    int bytesSent = 0;
    int bytesPerFrame = int(gameVar.sv_maxUploadRate * 1024 / 30);
    std::vector<SMapTransfer> temp;

    for(std::size_t i = 0; i < mapTransfers.size(); ++i)
    {
        // Limit upload rate
        if(bytesSent > bytesPerFrame)
        {
            // If a player can't get a chunk this time, make sure
            // the transfer is kept
            temp.push_back(mapTransfers[i]);
            continue;
        }

        // Open map
        if(mapTransfers[i].mapName != "") {
            CString filename("main/maps/%s.bvm", mapTransfers[i].mapName.s);
            FILE* fic = fopen(filename.s, "rb");

            if(fic)
            {
                net_svcl_map_chunk chunk;

                // Read in a chunk
                fseek(fic, 250 * mapTransfers[i].chunkNum, SEEK_SET);
                chunk.size = (unsigned short)fread(chunk.data, 1, 250, fic);

                // Send chunk
                bb_serverSend((char*)&chunk, sizeof(net_svcl_map_chunk), NET_SVCL_MAP_CHUNK, mapTransfers[i].uniqueClientID);

                // Accumulate bytes sent
                bytesSent += 250;

                // If some data was sent, keep this transfer
                if(chunk.size != 0)
                {
                    mapTransfers[i].chunkNum++;
                    temp.push_back(mapTransfers[i]);
                }
            }

            fclose(fic);
        }
    }

    // Replace old transfer list
    mapTransfers.swap(temp);


    frameID++;
}

bool Server::filterMapFromRotation(const mapInfo & map)
{
    int nbPlayer = 0;
    for(int i = 0; i < MAX_PLAYER; ++i)
    {
        if((game->players[i]) &&
            ((game->players[i]->teamID == PLAYER_TEAM_BLUE) || (game->players[i]->teamID == PLAYER_TEAM_RED)))
        {
            nbPlayer++;
        }
    }
    if(nbPlayer < 2)
        nbPlayer = 2;
    float tilesPerBabo = map.mapArea / (float)nbPlayer;
    if(tilesPerBabo < gameVar.sv_minTilesPerBabo || (tilesPerBabo > gameVar.sv_maxTilesPerBabo && gameVar.sv_maxTilesPerBabo != 0))
        return false;
    return true;
}

//
// Auto balance
//
void Server::autoBalance()
{
    //--- VoilclientVar.dkpp_ on autobalance les teams
    std::vector<Player*> reds;
    std::vector<Player*> blues;
    for(int i = 0; i < MAX_PLAYER; ++i)
    {
        if(game->players[i])
        {
            if(game->players[i]->teamID == PLAYER_TEAM_RED)
            {
                reds.push_back(game->players[i]);
            }
            else if(game->players[i]->teamID == PLAYER_TEAM_BLUE)
            {
                blues.push_back(game->players[i]);
            }
        }
    }

    //--- Check if they are uneven
    if((int)reds.size() < (int)blues.size() - 1)
    {
        //--- On balance!
        int nbToSwitch = (int)blues.size() - 1 - (int)reds.size();

        //--- Bon... comment on choisi les candidats, c'est simple,
        //    on switch les 2 plus poche en bas de la liste ^^
        while(nbToSwitch > 0)
        {
            int switchID = (int)blues.size() - 1;
            if(game->map->flagState[1] == blues[switchID]->playerID)
                switchID--;
            for(int i = switchID - 1; i >= 0; i--)
            {
                if(blues[switchID]->timePlayedCurGame > blues[i]->timePlayedCurGame && game->map->flagState[1] != blues[i]->playerID)
                    switchID = i;
            }
            blues[switchID]->teamID = game->assignPlayerTeam(blues[switchID]->playerID, PLAYER_TEAM_RED);
            //--- Switch him team
            net_clsv_svcl_team_request teamRequest;
            teamRequest.playerID = blues[switchID]->playerID;
            teamRequest.teamRequested = PLAYER_TEAM_RED;
            // On l'envoit clientVar.dkpp_tout le monde, (si clientVar.dkpp_ changclientVar.dkpp_
            bb_serverSend((char*)&teamRequest, sizeof(net_clsv_svcl_team_request), NET_CLSV_SVCL_TEAM_REQUEST, 0);
            nbToSwitch--;
        }
    }
    else if((int)blues.size() < (int)reds.size() - 1)
    {
        //--- On balance
        int nbToSwitch = (int)reds.size() - 1 - (int)blues.size();

        //--- Bon... comment on choisi les candidats, c'est simple,
        //    on switch les 2 plus poche en bas de la liste ^^
        while(nbToSwitch > 0)
        {
            int switchID = (int)reds.size() - 1;
            if(game->map->flagState[0] == reds[switchID]->playerID)
                switchID--;
            for(int i = switchID - 1; i >= 0; i--)
            {
                if(reds[switchID]->timePlayedCurGame > reds[i]->timePlayedCurGame && game->map->flagState[0] != reds[i]->playerID)
                    switchID = i;
            }
            reds[switchID]->teamID = game->assignPlayerTeam(reds[switchID]->playerID, PLAYER_TEAM_BLUE);
            //--- Switch him team
            net_clsv_svcl_team_request teamRequest;
            teamRequest.playerID = reds[switchID]->playerID;
            teamRequest.teamRequested = PLAYER_TEAM_BLUE;
            // On l'envoit clientVar.dkpp_tout le monde, (si clientVar.dkpp_ changclientVar.dkpp_
            bb_serverSend((char*)&teamRequest, sizeof(net_clsv_svcl_team_request), NET_CLSV_SVCL_TEAM_REQUEST, 0);
            nbToSwitch--;
        }
    }
    else
    {
        //--- Ha ok, les teams clientVar.dkpp_aient balancclientVar.dkpp_
    }
}



//
// Pour modifier une variable remotly
//
void Server::sendSVChange(CString varCom)
{
    if(varCom.len() > 255) varCom.resize(255);
    net_svcl_sv_change svChange;
    memcpy(svChange.svChange, varCom.s, varCom.len() + 1);
    bb_serverSend((char*)&svChange, sizeof(net_svcl_sv_change), NET_SVCL_SV_CHANGE, 0);
}

//
// Pour chatter
//
void Server::sayall(CString message)
{
    if(message.isNull()) return;
    if(game && isRunning)
    {
        // On send clientVar.dkpp_ sur la network oui messieur
        net_clsv_svcl_chat chat_message;

        //	chat_message.fromID = game->thisPlayer->playerID;
        chat_message.teamID = PLAYER_TEAM_SPECTATOR - 1; // All player!

        // On insert la couleur dclientVar.dkpp_endament du team
        // (une fois apres le nom du joueur, parce que ce dernier a surement
        // mis plein de caractclientVar.dkpp_es de couleurs)
        message.insert(" : \x8", 0);

        // On insert le nom du joueur
        message.insert("console", 0);


        // Si le message est trop grand, on le resize
        if(message.len() > 49 + 80) message.resize(49 + 80);

        // VoilclientVar.dkpp_ on copie le finale
        memcpy(chat_message.message, message.s, sizeof(char) * (message.len() + 1));

        // VoilclientVar.dkpp_ on send clientVar.dkpp_ sur le network!
        bb_serverSend((char*)&chat_message, sizeof(net_clsv_svcl_chat), NET_CLSV_SVCL_CHAT, 0);
    }
}

std::vector<invalidChecksumEntity> Server::getInvalidChecksums(unsigned long bbnetID, int number, int offsetFromEnd)
{
    sqlite3 *DB = 0;
    sqlite3_open("./bv2.db", &DB);

    std::vector<invalidChecksumEntity> list;
    //some infos to load the data
    char	*zErrMsg;		// holds error msg if any
    char	**azResult;		// contains the actual returned data
    int	nRow;			// number of record
    int	nColumn;		// number of column
    char	SQL[256];		// the query
    int maxRows = 50;

    if(number > maxRows)
        number = maxRows;
    //sprintf(SQL, CString("Select IP, Name From BadChecksum limit %i", maxRows).s);
    sprintf(SQL, CString("Select IP, Name From BadChecksum limit %i offset (select count(*) from BadChecksum) - %i",
        number, offsetFromEnd).s);
    sqlite3_get_table(DB, SQL, &azResult, &nRow, &nColumn, &zErrMsg);
    {
        for(int i = 0; i < nRow; i++)
        {
            invalidChecksumEntity tmp;
            char* ip = azResult[(i + 1) * nColumn];
            char* name = azResult[(i + 1) * nColumn + 1];
            int minLen = static_cast<int>((strlen(name) < 31) ? strlen(name) : 31);
            tmp.id = i + 1;
            strncpy(tmp.name, name, minLen);
            strncpy(tmp.playerIP, ip, 16);
            list.push_back(tmp);
        }
        sqlite3_free_table(azResult);
    }
    sqlite3_close(DB);
    return list;
}

void Server::deleteInvalidChecksums()
{
    sqlite3 *DB = 0;
    sqlite3_open("./bv2.db", &DB);

    sqlite3_exec(DB, "delete from BadChecksum", 0, 0, 0);

    sqlite3_close(DB);
}

int Server::getNumberOfInvalidChecksums()
{
    sqlite3 *DB = 0;
    sqlite3_open("./bv2.db", &DB);

    //some infos to load the data
    char	*zErrMsg;		// holds error msg if any
    char	**azResult;		// contains the actual returned data
    int	nRow;			// number of record
    int	nColumn;		// number of column
    char	SQL[256];		// the query
    int num = 0;

    sprintf(SQL, "select count(*) as Number from BadChecksum");
    sqlite3_get_table(DB, SQL, &azResult, &nRow, &nColumn, &zErrMsg);
    if(nRow == 1)
        num = atoi(azResult[1]);
    sqlite3_free_table(azResult);
    sqlite3_close(DB);
    return num;
}

void Server::cacheStats(const Player* player)
{
    if(player->userID == 0) // caching is only for logged users
        return;

    cacheStats(player, player->teamID);
}

void Server::cacheStats(const Player* player, int teamid)
{
    if(player->userID == 0) // caching is only for logged users
        return;

    //removeStatsFromCache(player->userID);
    PlayerStats* ps = new PlayerStats(player);
    ps->teamID = teamid;
    statsCache.insert(StatsCachePair(player->userID, ps));
}

PlayerStats* Server::getStatsFromCache(int userID)
{
    StatsCache::iterator it;
    if((it = statsCache.find(userID)) != statsCache.end())
        return it->second;
    return 0;
}

void Server::removeStatsFromCache(int userID)
{
    StatsCache::iterator it;
    if((it = statsCache.find(userID)) != statsCache.end())
    {
        delete it->second;
        statsCache.erase(it);
    }
}

void Server::clearStatsCache()
{
    StatsCache::iterator it;
    for(it = statsCache.begin(); it != statsCache.end(); it++)
        delete it->second;
    statsCache.clear();
}

void Server::updateStatsCache()
{
    //original cache may store few entries for player if he disconnected few times

    //cache that will store one cache element per team for each player
    StatsCache newCache;
    StatsCache::iterator it, itFound;

    std::pair<StatsCache::iterator, StatsCache::iterator> ret;

    for(it = statsCache.begin(); it != statsCache.end(); )
    {
        bool found = false;
        //check if player is in new cache
        //if yes, find cache element with same teamid and merge stats
        ret = newCache.equal_range(it->first);
        for(itFound = ret.first; itFound != ret.second; itFound++)
        {
            if(itFound->second->teamID == it->second->teamID)
            {
                itFound->second->MergeStats(it->second);
                found = true;
                break;
            }
        }

        //if not found, insert new
        if(found == false)
            newCache.insert(StatsCachePair(it->first, it->second));
        else
            delete it->second;
        statsCache.erase(it++);
    }

    statsCache = newCache;
    newCache.clear();

    //do the same but with active players
    for(int i = 0; i < MAX_PLAYER; i++)
    {
        if(game->players[i] == 0 || game->players[i]->timePlayedCurGame < EPSILON)/* ||
            (game->players[i]->teamID != PLAYER_TEAM_BLUE &&
            game->players[i]->teamID != PLAYER_TEAM_RED))*/
            continue;
        bool found = false;
        PlayerStats* ps = new PlayerStats(game->players[i]);
        if(ps->userID == 0)
        {
            delete ps;
            continue;
        }
        //check if player is in new cache
        //if yes, find cache element with same teamid and merge stats
        ret = statsCache.equal_range(game->players[i]->userID);
        for(itFound = ret.first; itFound != ret.second; itFound++)
        {
            if(itFound->second->teamID == game->players[i]->teamID)
            {
                itFound->second->MergeStats(ps);
                found = true;
                break;
            }
        }
        //if not found, insert new
        if(found == false)
            statsCache.insert(StatsCachePair(game->players[i]->userID, ps));
        else
            delete ps;
    }
}

void Server::addDelayedKick(unsigned long _babonetID, int _playerID, float _timeToKick)
{
    // add to list
    if(delayedKicks.find(_babonetID) == delayedKicks.end())
    {
        delayedKickStruct dks(_babonetID, _playerID, _timeToKick);
        delayedKicks.insert(DelayedKicksPair(_babonetID, dks));
    }
}

void Server::SendPlayerList(long in_peerId)
{
    if(!game) return;

    for(int i = 0; i < MAX_PLAYER; i++)
    {
        if(game->players[i])
        {
            net_ra_player_entry entry;
            CString name = textColorLess(game->players[i]->name);
            sprintf(entry.name, "%s", name.s);
            sprintf(entry.ip, "%s", game->players[i]->playerIP);
            entry.id = (long)game->players[i]->playerID;

            bb_peerSend(in_peerId, (char*)&entry, RA_PLAYER_ENTRY, sizeof(net_ra_player_entry), true);
        }
    }

}



void Server::updateCTF(float delay)
{
    int i;

    if(game->map->flagState[0] == -2)
    {
        for(i = 0; i < MAX_PLAYER; ++i)
        {
            if(game->players[i])
            {
                if(game->players[i]->teamID == PLAYER_TEAM_RED &&
                    game->players[i]->status == PLAYER_STATUS_ALIVE)
                {
                    float dis = distanceSquared(game->map->flagPodPos[0], CVector3f(
                        game->players[i]->currentCF.position[0], game->players[i]->currentCF.position[1], 0));
                    if(dis <= .25f*.25f)
                    {
                        // Ce joueur pogne le flag !!!!!!
                        game->map->flagState[0] = (char)i;

                        // On le dis au autres
                        net_svcl_change_flag_state flagState;
                        flagState.flagID = 0;
                        flagState.newFlagState = game->map->flagState[0];
                        flagState.playerID = (char)i;
                        bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);

                        // On fait emet un sont dépendament du team :P
                        console->add(CString("\x3> %s took the blue flag ID:%i", game->players[i]->name.s, i));
                        game->players[i]->flagAttempts++;
                        break;
                    }
                }
                if(game->players[i]->teamID == PLAYER_TEAM_BLUE &&
                    game->players[i]->status == PLAYER_STATUS_ALIVE &&
                    game->map->flagState[1] == (char)i)
                {
                    float dis = distanceSquared(game->map->flagPodPos[0], CVector3f(
                        game->players[i]->currentCF.position[0], game->players[i]->currentCF.position[1], 0));
                    if(dis <= .25f*.25f)
                    {
                        // Ce joueur pogne le flag !!!!!!
                        game->map->flagState[1] = -2; // On a scoooréééé !!!!!!

                        // On le dis au autres
                        net_svcl_change_flag_state flagState;
                        flagState.flagID = 1;
                        flagState.newFlagState = game->map->flagState[1];
                        flagState.playerID = (char)i;
                        bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);

                        // On fait emet un sont dépendament du team :P
                        CString message("\x03> \x01%s \x08scores for the Blue team! ID:%i", game->players[i]->name.s, i);
                        //console->add(CString("\x3> Blue team scores!"));
                        console->add(message);
                        game->players[i]->score++;
                        game->blueWin++;
                        game->blueScore = game->blueWin;
                        break;
                    }
                }
            }
        }
    }
    else if(game->map->flagState[0] == -1)
    {
        for(i = 0; i < MAX_PLAYER; ++i)
        {
            if(game->players[i])
            {
                if(game->players[i]->teamID == PLAYER_TEAM_RED &&
                    game->players[i]->status == PLAYER_STATUS_ALIVE)
                {
                    float dis = distanceSquared(game->map->flagPos[0], CVector3f(
                        game->players[i]->currentCF.position[0], game->players[i]->currentCF.position[1], 0));
                    if(dis <= .5f*.5f)
                    {
                        // Ce joueur pogne le flag !!!!!!
                        game->map->flagState[0] = (char)i;

                        // On le dis au autres
                        net_svcl_change_flag_state flagState;
                        flagState.flagID = 0;
                        flagState.newFlagState = game->map->flagState[0];
                        flagState.playerID = (char)i;
                        bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);

                        // On fait emet un sont dépendament du team :P
                        console->add(CString("\x3> %s took the blue flag ID:%i", game->players[i]->name.s, i));
                        game->players[i]->flagAttempts++;
                        break;
                    }
                }
                if(game->players[i]->teamID == PLAYER_TEAM_BLUE &&
                    game->players[i]->status == PLAYER_STATUS_ALIVE)
                {
                    float dis = distanceSquared(game->map->flagPos[0], CVector3f(
                        game->players[i]->currentCF.position[0], game->players[i]->currentCF.position[1], 0));
                    if(dis <= .5f*.5f)
                    {
                        // Ce joueur retourne le flag !
                        game->map->flagState[0] = -2;

                        // On le dis au autres
                        net_svcl_change_flag_state flagState;
                        flagState.flagID = 0;
                        flagState.newFlagState = game->map->flagState[0];
                        flagState.playerID = (char)i;
                        bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);

                        // On fait emet un sont dépendament du team :P
                        console->add(CString("\x3> %s returned the blue flag", game->players[i]->name.s));
                        game->players[i]->returns++; // Il gagne deux points pour avoir sauvé le flag !
                        //game->blueScore += 2;
                        break;
                    }
                }
            }
        }
    }


    if(game->map->flagState[1] == -2)
    {
        for(i = 0; i < MAX_PLAYER; ++i)
        {
            if(game->players[i])
            {
                if(game->players[i]->teamID == PLAYER_TEAM_BLUE &&
                    game->players[i]->status == PLAYER_STATUS_ALIVE)
                {
                    float dis = distanceSquared(game->map->flagPodPos[1], CVector3f(
                        game->players[i]->currentCF.position[0], game->players[i]->currentCF.position[1], 0));
                    if(dis <= .25f*.25f)
                    {
                        // Ce joueur pogne le flag !!!!!!
                        game->map->flagState[1] = (char)i;

                        // On le dis au autres
                        net_svcl_change_flag_state flagState;
                        flagState.flagID = 1;
                        flagState.newFlagState = game->map->flagState[1];
                        flagState.playerID = (char)i;
                        bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);

                        // On fait emet un sont dépendament du team :P
                        console->add(CString("\x3> %s took the red flag ID:%i", game->players[i]->name.s, i));
                        game->players[i]->flagAttempts++;
                        break;
                    }
                }
                if(game->players[i]->teamID == PLAYER_TEAM_RED &&
                    game->players[i]->status == PLAYER_STATUS_ALIVE &&
                    game->map->flagState[0] == (char)i)
                {
                    float dis = distanceSquared(game->map->flagPodPos[1], CVector3f(
                        game->players[i]->currentCF.position[0], game->players[i]->currentCF.position[1], 0));
                    if(dis <= .25f*.25f)
                    {
                        // Ce joueur pogne le flag !!!!!!
                        game->map->flagState[0] = -2; // On a scoooréééé !!!!!!

                        // On le dis au autres
                        net_svcl_change_flag_state flagState;
                        flagState.flagID = 0;
                        flagState.newFlagState = game->map->flagState[0];
                        flagState.playerID = (char)i;
                        bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);

                        // On fait emet un sont dépendament du team :P
                        CString message("\x03> \x01%s \x08scores for the Red team! ID:%i", game->players[i]->name.s, i);
                        //console->add(CString("\x3> Red team scores!"));
                        console->add(message);
                        game->players[i]->score++;
                        game->redWin++;
                        game->redScore = game->redWin;
                        break;
                    }
                }
            }
        }
    }
    else if(game->map->flagState[1] == -1)
    {
        for(i = 0; i < MAX_PLAYER; ++i)
        {
            if(game->players[i])
            {
                if(game->players[i]->teamID == PLAYER_TEAM_BLUE &&
                    game->players[i]->status == PLAYER_STATUS_ALIVE)
                {
                    float dis = distanceSquared(game->map->flagPos[1], CVector3f(
                        game->players[i]->currentCF.position[0], game->players[i]->currentCF.position[1], 0));
                    if(dis <= .25f*.25f)
                    {
                        // Ce joueur pogne le flag !!!!!!
                        game->map->flagState[1] = (char)i;

                        // On le dis au autres
                        net_svcl_change_flag_state flagState;
                        flagState.flagID = 1;
                        flagState.newFlagState = game->map->flagState[1];
                        flagState.playerID = (char)i;
                        bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);

                        // On fait emet un sont dépendament du team :P
                        console->add(CString("\x3> %s took the red flag ID:%i", game->players[i]->name.s, i));
                        game->players[i]->flagAttempts++;
                        break;
                    }
                }
                if(game->players[i]->teamID == PLAYER_TEAM_RED &&
                    game->players[i]->status == PLAYER_STATUS_ALIVE)
                {
                    float dis = distanceSquared(game->map->flagPos[1], CVector3f(
                        game->players[i]->currentCF.position[0], game->players[i]->currentCF.position[1], 0));
                    if(dis <= .25f*.25f)
                    {
                        // Ce joueur retourne le flag !
                        game->map->flagState[1] = -2;

                        // On le dis au autres
                        net_svcl_change_flag_state flagState;
                        flagState.flagID = 1;
                        flagState.newFlagState = game->map->flagState[1];
                        flagState.playerID = (char)i;
                        bb_serverSend((char*)&flagState, sizeof(net_svcl_change_flag_state), NET_SVCL_CHANGE_FLAG_STATE, 0);

                        // On fait emet un sont dépendament du team :P
                        console->add(CString("\x3> %s returned the blue flag", game->players[i]->name.s));
                        game->players[i]->returns++; // Il gagne deux points pour avoir sauvé le flag !
                        //game->redScore += 2;
                        break;
                    }
                }
            }
        }
    }
}




//
// On a reclientVar.dkpp_ un message yclientVar.dkpp_ !
//
void Server::recvPacket(char * buffer, int typeID, unsigned long bbnetID)
{
    int i;
    switch(typeID)
    {
    case NET_CLSV_MAP_REQUEST:
    {
        net_clsv_map_request request;
        memcpy(&request, buffer, sizeof(net_clsv_map_request));

        SMapTransfer mtrans;
        mtrans.chunkNum = 0;
        mtrans.mapName = request.mapName;
        mtrans.uniqueClientID = bbnetID;

        // Add to list, server will send chunks on each update
        mapTransfers.push_back(mtrans);

        break;
    };
    case NET_CLSV_VOTE:
    {
        if(!gameVar.sv_enableVote) return; // <- LÀ
        net_clsv_vote vote;
        memcpy(&vote, buffer, sizeof(net_clsv_vote));
        if(game)
        {
            if(game->players[vote.playerID])
            {
                std::vector<int>::iterator it = std::find(game->voting.activePlayersID.begin(),
                    game->voting.activePlayersID.end(), game->players[vote.playerID]->babonetID);
                if((!game->players[vote.playerID]->voted) && (it != game->voting.activePlayersID.end()))
                {
                    game->players[vote.playerID]->voted = true;

                    if(vote.value)
                    {
                        game->voting.votingResults[0]++;
                    }
                    else
                    {
                        game->voting.votingResults[1]++;
                    }

                    //--- Give the vote update to others
                    net_svcl_update_vote updateVote;
                    updateVote.nbYes = (char)game->voting.votingResults[0];
                    updateVote.nbNo = (char)game->voting.votingResults[1];
                    bb_serverSend((char*)(&updateVote), sizeof(net_svcl_update_vote), NET_SVCL_UPDATE_VOTE);
                }
            }
        }
        break;
    };
    case NET_CLSV_SVCL_VOTE_REQUEST:
    {
        net_clsv_svcl_vote_request voteRequest;
        memcpy(&voteRequest, buffer, sizeof(net_clsv_svcl_vote_request));
        if(game)
        {
            if(game->players[voteRequest.playerID])
            {
                //--- Is there a voting in progress?
                if(game->voting.votingInProgress)
                {
                    //--- Cancel that
                    console->add(CString("\x9> Warning, player %s trying to cast a vote, there is already a vote in progress", game->players[voteRequest.playerID]->name.s));
                }
                else
                {
                    //--- Check the validity of the vote, then cast it
                    if(!validateVote(voteRequest.vote))
                    {
                        console->add(CString("\x9> Warning, player %s trying to cast an invalid vote", game->players[voteRequest.playerID]->name.s));
                    }
                    else
                    {
                        for(int i = 0; i < MAX_PLAYER; ++i)
                        {
                            if(game->players[i])
                            {
                                game->players[i]->voted = false;
                            }
                        }
                        game->castVote(voteRequest);
                        /*for (i=0;i<MAX_PLAYER;++i)
                        {
                            if ( (game->players[i]) && (game->players[i]->teamID != PLAYER_TEAM_AUTO_ASSIGN) &&
                                (game->players[i]->teamID != PLAYER_TEAM_AUTO_ASSIGN) )
                                ++game->voting.nbActivePlayers;
                        }*/
                        game->voting.votingFrom = textColorLess(game->players[voteRequest.playerID]->name);
                        bb_serverSend((char*)(&voteRequest), sizeof(net_clsv_svcl_vote_request), NET_CLSV_SVCL_VOTE_REQUEST);
                    }
                }
            }
        }
        break;
    }
    case NET_CLSV_SVCL_PLAYER_SHOOT_MELEE:
    {
        net_clsv_svcl_player_shoot_melee playerShootMelee;
        memcpy(&playerShootMelee, buffer, sizeof(net_clsv_svcl_player_shoot_melee));
        if(game->players[playerShootMelee.playerID])
        {
            if(game->players[playerShootMelee.playerID]->status == PLAYER_STATUS_ALIVE)
            {
                if(game->players[playerShootMelee.playerID]->meleeWeapon)
                {
                    game->players[playerShootMelee.playerID]->meleeWeapon->shootMeleeSV(game->players[playerShootMelee.playerID]);
                    //--- Shoot clientVar.dkpp_ aux autres
                //  for (int i=0;i<MAX_PLAYER;++i)
                //  {
                //      if (game->players[i] && i != playerShootMelee.playerID)
                //      {
                    bb_serverSend((char*)&playerShootMelee, sizeof(net_clsv_svcl_player_shoot_melee), NET_CLSV_SVCL_PLAYER_SHOOT_MELEE, 0);//game->players[i]->babonetID);
        //      }
        //  }
                }
            }
        }
        break;
    }
    case NET_SVCL_CONSOLE:
    {
        if(game)
        {
            CString adminCommand = buffer;
            for(int i = 0; i < MAX_PLAYER; ++i)
            {
                if(game->players[i])
                {
                    if(game->players[i]->babonetID == bbnetID && game->players[i]->isAdmin)
                    {
                        CString log = "Executing command from ";
                        log += game->players[i]->name;
                        log += ", IP: ";
                        log += game->players[i]->playerIP;
                        console->add(log);
                        console->sendCommand(adminCommand, true, bbnetID);
                    }
                }
            }
        }
        break;
    }
    case NET_CLSV_ADMIN_REQUEST:
    {
        net_clsv_admin_request adminRequest;
        memcpy(&adminRequest, buffer, sizeof(net_clsv_admin_request));
        CString loginRecv(adminRequest.login);
        CString pwdRecv(adminRequest.password);

        if(!gameVar.zsv_adminPass.isNull() &&
            !gameVar.zsv_adminUser.isNull())
        {
            if(loginRecv.isNull() || pwdRecv.isNull())
            {
                for(int i = 0; i < MAX_PLAYER; ++i)
                {
                    if(game->players[i])
                    {
                        if(game->players[i]->babonetID == bbnetID)
                        {
                            game->players[i]->isAdmin = false;
                            break;
                        }
                    }
                }
                break;
            }
            RSA::MD5 login_((unsigned char*)gameVar.zsv_adminUser.s);
            CString login(login_.hex_digest());

            RSA::MD5 pwd_((unsigned char*)gameVar.zsv_adminPass.s);
            CString pwd(pwd_.hex_digest());

            /*console->add(CString("\x9> L: %s", login.s));
            console->add(CString("\x9> P: %s", loginRecv.s));
            console->add(CString("\x9> L: %s", pwd.s));
            console->add(CString("\x9> P: %s", pwdRecv.s));*/

            if(loginRecv == login && pwdRecv == pwd)
            {
                //--- Admin accepted !!
                for(int i = 0; i < MAX_PLAYER; ++i)
                {
                    if(game->players[i])
                    {
                        if(game->players[i]->babonetID == bbnetID)
                        {
                            bb_serverSend(0, 0, NET_SVCL_ADMIN_ACCEPTED, bbnetID);
                            game->players[i]->isAdmin = true;
                            CString logInName = "Admin name: ";
                            CString logInIP = "Admin IP: ";
                            logInName += game->players[i]->name;
                            logInIP += game->players[i]->playerIP;
                            console->add(logInName);
                            console->add(logInIP);
                            break;
                        }
                    }
                }
            }
            else
            {//Failed log in
                for(int i = 0; i < MAX_PLAYER; ++i)
                {
                    if(game->players[i])
                    {
                        if(game->players[i]->babonetID == bbnetID)
                        {
                            CString logInName = "Admin log-in attempt by: ";
                            CString logInIP = "Admin log-in attempt IP: ";
                            logInName += game->players[i]->name;
                            logInIP += game->players[i]->playerIP;
                            console->add(logInName);
                            console->add(logInIP);
                            break;
                        }
                    }
                }
            }
        }
        break;
    }
    case NET_CLSV_GAMEVERSION_ACCEPTED:
    {
        net_clsv_gameversion_accepted gameVersionAccepted;
        memcpy(&gameVersionAccepted, buffer, sizeof(net_clsv_gameversion_accepted));

        if(game->players[gameVersionAccepted.playerID])
        {
            if(gameVar.sv_password != "" && CString("%s", gameVersionAccepted.password) != gameVar.sv_password)
            {
                bb_serverDisconnectClient(game->players[gameVersionAccepted.playerID]->babonetID);
                break;
            }

            // On envoi clientVar.dkpp_CE player l'info sur la game
            net_svcl_server_info serverInfo;
            serverInfo.mapSeed = 0; // Pour l'instant on mettra rien (on va mettre le non dla map bientot)
            memcpy(serverInfo.mapName, game->mapName.s, game->mapName.len() + 1);
            serverInfo.blueScore = game->blueScore;
            serverInfo.redScore = game->redScore;
            serverInfo.blueWin = game->blueWin;
            serverInfo.redWin = game->redWin;
            serverInfo.gameType = game->gameType;
            bb_serverSend((char*)&serverInfo, sizeof(net_svcl_server_info), NET_SVCL_SERVER_INFO, game->players[gameVersionAccepted.playerID]->babonetID);

            // On lui envoit l'info sur le round
            net_svcl_round_state roundState;
            roundState.newState = game->roundState;
            roundState.reInit = false;
            bb_serverSend((char*)&roundState, sizeof(net_svcl_round_state), NET_SVCL_GAME_STATE, game->players[gameVersionAccepted.playerID]->babonetID);

            // On lui envoit la valeur de ses variables sv_
            gameVar.sendSVVar(game->players[gameVersionAccepted.playerID]->babonetID);

            // On lui envoit l'info sur tout les autres player
            for(i = 0; i < MAX_PLAYER; ++i)
            {
                if(game->players[i] && i != gameVersionAccepted.playerID)
                {
                    net_svcl_player_enum_state playerState;
                    playerState.playerID = (char)i;
                    memcpy(playerState.playerName, game->players[i]->name.s, game->players[i]->name.len() + 1);
                    memcpy(playerState.playerIP, game->players[i]->playerIP, 16);
                    playerState.kills = (short)game->players[i]->kills;
                    playerState.deaths = (short)game->players[i]->deaths;
                    playerState.score = (short)game->players[i]->score;
                    playerState.returns = (short)game->players[i]->returns;
                    playerState.flagAttempts = (short)game->players[i]->flagAttempts;
                    playerState.damage = (short)game->players[i]->damage;
                    playerState.status = (char)game->players[i]->status;
                    playerState.teamID = (char)game->players[i]->teamID;
                    playerState.life = game->players[i]->life;
                    playerState.dmg = game->players[i]->dmg;
                    playerState.babonetID = game->players[i]->babonetID;
                    memcpy(playerState.skin, game->players[i]->skin.s, (game->players[i]->skin.len() <= 6) ? game->players[i]->skin.len() + 1 : 7);
                    playerState.blueDecal[0] = (unsigned char)(game->players[i]->blueDecal[0] * 255.0f);
                    playerState.blueDecal[1] = (unsigned char)(game->players[i]->blueDecal[1] * 255.0f);
                    playerState.blueDecal[2] = (unsigned char)(game->players[i]->blueDecal[2] * 255.0f);
                    playerState.greenDecal[0] = (unsigned char)(game->players[i]->greenDecal[0] * 255.0f);
                    playerState.greenDecal[1] = (unsigned char)(game->players[i]->greenDecal[1] * 255.0f);
                    playerState.greenDecal[2] = (unsigned char)(game->players[i]->greenDecal[2] * 255.0f);
                    playerState.redDecal[0] = (unsigned char)(game->players[i]->redDecal[0] * 255.0f);
                    playerState.redDecal[1] = (unsigned char)(game->players[i]->redDecal[1] * 255.0f);
                    playerState.redDecal[2] = (unsigned char)(game->players[i]->redDecal[2] * 255.0f);
                    if(playerState.status == PLAYER_STATUS_ALIVE && game->players[i]->weapon)
                    {
                        playerState.weaponID = game->players[i]->weapon->weaponID;
                    }
                    else
                    {
                        playerState.weaponID = WEAPON_SMG;
                    }
                    bb_serverSend((char*)&playerState, sizeof(net_svcl_player_enum_state), NET_SVCL_PLAYER_ENUM_STATE, game->players[gameVersionAccepted.playerID]->babonetID);
                }
            }

            // Il faut lui envoyer tout les projectiles aussi!
            for(i = 0; i < (int)game->projectiles.size(); ++i)
            {
                Projectile * projectile = game->projectiles[i];

                net_clsv_svcl_player_projectile playerProjectile;
                playerProjectile.nuzzleID = 0; // Ici on s'en caliss
                playerProjectile.playerID = projectile->fromID;
                playerProjectile.position[0] = (short)(projectile->currentCF.position[0] * 100);
                playerProjectile.position[1] = (short)(projectile->currentCF.position[1] * 100);
                playerProjectile.position[2] = (short)(projectile->currentCF.position[2] * 100);
                playerProjectile.projectileType = projectile->projectileType;
                playerProjectile.vel[0] = (char)(projectile->currentCF.vel[0] * 10);
                playerProjectile.vel[1] = (char)(projectile->currentCF.vel[1] * 10);
                playerProjectile.vel[2] = (char)(projectile->currentCF.vel[2] * 10);
                playerProjectile.uniqueID = projectile->uniqueID;
                switch(playerProjectile.projectileType)
                {
                case PROJECTILE_ROCKET:
                    playerProjectile.weaponID = WEAPON_BAZOOKA;
                    break;
                case PROJECTILE_GRENADE:
                    playerProjectile.weaponID = WEAPON_GRENADE;
                    break;
                case PROJECTILE_COCKTAIL_MOLOTOV:
                    playerProjectile.weaponID = WEAPON_COCKTAIL_MOLOTOV;
                    break;
                case PROJECTILE_FLAME:
                    playerProjectile.weaponID = -2;
                    break;
                case PROJECTILE_LIFE_PACK:
                case PROJECTILE_DROPED_WEAPON:
                case PROJECTILE_DROPED_GRENADE:
                default:
                    playerProjectile.weaponID = -1;
                    break;
                }
                bb_serverSend((char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, game->players[gameVersionAccepted.playerID]->babonetID);
            }

            // Hum les deux sont des entities, note clientVar.dkpp_moi mclientVar.dkpp_e : les considclientVar.dkpp_er comme la meme chose next time

            // Envoyer l'etat des flags
            if(game->gameType == GAME_TYPE_CTF)
            {
                net_svcl_flag_enum flagEnum;
                flagEnum.flagState[0] = game->map->flagState[0];
                flagEnum.positionBlue[0] = game->map->flagPos[0][0];
                flagEnum.positionBlue[1] = game->map->flagPos[0][1];
                flagEnum.positionBlue[2] = game->map->flagPos[0][2];
                flagEnum.flagState[1] = game->map->flagState[1];
                flagEnum.positionRed[0] = game->map->flagPos[1][0];
                flagEnum.positionRed[1] = game->map->flagPos[1][1];
                flagEnum.positionRed[2] = game->map->flagPos[1][2];
                bb_serverSend((char*)&flagEnum, sizeof(net_svcl_flag_enum), NET_SVCL_FLAG_ENUM, game->players[gameVersionAccepted.playerID]->babonetID);
            }
        }
        break;
    }
    case NET_CLSV_SVCL_PLAYER_INFO:
    {
        net_clsv_svcl_player_info playerInfo;
        memcpy(&playerInfo, buffer, sizeof(net_clsv_svcl_player_info));
        if(game->players[playerInfo.playerID])
        {
            playerInfo.playerName[31] = '\0';
            game->players[playerInfo.playerID]->name = playerInfo.playerName;
            memcpy(playerInfo.playerIP, game->players[playerInfo.playerID]->playerIP, 16);
            bb_serverSend((char*)&playerInfo, sizeof(net_clsv_svcl_player_info), NET_CLSV_SVCL_PLAYER_INFO, 0);
            console->add(CString("server> %s joined the game id:%d", playerInfo.playerName, playerInfo.playerID), true);

            // broadcast the info at remote admins
            if(master) master->RA_NewPlayer(textColorLess(playerInfo.playerName).s, playerInfo.playerIP, (long)playerInfo.playerID);

            // if we are using the pro client/serv, generate a new hash query
            m_checksumQueries.push_back(new CChecksumQuery(playerInfo.playerID, bbnetID));

            if(gameVar.sv_gamePublic)
            {
                // test to see if we need to ask master if the guy is banned...legal issues
                bool canTest = true;
                if(strstr(playerInfo.playerIP, "192.168."))
                {
                    canTest = false;
                }

                if(strstr(playerInfo.playerIP, "127.0.0.1"))
                {
                    canTest = false;
                }

                if(canTest)
                {
                    stCacheBanned banned;
                    banned.ID = playerInfo.playerID;
                    sprintf(banned.IP, "%s", playerInfo.playerIP);
                    sprintf(banned.MAC, "%s", playerInfo.macAddr);


                    if(master)
                    {
                        master->sendPacket((char*)&banned, sizeof(stCacheBanned), CACHE_BANNED);
                    }
                }
            }

            //let's cache the player
            //see if his mac adress is already in the  list first, if so, overwrite
            for(int i = 0; i < 50; i++)
            {
                if(!stricmp(CachedPlayers[i].macAddr, playerInfo.macAddr) && CachedPlayers[i].Valid)
                {
                    memcpy(CachedPlayers[CachedIndex].IP, playerInfo.playerIP, 16);
                    memcpy(CachedPlayers[CachedIndex].NickName, playerInfo.playerName, 32);
                    memcpy(CachedPlayers[CachedIndex].macAddr, playerInfo.macAddr, 20);
                    return;
                }
            }
            memcpy(CachedPlayers[CachedIndex].IP, playerInfo.playerIP, 16);
            memcpy(CachedPlayers[CachedIndex].NickName, playerInfo.playerName, 32);
            memcpy(CachedPlayers[CachedIndex].macAddr, playerInfo.macAddr, 20);
            CachedPlayers[CachedIndex].Valid = true;
            CachedIndex++;

            if(CachedIndex == 50) CachedIndex = 0;

        }
        break;
    }
    case NET_SVCL_PLAY_SOUND:
    {
        //--- On l'envoit clientVar.dkpp_toute les autres player
        for(int i = 0; i < MAX_PLAYER; ++i)
        {
            if(game->players[i])
            {
                if(game->players[i]->babonetID != bbnetID)
                {
                    bb_serverSend(buffer, sizeof(net_svcl_play_sound), NET_SVCL_PLAY_SOUND, game->players[i]->babonetID);
                }
            }
        }
        break;
    }
    case NET_CLSV_SVCL_CHAT:
    {
        net_clsv_svcl_chat chat;
        memcpy(&chat, buffer, sizeof(net_clsv_svcl_chat));
        chat.message[129] = '\0';
        // On print dans console
        console->add(chat.message, false, false);

        // if everybody can see the messagem send it to everyone!
        if(chat.teamID == -2)
        {
            bb_serverSend((char*)&chat, sizeof(net_clsv_svcl_chat), NET_CLSV_SVCL_CHAT, 0);
        }
        else if(chat.teamID == PLAYER_TEAM_SPECTATOR)
        {
            for(int i = 0; i < MAX_PLAYER; ++i)
            {
                if(game->players[i])
                {
                    if(game->players[i]->teamID == PLAYER_TEAM_SPECTATOR)
                    {
                        bb_serverSend((char*)&chat, sizeof(net_clsv_svcl_chat), NET_CLSV_SVCL_CHAT, game->players[i]->babonetID);
                    }
                }
            }
        }
        else if(chat.teamID == PLAYER_TEAM_BLUE)
        {
            for(int i = 0; i < MAX_PLAYER; ++i)
            {
                if(game->players[i])
                {
                    if(game->players[i]->teamID == PLAYER_TEAM_BLUE)
                    {
                        bb_serverSend((char*)&chat, sizeof(net_clsv_svcl_chat), NET_CLSV_SVCL_CHAT, game->players[i]->babonetID);
                    }
                }
            }
        }
        else if(chat.teamID == PLAYER_TEAM_RED)
        {
            for(int i = 0; i < MAX_PLAYER; ++i)
            {
                if(game->players[i])
                {
                    if(game->players[i]->teamID == PLAYER_TEAM_RED)
                    {
                        bb_serverSend((char*)&chat, sizeof(net_clsv_svcl_chat), NET_CLSV_SVCL_CHAT, game->players[i]->babonetID);
                    }
                }
            }
        }

        // broadcast to potential remote admins
        CString chatString = chat.message;
        chatString = textColorLess(chatString);
        if(master)
        {
            // find the player id from bbnet id
            int playerid = -1;
            for(int i = 0; i < MAX_PLAYER; ++i)
            {
                if(game->players[i])
                {
                    if(game->players[i]->babonetID == bbnetID)
                    {
                        playerid = i;
                        break;
                    }
                }
            }
            master->RA_Chat(chatString.s, playerid);
        }

        break;
    }
    case NET_CLSV_SVCL_TEAM_REQUEST:
    {
        net_clsv_svcl_team_request teamRequest;
        memcpy(&teamRequest, buffer, sizeof(net_clsv_svcl_team_request));

        // Est-ce que ce player existe
        if(game->players[teamRequest.playerID])
        {
            char oldTeam = game->players[teamRequest.playerID]->teamID;
            char newTeam = game->assignPlayerTeam(teamRequest.playerID, teamRequest.teamRequested);
            if(newTeam != oldTeam)
            {
                if((oldTeam == PLAYER_TEAM_RED || oldTeam == PLAYER_TEAM_BLUE) &&
                    (game->players[teamRequest.playerID]->timePlayedCurGame > EPSILON))
                {
                    cacheStats(game->players[teamRequest.playerID], oldTeam);
                    game->players[teamRequest.playerID]->reinit();
                }
                teamRequest.teamRequested = newTeam;
                // On l'envoit clientVar.dkpp_tout le monde, (si clientVar.dkpp_ changclientVar.dkpp_
                bb_serverSend((char*)&teamRequest, sizeof(net_clsv_svcl_team_request), NET_CLSV_SVCL_TEAM_REQUEST, 0);
            }
        }
        break;
    }
    case NET_CLSV_PONG:
    {
        net_clsv_pong pong;
        memcpy(&pong, buffer, sizeof(net_clsv_pong));
        if(game->players[pong.playerID])
        {
            if(game->players[pong.playerID]->waitForPong)
            {
                game->players[pong.playerID]->waitForPong = false;
                game->players[pong.playerID]->ping = game->players[pong.playerID]->currentPingFrame;
            }
        }
        break;
    }
    case NET_CLSV_SPAWN_REQUEST:
    {
        // Tout respawn sera refussi on a fini le round ou la game
        if(game->roundState == GAME_PLAYING)
        {
            net_clsv_spawn_request spawnRequest;
            memcpy(&spawnRequest, buffer, sizeof(net_clsv_spawn_request));
            if(game->players[spawnRequest.playerID])
            {
                //--- Validate weapons, if the validation var is set
                if(gameVar.sv_validateWeapons)
                {
                    if(!(spawnRequest.weaponID == WEAPON_SMG ||
                        spawnRequest.weaponID == WEAPON_SHOTGUN ||
                        spawnRequest.weaponID == WEAPON_SNIPER ||
                        spawnRequest.weaponID == WEAPON_DUAL_MACHINE_GUN ||
                        spawnRequest.weaponID == WEAPON_CHAIN_GUN ||
                        spawnRequest.weaponID == WEAPON_BAZOOKA ||
                        spawnRequest.weaponID == WEAPON_PHOTON_RIFLE ||
                        spawnRequest.weaponID == WEAPON_FLAME_THROWER))
                    {
                        //--- Kick him so HARD!
                        console->sendCommand(CString("sayall %s is trying to hack his primary weapon! Kicked him", game->players[spawnRequest.playerID]->name.s));
                        scene->kick(spawnRequest.playerID);
                        return;
                    }

                    if(!(spawnRequest.meleeID == WEAPON_KNIVES ||
                        spawnRequest.meleeID == WEAPON_SHIELD
                        ))
                    {
                        //--- Kick him so HARD!
                        console->sendCommand(CString("sayall %s is trying to hack his secondary weapon! Kicked him", game->players[spawnRequest.playerID]->name.s));
                        scene->kick(spawnRequest.playerID);
                        return;
                    }
                }

                // *** small checkup to make sure people can't choose minibot if sv_enableMinibot is false
                game->players[spawnRequest.playerID]->nextSpawnWeapon = spawnRequest.weaponID;
                game->players[spawnRequest.playerID]->nextMeleeWeapon = spawnRequest.meleeID;

                // On lui trouve une place o le spawner
                // C'est impossible qu'il ne spawn pas (mais bon au cas)
                if(game->spawnPlayer(spawnRequest.playerID))
                {
                    // On renvois clientVar.dkpp_ clientVar.dkpp_tout le monde
                    net_svcl_player_spawn playerSpawn;
                    memcpy(playerSpawn.skin, spawnRequest.skin, 7);
                    memcpy(playerSpawn.blueDecal, spawnRequest.blueDecal, 3);
                    memcpy(playerSpawn.greenDecal, spawnRequest.greenDecal, 3);
                    memcpy(playerSpawn.redDecal, spawnRequest.redDecal, 3);
                    playerSpawn.weaponID = spawnRequest.weaponID;
                    playerSpawn.meleeID = spawnRequest.meleeID;
                    playerSpawn.playerID = spawnRequest.playerID;
                    playerSpawn.position[0] = (short)(game->players[spawnRequest.playerID]->currentCF.position[0] * 10);
                    playerSpawn.position[1] = (short)(game->players[spawnRequest.playerID]->currentCF.position[1] * 10);
                    playerSpawn.position[2] = (short)(game->players[spawnRequest.playerID]->currentCF.position[2] * 10);
                    spawnRequest.skin[6] = '\0';
                    game->players[spawnRequest.playerID]->skin = spawnRequest.skin;
                    game->players[spawnRequest.playerID]->blueDecal.set(
                        ((float)spawnRequest.blueDecal[0]) / 255.0f,
                        ((float)spawnRequest.blueDecal[1]) / 255.0f,
                        ((float)spawnRequest.blueDecal[2]) / 255.0f);
                    game->players[spawnRequest.playerID]->greenDecal.set(
                        ((float)spawnRequest.greenDecal[0]) / 255.0f,
                        ((float)spawnRequest.greenDecal[1]) / 255.0f,
                        ((float)spawnRequest.greenDecal[2]) / 255.0f);
                    game->players[spawnRequest.playerID]->redDecal.set(
                        ((float)spawnRequest.redDecal[0]) / 255.0f,
                        ((float)spawnRequest.redDecal[1]) / 255.0f,
                        ((float)spawnRequest.redDecal[2]) / 255.0f);
                    bb_serverSend((char*)&playerSpawn, sizeof(net_svcl_player_spawn), NET_SVCL_PLAYER_SPAWN, 0);

                    // add console message of where this guy spawned with what weapon/secondary (to help modders)
                    CString spawn = "Player ";
                    spawn += game->players[spawnRequest.playerID]->name;
                    spawn += " spawned; ID:";
                    spawn += playerSpawn.playerID;
                    spawn += " WeaponID:";
                    spawn += game->players[spawnRequest.playerID]->nextSpawnWeapon;
                    spawn += " SecondaryID:";
                    spawn += game->players[spawnRequest.playerID]->nextMeleeWeapon;
                    spawn += " Position:";
                    spawn += game->players[spawnRequest.playerID]->currentCF.position[0];
                    spawn += ",";
                    spawn += game->players[spawnRequest.playerID]->currentCF.position[1];
                    spawn += " teamID:";
                    spawn += game->players[spawnRequest.playerID]->teamID;

                    console->add(spawn);
                }
                else
                {
                    // Ici le joueur qui l'a requestclientVar.dkpp_est baisclientVar.dkpp_
                }
            }
        }
        break;
    }
    case NET_CLSV_SVCL_PLAYER_COORD_FRAME:
    {
        //  console->add("Server recved playerPos");
        net_clsv_svcl_player_coord_frame playerCoordFrame;
        memcpy(&playerCoordFrame, buffer, sizeof(net_clsv_svcl_player_coord_frame));
        if(game->players[playerCoordFrame.playerID])
        {
            if(gameVar.sv_beGoodServer == false &&
                (game->players[playerCoordFrame.playerID]->teamID == PLAYER_TEAM_RED ||
                    game->players[playerCoordFrame.playerID]->teamID == PLAYER_TEAM_BLUE) &&
                game->players[playerCoordFrame.playerID]->weapon->weaponID != WEAPON_SNIPER &&
                playerCoordFrame.camPosZ >= 9) // default z pos is 7
            {
                addDelayedKick(game->players[playerCoordFrame.playerID]->babonetID,
                    playerCoordFrame.playerID, 7);
            }

            //--- Is he alive? Else we ignore it
            if(game->players[playerCoordFrame.playerID]->status == PLAYER_STATUS_ALIVE)
            {
                if(game->players[playerCoordFrame.playerID]->babonetID == playerCoordFrame.babonetID)
                {
                    game->players[playerCoordFrame.playerID]->timeIdle = 0.0f;
                    //--- We compare the time between packet (important against cheating)
                    if(game->players[playerCoordFrame.playerID]->lastFrame == 0)
                    {
                        game->players[playerCoordFrame.playerID]->lastFrame = playerCoordFrame.frameID;
                    }
                    game->players[playerCoordFrame.playerID]->currentFrame = playerCoordFrame.frameID;
                    if(game->players[playerCoordFrame.playerID]->frameSinceLast >= 90)
                    {
                        // Check for acceleration hack
                        CVector3f vel;
                        vel[0] = (float)playerCoordFrame.vel[0] / 10.0f;
                        vel[1] = (float)playerCoordFrame.vel[1] / 10.0f;
                        vel[2] = (float)playerCoordFrame.vel[2] / 10.0f;

                        //console->add( CString( "vel : %f  %f  %f" , vel[0] , vel[1] , vel[2] ), true );

                        if((game->players[playerCoordFrame.playerID]->currentFrame - game->players[playerCoordFrame.playerID]->lastFrame > game->players[playerCoordFrame.playerID]->frameSinceLast + 5) ||
                            vel.length() > 3.3f)
                        {
                            //--- Hey, on a 10 frame de plus que le server.. hacking???
                            game->players[playerCoordFrame.playerID]->speedHackCount++;

                            //printf("hackcount++\n");

                            //--- Apres 3 shot (9sec) BOUM ON LE KICK L'ENFANT DE PUTE
                            if(game->players[playerCoordFrame.playerID]->speedHackCount >= 3)
                            {
                                // ?????? Save stats to cache
                                //cacheStats(game->players[playerCoordFrame.playerID]);

                                //printf("anti hack count = 3\n");
                                //--- On envoit clientVar.dkpp_tout le monde (y compris lui) quil essaye de speedhacking
                                //sayall(CString("Disconnecting %s (%s): Speed Hack Detected", game->players[playerCoordFrame.playerID]->name.s, game->players[playerCoordFrame.playerID]->playerIP));
                                if(master) master->RA_DisconnectedPlayer(textColorLess(game->players[playerCoordFrame.playerID]->name).s, game->players[playerCoordFrame.playerID]->playerIP, (long)game->players[playerCoordFrame.playerID]->playerID);
                                bb_serverDisconnectClient(game->players[playerCoordFrame.playerID]->babonetID);
                                //console->add(CString("\x3server> POSSIBLE HACKER: %s (%s), Speed Hack", game->players[playerCoordFrame.playerID]->name.s, game->players[playerCoordFrame.playerID]->playerIP), true);
                                ZEVEN_SAFE_DELETE(game->players[playerCoordFrame.playerID]);
                                net_svcl_player_disconnect playerDisconnect;
                                playerDisconnect.playerID = (char)playerCoordFrame.playerID;
                                bb_serverSend((char*)&playerDisconnect, sizeof(net_svcl_player_disconnect), NET_SVCL_PLAYER_DISCONNECT, 0);
                                return;
                            }
                        }
                        else
                        {
                            //--- Reset hacking count
                            game->players[playerCoordFrame.playerID]->speedHackCount = 0;
                        }

                        game->players[playerCoordFrame.playerID]->frameSinceLast = 0;
                        game->players[playerCoordFrame.playerID]->lastFrame = 0;
                        game->players[playerCoordFrame.playerID]->currentFrame = 0;
                    }
                    game->players[playerCoordFrame.playerID]->setCoordFrame(playerCoordFrame);


                    // Check for teleportation hack * Not anymore , too many problems
                    //CVector3f vDiff;
                    //vDiff = game->players[playerCoordFrame.playerID]->currentCF.position - game->players[playerCoordFrame.playerID]->lastCF.position;
                    //
                    //game->players[playerCoordFrame.playerID]->miNbCoord++;

                    //#ifdef WIN32
                    //  game->players[playerCoordFrame.playerID]->mfCumulativeVel += max( fabsf(vDiff.x()) , fabsf(vDiff.y()) );
                    //#else
                    //  game->players[playerCoordFrame.playerID]->mfCumulativeVel += amax( fabsf(vDiff.x()) , fabsf(vDiff.y()) );
                    //#endif

                    // as it been 3 seconds ?
                    // if so, check the average velocity
                    //if( game->players[playerCoordFrame.playerID]->mfCFTimer > 3.0f )
                    //{
                    //  game->players[playerCoordFrame.playerID]->mfCFTimer = 0;

                    //  float average =  game->players[playerCoordFrame.playerID]->mfCumulativeVel / 90.0f / dkcGetElapsedf();

                    //  //console->add(CString("average vel : %f   el %f",average,dkcGetElapsedf()));
                    //  // hax0rz
                    //  if( average > 2.5f )
                    //  {
                    //      sayall(CString("%s forced to spectator : teleporting suspected", game->players[playerCoordFrame.playerID]->name.s, game->players[playerCoordFrame.playerID]->playerIP));
                    //      net_clsv_svcl_team_request teamRequest;
                    //      teamRequest.playerID = playerCoordFrame.playerID;
                    //      teamRequest.teamRequested = PLAYER_TEAM_SPECTATOR;
                    //      bb_serverSend((char*)&teamRequest, sizeof(net_clsv_svcl_team_request), NET_CLSV_SVCL_TEAM_REQUEST, 0);
                    //      game->players[playerCoordFrame.playerID]->status = PLAYER_STATUS_DEAD;

                    //  }

                    //  game->players[playerCoordFrame.playerID]->miNbCoord = 0;
                    //  game->players[playerCoordFrame.playerID]->mfCumulativeVel = 0.0f;
                    //}


                    //if( fabsf(vDiff.x()) > 5.0f || fabsf(vDiff.y()) > 5.0f )
                    //{
                    //  // hax0rz
                    //  bb_serverDisconnectClient(game->players[playerCoordFrame.playerID]->babonetID);
                    //  ZEVEN_SAFE_DELETE(game->players[playerCoordFrame.playerID]);
                    //  net_svcl_player_disconnect playerDisconnect;
                    //  playerDisconnect.playerID = (char)playerCoordFrame.playerID;
                    //  bb_serverSend((char*)&playerDisconnect,sizeof(net_svcl_player_disconnect),NET_SVCL_PLAYER_DISCONNECT,0);
                    //  return;
                    //}


                }
                // Sinon ce babonet ID est dclientVar.dkpp_uet
            }
            else
            {
                game->players[playerCoordFrame.playerID]->speedHackCount = 0;
                game->players[playerCoordFrame.playerID]->frameSinceLast = 0;
                game->players[playerCoordFrame.playerID]->lastFrame = 0;
                game->players[playerCoordFrame.playerID]->currentFrame = 0;
            }
        }
        break;
    }
    case NET_CLSV_SVCL_PLAYER_CHANGE_NAME:
    {
        net_clsv_svcl_player_change_name playerChangeName;
        memcpy(&playerChangeName, buffer, sizeof(net_clsv_svcl_player_change_name));
        if(game->players[playerChangeName.playerID])
        {
            //what was this for?
            //if (game->players[playerChangeName.playerID]->status != PLAYER_STATUS_ALIVE)
            //  return;
            playerChangeName.playerName[31] = '\0';
            console->add(CString("\x3server> Player %s changed his name for %s", game->players[playerChangeName.playerID]->name.s, playerChangeName.playerName));
            game->players[playerChangeName.playerID]->name = playerChangeName.playerName;

            // On envoit maintenant clientVar.dkpp_ clientVar.dkpp_tout les autres joueurs
            for(int i = 0; i < MAX_PLAYER; ++i)
            {
                if(game->players[i] && i != playerChangeName.playerID)
                {
                    bb_serverSend((char*)&playerChangeName, sizeof(net_clsv_svcl_player_change_name), NET_CLSV_SVCL_PLAYER_CHANGE_NAME, game->players[i]->babonetID);
                }
            }
        }
        break;
    }
    case NET_CLSV_PLAYER_SHOOT:
    {
        net_clsv_player_shoot playerShoot;
        memcpy(&playerShoot, buffer, sizeof(net_clsv_player_shoot));
        if(game->players[playerShoot.playerID])
        {
            //--- Is he alive? Else we ignore it
            if(game->players[playerShoot.playerID]->status == PLAYER_STATUS_ALIVE || (game->players[playerShoot.playerID]->status == PLAYER_STATUS_DEAD && game->players[playerShoot.playerID]->timeDead < 0.2f))
            {

                //special case with the shotty and sniper that shots 2 and 5 bullets on the same frame :(
                if(playerShoot.weaponID == WEAPON_SHOTGUN || playerShoot.weaponID == WEAPON_SNIPER)
                {
                    if(game->players[playerShoot.playerID]->weapon->weaponID == WEAPON_SNIPER)
                    {
                        if(game->players[playerShoot.playerID]->currentCF.camPosZ >= 10.0f)
                            game->players[playerShoot.playerID]->weapon->nbShot = 3;
                        else
                            game->players[playerShoot.playerID]->weapon->nbShot = 2;
                    }

                    if(game->players[playerShoot.playerID]->mfElapsedSinceLastShot + 0.061f > gameVar.weapons[playerShoot.weaponID]->fireDelay)
                    {
                        //we are ok to shoot our first bullet
                        game->players[playerShoot.playerID]->shotCount = 1;
                        game->players[playerShoot.playerID]->mfElapsedSinceLastShot = 0.0f;

                    }
                    else
                    {
                        if(game->players[playerShoot.playerID]->shotCount < (playerShoot.weaponID == WEAPON_SHOTGUN ? 5 : 3))

                        {
                            game->players[playerShoot.playerID]->shotCount += 1;
                        }
                        else
                        {
                            return;
                        }
                    }

                    game->shootSV(playerShoot);
                    game->players[playerShoot.playerID]->firedShowDelay = 2;

                }
                else // not using a shotty or sniper ( so its SMG / DMG / CG obviously )
                {
                    if(game->players[playerShoot.playerID]->mfElapsedSinceLastShot < gameVar.weapons[playerShoot.weaponID]->fireDelay + 0.05f)
                    {//used to track the number of seconds an auto has been shooting for
                        game->players[playerShoot.playerID]->secondsFired += gameVar.weapons[playerShoot.weaponID]->fireDelay;
                    }
                    else
                    {
                        game->players[playerShoot.playerID]->secondsFired = 0;
                    }
                    if(game->players[playerShoot.playerID]->mfElapsedSinceLastShot + (playerShoot.weaponID == WEAPON_CHAIN_GUN ? 0.04f : 0.051f) > gameVar.weapons[playerShoot.weaponID]->fireDelay)
                    {
                        //we are ok to shoot our first bullet
                        //game->players[playerShoot.playerID]->shotCount = 1;
                        game->players[playerShoot.playerID]->mfElapsedSinceLastShot = 0.0f;
                    }
                    else
                    {
                        //if(game->players[playerShoot.playerID]->shotCount < 2)
                        //{
                        //  game->players[playerShoot.playerID]->shotCount += 1;
                        //}
                        //else
                        //{
                        return;
                        //}
                    }

                    game->shootSV(playerShoot);
                    game->players[playerShoot.playerID]->firedShowDelay = 2;
                }
            }
        }
        break;
    }
    case NET_CLSV_SVCL_PLAYER_PROJECTILE:
    {
        net_clsv_svcl_player_projectile playerProjectile;
        memcpy(&playerProjectile, buffer, sizeof(net_clsv_svcl_player_projectile));
        if(game->players[playerProjectile.playerID])
        {
            //--- Is he alive? Else we ignore it
            if(game->players[playerProjectile.playerID]->status == PLAYER_STATUS_ALIVE || (game->players[playerProjectile.playerID]->status == PLAYER_STATUS_DEAD
                && (playerProjectile.weaponID == WEAPON_GRENADE || playerProjectile.weaponID == WEAPON_COCKTAIL_MOLOTOV || playerProjectile.projectileType == PROJECTILE_ROCKET) && game->players[playerProjectile.playerID]->timeDead < 0.2f)
                )
            {
                if(playerProjectile.projectileType == PROJECTILE_COCKTAIL_MOLOTOV && !gameVar.sv_enableMolotov)
                {
                    return;
                }
                // if its a rocket we are launching, do we have a rocket launcher in hand ?
                if(playerProjectile.projectileType == PROJECTILE_ROCKET)
                {

                    //no rocket launcher in hand, dont launch anything
                    if(!(game->players[playerProjectile.playerID]->weapon->weaponID == WEAPON_BAZOOKA))
                    {
                        return;
                    }
                    if(gameVar.sv_zookaRemoteDet && gameVar.sv_serverType == 1)
                    {
                        if(game->players[playerProjectile.playerID]->rocketInAir && game->players[playerProjectile.playerID]->mfElapsedSinceLastShot > 0.25f)
                        {
                            //Rocket already in air?  Detonate and don't launch
                            game->players[playerProjectile.playerID]->detonateRocket = true;
                            return;
                        }
                    }
                }

                // Ignore nades and moltov since they are separate from other weapons
                if(playerProjectile.weaponID != WEAPON_GRENADE &&
                    playerProjectile.weaponID != WEAPON_COCKTAIL_MOLOTOV)
                {

                    // Check if shotsPerSecond is set by Player::Update
                    //if(game->players[playerProjectile.playerID]->shotsPerSecond > 0)

                    // enough time has elapsed for shotting
                    if(game->players[playerProjectile.playerID]->mfElapsedSinceLastShot + 0.1f > gameVar.weapons[playerProjectile.weaponID]->fireDelay)
                    {
                        if(!(game->spawnProjectile(playerProjectile, true)))
                        {
                            return;
                        }
                    }
                    else
                    {
                        return;
                    }
                    if(playerProjectile.projectileType == PROJECTILE_ROCKET)
                    {   //We're launching the rocket so flag it as in the air
                        game->players[playerProjectile.playerID]->rocketInAir = true;
                    }
                    game->players[playerProjectile.playerID]->mfElapsedSinceLastShot = 0.0f;
                }
                else
                {
                    if(!(game->spawnProjectile(playerProjectile, true)))
                    {
                        return;
                    }
                }



                // On l'envoit aux autres joueurs
                bb_serverSend((char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, 0);

            }
        }
        break;
    }
    case NET_CLSV_PICKUP_REQUEST:
    {
        net_clsv_pickup_request pickupRequest;
        memcpy(&pickupRequest, buffer, sizeof(net_clsv_pickup_request));
        if(game->players[pickupRequest.playerID])
        {
            //--- Is he alive? Else we ignore it
            if(game->players[pickupRequest.playerID]->status == PLAYER_STATUS_ALIVE)
            {
                // On check s'il n'y a pas un weapon clientVar.dkpp_pickuper proche
                for(int i = 0; i < (int)game->projectiles.size(); ++i)
                {
                    Projectile * projectile = game->projectiles[i];
                    if(projectile->projectileType == PROJECTILE_DROPED_WEAPON && !projectile->needToBeDeleted)
                    {
                        float dis = distanceSquared(
                            CVector3f(projectile->currentCF.position[0], projectile->currentCF.position[1], 0),
                            CVector3f(game->players[pickupRequest.playerID]->currentCF.position[0],
                                game->players[pickupRequest.playerID]->currentCF.position[1], 0));
                        if(dis <= .5f*.5f)
                        {
                            // On drope l'autre
                            net_clsv_svcl_player_projectile playerProjectile;
                            playerProjectile.nuzzleID = 0;
                            playerProjectile.playerID = game->players[pickupRequest.playerID]->playerID;
                            playerProjectile.position[0] = (short)(game->players[pickupRequest.playerID]->currentCF.position[0] * 100);
                            playerProjectile.position[1] = (short)(game->players[pickupRequest.playerID]->currentCF.position[1] * 100);
                            playerProjectile.position[2] = (short)(game->players[pickupRequest.playerID]->currentCF.position[2] * 100);
                            playerProjectile.vel[0] = (char)(game->players[pickupRequest.playerID]->currentCF.vel[0] * 10);
                            playerProjectile.vel[1] = (char)(game->players[pickupRequest.playerID]->currentCF.vel[1] * 10);
                            playerProjectile.vel[2] = (char)(game->players[pickupRequest.playerID]->currentCF.vel[2] * 10);
                            playerProjectile.weaponID = game->players[pickupRequest.playerID]->weapon->weaponID;
                            playerProjectile.projectileType = PROJECTILE_DROPED_WEAPON;
                            //  playerProjectile.uniqueProjectileID = ++(game->uniqueProjectileID);
                            game->spawnProjectile(playerProjectile, true);
                            bb_serverSend((char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, 0);

                            // On pickup clientVar.dkpp_ !!
                            game->players[pickupRequest.playerID]->switchWeapon(projectile->fromID);
                            net_svcl_pickup_item pickupItem;
                            pickupItem.playerID = pickupRequest.playerID;
                            pickupItem.itemType = ITEM_WEAPON;
                            pickupItem.itemFlag = projectile->fromID;
                            bb_serverSend((char*)&pickupItem, sizeof(net_svcl_pickup_item), NET_SVCL_PICKUP_ITEM, 0);
                            projectile->needToBeDeleted = true;
                            break;
                        }
                    }
                }
            }
        }
        break;
    }
    case NET_CLSV_MAP_LIST_REQUEST:
    {
        net_clsv_map_list_request maplRequest;
        memcpy(&maplRequest, buffer, sizeof(net_clsv_map_list_request));
        if(game->players[maplRequest.playerID])
        {
            std::vector<CString> maps = populateMapList(maplRequest.all);
            net_svcl_map_list mapl;
            for(int i = 0; i < (int)maps.size(); i++)
            {
                memset(mapl.mapName, 0, 16);
                strncpy(mapl.mapName, maps[i].s, std::min(maps[i].len(), 16));
                bb_serverSend((char*)&mapl, sizeof(net_svcl_map_list), NET_SVCL_MAP_LIST, game->players[maplRequest.playerID]->babonetID);
            }
        }
        break;
    }
    case NET_SVCL_HASH_SEED_REPLY:
    {
        net_svcl_hash_seed hashseed;
        memcpy(&hashseed, buffer, sizeof(net_svcl_hash_seed));

        // find associated checksum query
        for(unsigned int y = 0; y < m_checksumQueries.size(); y++)
        {
            if(m_checksumQueries[y]->GetBBid() == bbnetID)
            {
                // we found it
                if(m_checksumQueries[y]->isValid(hashseed))
                {
                    // this client is good
                    console->add(CString("\x9> Player %s was successfully authenticated", game->players[m_checksumQueries[y]->GetID()]->name.s));
                }
                else
                {
                    console->add(CString("\x9> Player %s was NOT successfully authenticated", game->players[m_checksumQueries[y]->GetID()]->name.s));
                    // this client isnt good, log IP + Name in the local database
                    sqlite3 *DB = 0;
                    sqlite3_open("bv2.db", &DB);
                    char    SQL[300];

                    sprintf(SQL, "Insert into BadChecksum(IP,Name) Values('%s','%s')", game->players[m_checksumQueries[y]->GetID()]->playerIP, game->players[m_checksumQueries[y]->GetID()]->name.s);
                    sqlite3_exec(DB, SQL, 0, 0, 0);

                    sqlite3_close(DB);
                }
                delete m_checksumQueries[y];
                m_checksumQueries.erase(m_checksumQueries.begin() + y);
                return;
            }
        }
        // if we arrive here, thats abnormal, kick the client

        break;
    }

    case NET_CLSV_SVCL_PLAYER_UPDATE_SKIN:
    {
        net_clsv_svcl_player_update_skin updateSkin;
        memcpy(&updateSkin, buffer, sizeof(net_clsv_svcl_player_update_skin));

        if(game)
        {
            for(int i = 0; i < MAX_PLAYER; ++i)
            {
                if(game->players[i])
                {
                    bb_serverSend((char*)(&updateSkin), sizeof(net_clsv_svcl_player_update_skin), NET_CLSV_SVCL_PLAYER_UPDATE_SKIN);
                }
            }
        }
        break;
    }
    }
}


