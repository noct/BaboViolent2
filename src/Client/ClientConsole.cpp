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
#include "ClientConsole.h"
#include "ClientScene.h"
#include <Zeven/FileIO.h>
#include "CMaster.h"
#include "Scene.h"
#include "ClientMap.h"
#include <algorithm>
#include <string>
#include <glad/glad.h>

#include "KeyManager.h"
#define CONSOLE_MAX_RECOGNITION_VAR 10

#include "md5.h"

// La scene, par o nos messages consoles passent
extern Scene * scene;

ClientConsole::ClientConsole()
{
    m_font = 0;
    m_isActive = false;
    m_vPos = 0;
    m_currentText = 0;
    recognitionVar = new char*[CONSOLE_MAX_RECOGNITION_VAR];
    for(int i = 0; i < CONSOLE_MAX_RECOGNITION_VAR; ++i)
    {
        recognitionVar[i] = new char[256]; // Pomal assez grand 256 pour un nom de variable + brief description
        recognitionVar[i][0] = '\0';
    }
    showRecognitionVar = false;
    curRecognitionVar = 0;
    locked = false;
    m_cmdHistoryID = -1;
    m_visibleMsgOffset = 0;
    SetDisplayEvents(true);
}


ClientConsole::~ClientConsole()
{
    ZEVEN_SAFE_DELETE(m_currentText);
    m_eventMessages.clear();
    m_chatMessages.clear();
    if(m_font) dkfDeleteFont(&m_font);
    for(int i = 0; i < CONSOLE_MAX_RECOGNITION_VAR; ++i)
    {
        ZEVEN_SAFE_DELETE_ARRAY(recognitionVar[i]);
    }
    ZEVEN_SAFE_DELETE_ARRAY(recognitionVar);
}

void ClientConsole::add(CString message, bool fromServer, bool isEvent)
{
    auto cscene = static_cast<ClientScene*>(scene);
    auto client = cscene->client;
    printf("%s\n", textColorLess(message).s);

    // broadcast to potential remote admins
    CString messageStr = textColorLess(message);
    if(master) master->RA_ConsoleBroadcast(messageStr.s);

    if(isEvent == true)
        m_eventMessages.push_back(message);
    else
        m_chatMessages.push_back(message);

    //--- On Check s'il y a pas un joueur admin.
    //    Si oui on lui envoit tout les messages console
    if(scene && fromServer)
    {
        if(scene->server)
        {
            for(int i = 0; i < MAX_PLAYER; ++i)
            {
                if(scene->server->game->players[i])
                {
                    if(scene->server->game->players[i]->isAdmin)
                    {
                        if(client)
                        {
                            if(client->game->thisPlayer == scene->server->game->players[i]) continue;
                        }
                        message = CString(">> ") + message;
                        bb_serverSend(message.s, message.len() + 1, NET_SVCL_CONSOLE, scene->server->game->players[i]->babonetID);
                    }
                }
            }
        }
    }


    if(gameVar.c_debug)
    {
        FileIO fileIO(m_outputFilename, "a");
        if(fileIO.isValid())
        {
            // avant de printer ? dans le fichier, il faut enlever les couleurs dans le texte
            int len = message.len();
            for(int i = 0; i < len; ++i)
            {
                if(message[i] >= '\x1' && message[i] <= '\x9')
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

//
// Pour afficher la console
//
void ClientConsole::render()
{
    int i;
    if(m_vPos > 0)
    {
        CVector2i res = dkwGetResolution();

        // on print ?l'?ran les 10 dernier messages encouru
        dkglPushOrtho((float)res[0], (float)res[1]);
        glTranslatef(0, m_vPos, 0);
        dkfBindFont(m_font);
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
        m_currentText->print(30, 20, 5, 0);
        glPushMatrix();
        const std::vector<CString>& displayMessages = GetActiveMessages();

        int linesPerPage;
        if(displayMessages.size() > 0)
            linesPerPage = (int)ceil(((gameVar.c_huge) ? 510 : 310) /
                dkfGetStringHeight(24, displayMessages[0].s));
        else
            linesPerPage = 0;
        for(i = m_visibleMsgOffset;
            i < std::min<int>(m_visibleMsgOffset + linesPerPage, (int)displayMessages.size());
            ++i)
        {
            if((int)(displayMessages.size()) - 1 - i < 0) break;
            float height = dkfGetStringHeight(24, displayMessages[displayMessages.size() - i - 1].s);
            glTranslatef(0, -height, 0);
            dkfPrint(24, 20, 0, 0, displayMessages[displayMessages.size() - i - 1].s);
        }
        glPopMatrix();
        if(showRecognitionVar)
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
            dkfPrint(30, 40, 45, 0, recognitionVar[0]);
            glTranslatef(0, dkfGetStringHeight(30, recognitionVar[0]), 0);

            // If we are cycling, get the other matches
            CString temp = CString(recognitionVar[0]).getFirstToken(' ');
            if(lastRecognitionVar != "")
                dksvarGetFilteredVar(lastRecognitionVar.s, recognitionVar, CONSOLE_MAX_RECOGNITION_VAR);

            // Render the rest
            for(i = curRecognitionVar + 1; i < CONSOLE_MAX_RECOGNITION_VAR; ++i)
            {
                dkfPrint(30, 40, 45, 0, recognitionVar[i]);
                float height = dkfGetStringHeight(30, recognitionVar[i]);
                glTranslatef(0, height, 0);
            }

            // Restore the recognition vars
            if(lastRecognitionVar != "")
                dksvarGetFilteredVar(temp.s, recognitionVar, CONSOLE_MAX_RECOGNITION_VAR);

            glPopMatrix();
        }
        glPopAttrib();
        dkglPopOrtho();
    }
}


//
// Pour updater la console (meton taper dedans, etc)
//
void ClientConsole::update(float delay)
{
    Console::update(delay);

    if(m_isActive)
    {
        if(!m_currentText->haveFocus()) m_currentText->giveFocus();

        if(dkiGetState(KeyF1) == DKI_DOWN)
            SetDisplayEvents(true);
        else if(dkiGetState(KeyF2) == DKI_DOWN)
            SetDisplayEvents(false);

        // Si on l'a activ?
        if(m_currentText->isActivated())
        {
            CString command = *m_currentText;
            CString token = command.getFirstToken(' ');
            if(token == "-")
                token = command.getFirstToken(' ');
            if(std::find(m_excludeFromLog.begin(), m_excludeFromLog.end(), token) == m_excludeFromLog.end())
                console->add(CString(": ") + *m_currentText);

            // On va g?er la commande dans la console m?e ici
            sendCommand(*m_currentText);

            m_cmdHistory.push_back(*m_currentText);
            while((int)m_cmdHistory.size() > m_maxCmdHistorySize)
                m_cmdHistory.erase(m_cmdHistory.begin());
            m_currentText->clear(); // On l'efface ensuite
            m_cmdHistoryID = -1;
            m_visibleMsgOffset = 0;
        }

        if(m_vPos < ((gameVar.c_huge) ? 510 : 310))
        {
            m_vPos += delay * 720 * 4;
            if(m_vPos > ((gameVar.c_huge) ? 510 : 310)) m_vPos = (float)((gameVar.c_huge) ? 510 : 310);
        }

        if(dkiGetState(KeyGrave) == DKI_DOWN)
        {
            m_isActive = false;
            m_currentText->clear();
            curRecognitionVar = 0;
            lastRecognitionVar = "";
            m_visibleMsgOffset = 0;
        }

        // On check si on n'est pas entrein d'?rire un set, l?va faloir ?rire la liste des vars
        if(strnicmp(m_currentText->s, "set ", 4) == 0 || strnicmp(m_currentText->s, "- set ", 6) == 0)
        {
            CString textCpy(m_currentText->s);
            CString token = textCpy.getFirstToken(' ');
            CString prefix;

            // Support remote admin
            if(token == "-")
            {
                token = textCpy.getNextToken(' ');
                prefix = "- set ";
            }
            else
            {
                token = textCpy.getFirstToken(' ');
                prefix = "set ";
            }

            dksvarGetFilteredVar(token.s, recognitionVar, CONSOLE_MAX_RECOGNITION_VAR);

            if(recognitionVar[0][0])
            {
                showRecognitionVar = true;
            }
            else
            {
                showRecognitionVar = false;
            }

            // If the user backspaces, reset the tab-completion
            if(m_lastToken.len() > token.len())
            {
                lastRecognitionVar = "";
                curRecognitionVar = 0;
            }
            m_lastToken = token;

            if(dkiGetState(KeyTab) == DKI_DOWN)
            {
                // If tab-completion has not yet been started, or the token has been backspaced
                // set a new recognition var
                if(lastRecognitionVar == "" || token.len() < lastRecognitionVar.len())
                {
                    lastRecognitionVar = token;
                }
                // Otherwise, if the token is the last recognized var, cycle
                else if((CString("%s", recognitionVar[0]).getFirstToken(' ') == token) == 1)
                {
                    curRecognitionVar++;
                    dksvarGetFilteredVar(lastRecognitionVar.s, recognitionVar, CONSOLE_MAX_RECOGNITION_VAR);

                    // Restart cycle if end is reached
                    if(curRecognitionVar == CONSOLE_MAX_RECOGNITION_VAR || CString("%s", recognitionVar[curRecognitionVar]) == "")
                    {
                        // Don't bother cycling if there is only 1 match
                        if(curRecognitionVar < 2)
                            lastRecognitionVar = "";
                        curRecognitionVar = 0;
                    }
                }
                else
                {
                    // Reset if no match
                    curRecognitionVar = 0;
                    lastRecognitionVar = "";
                }

                // Save the current token and replace the text
                m_lastToken = CString(recognitionVar[curRecognitionVar]).getFirstToken(' ');
                m_currentText->replaceText(prefix + CString(recognitionVar[curRecognitionVar]).getFirstToken(' ') + " ");
            }

        }
        else
        {
            showRecognitionVar = false;
        }

        if(dkiGetState(KeyUp) == DKI_DOWN)
        {
            if(m_cmdHistoryID == -1) m_cmdHistoryID = int(m_cmdHistory.size()) - 1;
            else if(m_cmdHistoryID > 0)
                m_cmdHistoryID--;
            if(m_cmdHistoryID != -1)
                m_currentText->replaceText(m_cmdHistory[m_cmdHistoryID]);
        }

        if(dkiGetState(KeyDown) == DKI_DOWN)
        {
            if(m_cmdHistoryID != -1)
                m_cmdHistoryID++;

            if(m_cmdHistoryID >= int(m_cmdHistory.size()))
            {
                m_cmdHistoryID = -1;
                m_currentText->replaceText("");
            }
            else if(m_cmdHistoryID != -1)
            {
                m_currentText->replaceText(m_cmdHistory[m_cmdHistoryID]);
            }
        }

        if(dkiGetState(KeyEscape) == DKI_DOWN)
        {
            m_currentText->replaceText("");
            curRecognitionVar = 0;
            lastRecognitionVar = "";
        }

        if(dkiGetState(KeyPageUp) == DKI_DOWN)
        {
            const std::vector<CString>& displayMessages = GetActiveMessages();
            int linesPerPage;
            if(displayMessages.size() > 0)
                linesPerPage = (int)ceil(((gameVar.c_huge) ? 510 : 310) /
                    dkfGetStringHeight(24, displayMessages[0].s));
            else
                linesPerPage = 0;
            if(m_visibleMsgOffset + m_historyMod < int(displayMessages.size()) - linesPerPage)
                m_visibleMsgOffset += m_historyMod;
            else if(int(displayMessages.size()) - linesPerPage < 0)
                m_visibleMsgOffset = 0;
            else
                m_visibleMsgOffset = int(displayMessages.size()) - linesPerPage;
            //m_currentText->replaceText(CString("> %i", m_visibleMsgOffset));
        }

        if(dkiGetState(KeyPageDown) == DKI_DOWN)
        {
            if(m_visibleMsgOffset > m_historyMod)
                m_visibleMsgOffset -= m_historyMod;
            else
                m_visibleMsgOffset = 0;
            //m_currentText->replaceText(CString("> %i", m_visibleMsgOffset));
        }
    }
    else
    {
        showRecognitionVar = false;
        m_currentText->loseFocus();
        if(m_vPos > 0)
        {
            m_vPos -= delay * 720 * 4;
            if(m_vPos < 0) m_vPos = 0;
        }

        if((dkiGetState(KeyGrave) == DKI_DOWN) && !locked)
        {
            m_isActive = true;
            m_currentText->clear(); // On l'efface au d?ut
        }
    }
}

void ClientConsole::init()
{
    m_currentText = new Writting();
    m_font = dkfCreateFont("main/fonts/babo.png");
    Console::init();
}

void ClientConsole::sendCommand(CString commandLine, bool isAdmin, unsigned long bbnetID)
{
    auto cscene = static_cast<ClientScene*>(scene);
    auto client = cscene ? cscene->client : nullptr;
    if(scene)
    {
        if(cscene->introScreen)
        {
            if(cscene->introScreen->showDelay > 0)
            {
                cscene->introScreen->showDelay = -1;
                scene->update(0.033f);
            }
        }
    }

    commandLine.trim('\n');

    CString tokenize = commandLine;

    // On va chercher le premier token, ? va nous donner la commande
    CString command = tokenize.getFirstToken(' ');

    // On fait un if sur toute les commandes possibles
    if(command == "help" || command == "?")
    {
        add("help ? info admin - set quit host dedicate voteon novote");
        add("playerlist maplist addmap removemap changemap connect");
        add("disconnect sayall sayteam edit restart kick kickid");
        add("banlist ban banid banip unban move moveid allwatch");
        return;
    }

    // File script
    if(command == "execute")
    {
        int i;
        FileIO file(CString("main/LaunchScript/%s.cfg", tokenize.s), "r");
        if(file.isValid())
        {
            // Clear previous vote settings
            sendCommand("novote");

            CString line = file.getLine();

            //--- We remove the \n at the end of the string
            line.trim('\n');

            //making sure win32 and linux have the same line feed
            line.trim('\r');

            while(line != "endscript")
            {
                //--- Put the ^color code.
                int len = line.len();
                CString newText;
                for(i = 0; i < len; ++i)
                {
                    if(line[i] == '^' && i < len - 1)
                    {
                        int number = line[i + 1] - '0';
                        switch(number)
                        {
                        case 1:newText += "\x1"; break;
                        case 2:newText += "\x2"; break;
                        case 3:newText += "\x3"; break;
                        case 4:newText += "\x4"; break;
                        case 5:newText += "\x5"; break;
                        case 6:newText += "\x6"; break;
                        case 7:newText += "\x7"; break;
                        case 8:newText += "\x8"; break;
                        case 9:newText += "\x9"; break;
                        }
                        ++i;
                    }
                    else
                    {
                        newText += CString("%c", line[i]);
                    }
                }
                line = newText;

                for(;;)
                {
                    if(strnicmp("//", line.s, 2) == 0) break;
                    if(line.s[0] == ' ' || line.s[0] == '\0' || line.s[0] == '\n') break;

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

    // Admin request
    if(command == "admin")
    {
        if(scene->server)
        {
            add(CString("\x9> You are already server"));
            return;
        }
        if(client)
        {
            CString login = tokenize.getFirstToken(' ');
            CString pwd = tokenize.getFirstToken(' ');
            if(login != "" && pwd != "")
            {
                net_clsv_admin_request adminRequest;
                memset(&adminRequest, 0, sizeof(net_clsv_admin_request));
                RSA::MD5 login_((unsigned char*)login.s);
                char* hex_digest = login_.hex_digest();
                memcpy(adminRequest.login, hex_digest, 32);

                RSA::MD5 pwd_((unsigned char*)pwd.s);
                hex_digest = pwd_.hex_digest();
                memcpy(adminRequest.password, hex_digest, 32);

                /*add(CString("\x9> L: %s", adminRequest.login));
                add(CString("\x9> P: %s", adminRequest.password));*/

                bb_clientSend(client->uniqueClientID, (char*)(&adminRequest), sizeof(net_clsv_admin_request), NET_CLSV_ADMIN_REQUEST);
            }
            else
            {
                client->isAdmin = false;
                add(CString("\x9> Invalid arguments"));
            }
        }
        if(client)
        {
            if(tokenize.isNull())
            {
                client->isAdmin = false;
            }
            bb_clientSend(client->uniqueClientID, tokenize.s, tokenize.len() + 1, NET_CLSV_ADMIN_REQUEST);
        }
        return;
    }

    if(command == "info")
    {
        if(scene->server && scene->server->game)
        {
            add(CString("\x9[Server Info] Game Type: %d - Port: %d - Name: %s", gameVar.sv_gameType, gameVar.sv_port, gameVar.sv_gameName.s), true);
        }
        return;
    }

    // Remote admin request
    if(command == "remoteAdmin")
    {
        // On join une game en cours y??!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        CString IPAddress = tokenize.getFirstToken(' ');
        int port = tokenize.getFirstToken(' ').toInt();
        if(port == 0) port = 3333; // Try on the regular port

        //--- The rest of the token is the user + pass :)
        cscene->join(IPAddress, port/*, tokenize*/);
        m_isActive = false;
        return;
    }

    // On envoit un message console au server
    if(command == "-")
    {
        if(client)
        {
            if(client->isAdmin)
            {
                bb_clientSend(client->uniqueClientID, tokenize.s, tokenize.len() + 1, NET_SVCL_CONSOLE);
            }
            else
            {
                add(CString("\x9> You need to be admin for this command"));
            }
        }
        return;
    }

    // Le set
    if(command == "set")
    {
        command = tokenize.getFirstToken(' ');

        // only set to true when changing sv_gameTimeLimit
        bool updateTimer = (command == "sv_gameTimeLimit");

        // only set true when changing weather effects setting
        bool reloadWeather = (command == "r_weatherEffects");

        if(strnicmp(command.s, "sv_", 3) == 0)
        {
            if(scene->server)
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

                if(command == "set sv_port")
                {
                    //  bb_serverChangePort(gameVar.sv_port);
                }
            }
            else
            {
                add(CString("\x9> This command is reserved to the server"));
                return;
            }
        }

        // On check, si cest une variable touche, on va chercher sa valeur avec notre keyManager
        if(command[0] == 'k')
        {
            // Si console on ignore ?
            CString keyName = tokenize;
            CString keyCommand = CString("set ") + command + " " + keyManager.getKeyByName(keyName);
            dksvarCommand(keyCommand.s);
            add(CString("\x3> %s", keyCommand.s));
            return;
        }

        // On donne la job ?dksvar pour ?
        CMD_RET cr = dksvarCommand(commandLine.s);
        if(cr == CR_NOSUCHVAR)
            add(CString("\x4> Unknown variable"));
        else if(cr == CR_INVALIDARGS)
            add(CString("\x4> Invalid arguments"));
        else if(cr == CR_NOTSUPPORTED)
            add(CString("\x4> Unknown command"));
        else
            add(CString("\x3> %s", command.s));

        // Do we need to update current game time?
        if(updateTimer)
        {
            if(scene)
            {
                if(scene->server)
                {
                    if(scene->server->game)
                    {
                        scene->server->game->gameTimeLeft = gameVar.sv_gameTimeLimit;
                    }
                }
            }
        }
        else if(reloadWeather && scene && client && client->game && client->game->map) {
            auto cmap = static_cast<ClientMap*>(client->game->map);
            cmap->reloadWeather();
        }

        return;
    }

    // Le quit
    if(command == "quit")
    {
        // On quit le jeu de force !!
        MainLoopForceQuit();
        add(CString("\x3> Quitting application..."));
        return;
    }
    // Pour hoster une game
    if(command == "host")
    {
        // On cr?le server
        cscene->host(tokenize);
        m_isActive = false;
        return;
    }
    // Pour hoster une game
    if(command == "dedicate")
    {
        // If already running, only change map, since if the server recieves an
        if(scene->server && scene->server->isRunning)
            scene->server->changeMap(tokenize);
        else
            scene->dedicate(tokenize);
        return;
    }

    // Allow command to be voted on
    if(command == "voteon")
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
    if(command == "novote")
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
    if(command == "addmap")
    {
        if(scene->server) scene->server->addmap(tokenize);
        return;
    }

    // Lister les player et leur IP !
    if(command == "playerlist")
    {
        if(scene->server)
        {
            if(scene->server->game)
            {
                for(int i = 0; i < MAX_PLAYER; ++i)
                {
                    if(scene->server->game->players[i])
                    {
                        add(CString("[%02i] %s - IP:%s", i,
                            scene->server->game->players[i]->name.s,
                            scene->server->game->players[i]->playerIP), true);
                    }
                }
            }
        }
        else if(client && client->game)
        {
            for(int i = 0; i < MAX_PLAYER; ++i)
            {
                if(client->game->players[i])
                {
                    if(client->game->players[i] != client->game->thisPlayer)
                        add(CString("[%02i] %s - IP:%s", i,
                            client->game->players[i]->name.s,
                            client->game->players[i]->playerIP));
                    else
                        add(CString("\x9[%02i]\x8 %s - IP:%s", i,
                            client->game->players[i]->name.s,
                            bb_getMyIP()));
                }
            }
        }
        return;
    }

    // Put player on a specified team
    if(command == "move")
    {
        if(scene->server)
        {
            if(scene->server->game)
            {
                int teamID = tokenize.getFirstToken(' ').toInt();
                int playerID = -1;
                if((teamID < -1) || (teamID > 1))
                {
                    add(CString("Error: team ID must be one of the following:\n    -1 for spectator, 0 for blue or 1 for red."), true);
                }
                else
                {
                    for(int i = 0; i < MAX_PLAYER; ++i)
                    {
                        if(scene->server->game->players[i] && (textColorLess(tokenize) == textColorLess(scene->server->game->players[i]->name)))
                        {
                            playerID = i;
                            break;
                        }
                    }
                    if(playerID == -1)
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
    if(command == "moveid")
    {
        if(scene->server)
        {
            if(scene->server->game)
            {
                int teamID = tokenize.getFirstToken(' ').toInt();
                int playerID = tokenize.getNextToken(' ').toInt();
                if((teamID < -1) || (teamID > 1))
                {
                    add(CString("Error: team ID must be one of the following:\n    -1 for spectator, 0 for blue or 1 for red."), true);
                }
                else if((playerID < -1) || (playerID >= MAX_PLAYER) || !scene->server->game->players[playerID])
                {
                    add(CString("Error: Bad player ID (use playerlist command to obtain the correct ID)"), true);
                }
                else
                {
                    if(playerID == -1)
                    {
                        // move everyone to the selected team
                        for(int i = 0; i < MAX_PLAYER; i++)
                        {
                            if(scene->server->game->players[i])
                            {
                                if(scene->server->game->map->flagState[0] == scene->server->game->players[i]->playerID)
                                {
                                    scene->server->game->map->flagState[0] = -1; // Le server va nous communiquer la position du flag exacte
                                    scene->server->game->map->flagPos[0] = scene->server->game->players[i]->currentCF.position;
                                    scene->server->game->map->flagPos[0][2] = 0;
                                }
                                if(scene->server->game->map->flagState[1] == scene->server->game->players[i]->playerID)
                                {
                                    scene->server->game->map->flagState[1] = -1; // Le server va nous communiquer la position du flag exacte
                                    scene->server->game->map->flagPos[1] = scene->server->game->players[i]->currentCF.position;
                                    scene->server->game->map->flagPos[1][2] = 0;
                                }
                                scene->server->game->players[i]->currentCF.position.set(-999, -999, 0);
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
                        if(scene->server->game->players[playerID])
                        {
                            if(scene->server->game->map->flagState[0] == scene->server->game->players[playerID]->playerID)
                            {
                                scene->server->game->map->flagState[0] = -1; // Le server va nous communiquer la position du flag exacte
                                scene->server->game->map->flagPos[0] = scene->server->game->players[playerID]->currentCF.position;
                                scene->server->game->map->flagPos[0][2] = 0;
                            }
                            if(scene->server->game->map->flagState[1] == scene->server->game->players[playerID]->playerID)
                            {
                                scene->server->game->map->flagState[1] = -1; // Le server va nous communiquer la position du flag exacte
                                scene->server->game->map->flagPos[1] = scene->server->game->players[playerID]->currentCF.position;
                                scene->server->game->map->flagPos[1][2] = 0;
                            }
                            scene->server->game->players[playerID]->currentCF.position.set(-999, -999, 0);
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
    if(command == "allwatch")
    {
        if(scene->server)
        {
            if(scene->server->game)
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
    if(command == "approveall")
    {
        if(scene->server)
        {
            if(scene->server->game)
            {
                int teamid;
                CString strteamid = tokenize.getFirstToken(' ');
                if(strteamid != "")
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

    if(command == "approveplayer")
    {
        if(scene->server)
        {
            if(scene->server->game)
            {
                int id;
                CString strid = tokenize.getFirstToken(' ');
                int teamid;
                CString strteamid = tokenize.getFirstToken(' ');
                if(strid != "" && strteamid != "")
                {
                    id = atoi(strid.s);
                    teamid = atoi(strteamid.s);
                    if(scene->server->game->approvePlayer(id, teamid) == false)
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
    if(command == "rejectplayer")
    {
        if(scene->server)
        {
            if(scene->server->game)
            {
                CString strid = tokenize.getFirstToken(' ');
                CString strteamid = tokenize.getFirstToken(' ');
                if(strid != "")
                {
                    if(strteamid == "")
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
    if(command == "rejectallplayers")
    {
        if(scene->server)
            if(scene->server->game)
            {
                scene->server->game->rejectAllPlayers();
                sendCommand("allwatch", true);
            }
        return;
    }

    // printout of approved players
    if(command == "listapprovedplayers")
    {
        if(scene->server)
        {
            if(scene->server->game)
            {
                add("\x3> Approved players:", true);
                int teams[2] = { PLAYER_TEAM_RED, PLAYER_TEAM_BLUE };
                CString strplayer;
                CString strteam;
                for(int i = 0; i < 2; i++)
                {
                    strteam.set("\x3 Team %d", teams[i]);
                    add(strteam, true);
                    if(scene->server->game->teamApproveAll[teams[i]] == true)
                        add("\x3 - All", true);
                    else
                    {
                        const std::vector<int>& userids = scene->server->game->approvedPlayers[teams[i]];
                        for(int j = 0; j < (int)userids.size(); j++)
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

    if(command == "savemap")
    {
        if(client)
        {
            if(client->game && client->game->map && client->game->mapBytesRecieved != 0)
            {
                if(client->game->thisPlayer)
                {
                    CString fileName = tokenize.getFirstToken(' ');
                    CString path;
                    if(fileName == "")
                    {
                        if(client->game->map->mapName == "")
                        {
                            add("\x3> Could not save map");
                            return;
                        }
                        fileName.set("%s.bvm", client->game->map->mapName.s);
                    }
                    else
                    {
                        int id = 0;
                        if(fileName.find(".bvm", id))
                        {
                            if(id == 0)
                            {
                                add("\x3> Invalid map name");
                                return;
                            }
                            else if(id != fileName.len() - 4)
                                fileName = fileName + ".bvm";
                        }
                        else
                            fileName = fileName + ".bvm";
                    }
                    path.set("./main/maps/%s", fileName.s);
                    FileIO file(path, "r");
                    if(file.isValid())
                    {
                        add("\x3> Map with same name exists already");
                        file.Close();
                        return;
                    }

                    file.Open(path, "wb");
                    if(file.isValid())
                    {
                        client->game->mapBuffer.reset();
                        file.put(client->game->mapBuffer.getByteArray(client->game->mapBytesRecieved),
                            client->game->mapBytesRecieved);
                        file.Close();
                        add(CString("\x3> Map saved as '%s'", fileName.s));
                    }
                    else
                        add(CString("\x3> Error while saving map '%s'", fileName.s));
                }
            }
        }
        return;
    }

    if(command == "savemapforce")
    {
        if(client)
        {
            if(client->game && client->game->map && client->game->mapBytesRecieved != 0)
            {
                if(client->game->thisPlayer)
                {
                    CString fileName = tokenize.getFirstToken(' ');
                    CString path;
                    if(fileName == "")
                    {
                        if(client->game->map->mapName == "")
                        {
                            add("\x3> Could not save map");
                            return;
                        }
                        fileName.set("%s.bvm", client->game->map->mapName.s);
                    }
                    path.set("./main/maps/%s", fileName.s);
                    FileIO file(path, "r");
                    if(file.isValid())
                    {
                        file.Close();
                        if(remove(path.s) == -1)
                            add(CString("\x3> Could not delete old map '%s'", path.s));
                    }
                    sendCommand(CString("savemap %s", fileName.s));
                }
            }
        }
        return;
    }

    // Pour lancer un vote (? c hot)
    if(command == "vote")
    {
        if(client)
        {
            if(client->game)
            {
                if(client->game->thisPlayer)
                {
                    //--- On send le token au server pour lancer le vote
                    if(tokenize.len() > 79) tokenize.resize(79);
                    net_clsv_svcl_vote_request voteRequest;
                    strcpy(voteRequest.vote, tokenize.s);
                    voteRequest.playerID = client->game->thisPlayer->playerID;
                    bb_clientSend(client->uniqueClientID, (char*)(&voteRequest), sizeof(net_clsv_svcl_vote_request), NET_CLSV_SVCL_VOTE_REQUEST);
                }
            }
        }
        return;
    }

    // Pour voir la queue des map
    if(command == "maplist")
    {
        if(scene->server)
        {
            for(int i = 0; i < (int)scene->server->mapList.size(); ++i)
            {
                add(scene->server->mapList[i], false);
            }

            if(scene->server->mapList.size() == 0)
                add("\x3> No maps on list", false);
        }
        else if(client)
        {
            if(client->game)
            {
                if(client->game->thisPlayer)
                {
                    net_clsv_map_list_request maplRequest;
                    maplRequest.playerID = client->game->thisPlayer->playerID;
                    maplRequest.all = false;
                    bb_clientSend(client->uniqueClientID, (char*)(&maplRequest), sizeof(net_clsv_map_list_request), NET_CLSV_MAP_LIST_REQUEST);
                }
            }
        }
        return;
    }

    if(command == "maplistall")
    {
        if(scene->server)
        {
            std::vector<CString> maps;
            maps = scene->server->populateMapList(true);
            for(int i = 0; i < (int)maps.size(); ++i)
            {
                add(maps[i], false);
            }

            if(maps.size() == 0)
                add("\x3> No maps on server", false);
        }
        else if(client)
        {
            if(client->game)
            {
                if(client->game->thisPlayer)
                {
                    net_clsv_map_list_request maplRequest;
                    maplRequest.playerID = client->game->thisPlayer->playerID;
                    maplRequest.all = true;
                    bb_clientSend(client->uniqueClientID, (char*)(&maplRequest), sizeof(net_clsv_map_list_request), NET_CLSV_MAP_LIST_REQUEST);
                }
            }
        }
        return;
    }

    // Pour enlever une map du queue
    if(command == "removemap")
    {
        if(scene->server) scene->server->removemap(tokenize);
        return;
    }

    // Pour changer la map, si on est server
    if(command == "changemap")
    {
        if(scene->server) scene->server->changeMap(tokenize);
        return;
    }
    // Pour rejoindre une game
    if(command == "connect")
    {
        // On join une game en cours y??!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        CString IPAddress = tokenize.getFirstToken(' ');
        int port = tokenize.getFirstToken(' ').toInt();
        CString password = tokenize;
        if(port == 0) port = 3333; // Try on the regular port
        cscene->join(IPAddress, port, password);
        m_isActive = false;
        return;
    }
    // Le quit
    if(command == "disconnect")
    {
        // On disconnect si c'est bien le cas
        scene->disconnect();
        return;
    }

    // Pour chatter
    if(command == "sayall")
    {
        scene->sayall(tokenize);
        return;
    }

    // Pour chatter
    if(command == "sayteam")
    {
        cscene->sayteam(tokenize);
        return;
    }
    // Pour modifier ou cr?r une map
    if(command == "edit")
    {
        cscene->edit(tokenize);
        m_isActive = false;
        return;
    }

    if(command == "rebuildmap")
    {
        if(scene && client && client->game && client->game->map)
        {
            auto cmap = static_cast<ClientMap*>(client->game->map);
            cmap->buildAll();
        }
    }

    // Pour carr?ent restarter toute la patente
    if(command == "restart")
    {
        dkContext* ctx = scene->ctx;
        ZEVEN_SAFE_DELETE(scene);
        scene = new ClientScene(ctx);
        return;
    }

    // KICK UN CRISS DE CHEATEUX ?MARDE
    if(command == "kick")
    {
        scene->kick(tokenize);
        return;
    }

    // KICK LE CHEATER PAR PLAYER ID
    if(command == "kickid")
    {
        int playerID = tokenize.toInt();
        if(playerID >= 0 && playerID < MAX_PLAYER)
        {
            scene->kick(playerID);
        }
        return;
    }

    // List bans
    if(command == "banlist")
    {
        if(scene->server)
        {
            for(int i = 0; i < (int)scene->server->banList.size(); ++i)
            {
                add(CString("[%02i] %s \x8- %s", i,
                    scene->server->banList[i].first.s,
                    scene->server->banList[i].second.s), true);
            }
        }
        return;
    }

    // Ban by name
    if(command == "ban")
    {
        scene->ban(tokenize);
        return;
    }

    // Ban by IP
    if(command == "banip")
    {
        scene->banIP(tokenize);
        return;
    }

    // Ban by player ID
    if(command == "banid")
    {
        int playerID = tokenize.toInt();
        if(playerID >= 0 && playerID < MAX_PLAYER)
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
    if(command == "cachelist")
    {
        if(scene->server)
        {
            for(int i = 0; i < 50; i++)
            {
                if(scene->server->CachedPlayers[i].Valid)
                {
                    //if a parameter was entered after the list, onlyshow those who fits
                    if(tokenize != "")
                    {
                        tokenize.toLower();
                        CString name(scene->server->CachedPlayers[i].NickName);
                        name.toLower();

                        if(strstr(name.s, tokenize.s))
                        {
                            add(CString("[%02i] %s \x8- %s  %s", i,
                                scene->server->CachedPlayers[i].NickName,
                                scene->server->CachedPlayers[i].IP,
                                scene->server->CachedPlayers[i].macAddr),
                                true);
                        }
                        else if(strstr(scene->server->CachedPlayers[i].IP, tokenize.s))
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
    if(command == "cacheban")
    {
        if(scene->server)
        {
            CString strPass = tokenize.getFirstToken(' ');
            //          if( strPass != "roxbabo" )
            //          {
            //              return;
            //          }

            CString strID = tokenize.getFirstToken(' ');
            if(strID == "")
            {
                return;
            }

            int ID = strID.toInt();
            if(ID >= 50 || ID < 0 || !scene->server->CachedPlayers[ID].Valid)
            {
                return;
            }

            CString strDuration = tokenize.getFirstToken(' ');
            int duration = 0;
            if(strDuration != "")
            {
                duration = strDuration.toInt();
                if(duration < 0) duration = 1;
            }

            stCacheBan cb;
            cb.Duration = duration;
            cb.ID = ID;
            sprintf(cb.IP, "%s", scene->server->CachedPlayers[ID].IP);
            sprintf(cb.MAC, "%s", scene->server->CachedPlayers[ID].macAddr);
            sprintf(cb.Nick, "%s", scene->server->CachedPlayers[ID].NickName);
            memcpy(cb.Pass, strPass.s, 7);
            cb.Pass[7] = '\0';

            // we have everything, tell the master server to ban him!
            master->sendPacket((char*)&cb, sizeof(stCacheBan), CACHE_BAN);

        }

        return;
    }

    // see who was cachebanned by the master server
    // parameter is a filter if we want to only list those containing some characters in the nick name
    if(command == "cachebanned")
    {
        if(scene->server)
        {
            CString strFilter = tokenize.getFirstToken(' ');
            stCacheList cl;
            sprintf(cl.Filter, "%s", strFilter.s);

            // we have everything, tell the master server to send us the cachebanned
            master->sendPacket((char*)&cl, sizeof(stCacheList), CACHE_BAN_LIST);

        }

        return;
    }


    // unban someone from master server
    // parameter is the pass + ID of the guy
    if(command == "cacheunban")
    {
        if(scene->server)
        {
            CString strPass = tokenize.getFirstToken(' ');
            if(strPass == "")
            {
                return;
            }

            CString strID = tokenize.getFirstToken(' ');
            if(strID == "")
            {
                return;
            }
            short ID = strID.toInt();

            stCacheUnban cu;
            cu.ID = ID;
            memcpy(cu.Pass, strPass.s, 7);
            cu.Pass[7] = '\0';

            // we have everything, tell the master server to unban this guy
            master->sendPacket((char*)&cu, sizeof(stCacheUnban), CACHE_UNBAN);

        }

        return;
    }

    // tell master to send us cache list of remote server (no need to have admin on that server)
    if(command == "cachelistremote")
    {
        if(scene->server && bbnetID != -1)
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
    if(command == "cachebanremote")
    {
        if(scene->server)
        {
            CString strPass = tokenize.getFirstToken(' ');
            if(strPass == "") return;

            CString strID = tokenize.getFirstToken(' ');
            if(strID == "") return;
            int ID = strID.toInt();
            if(ID >= 50 || ID < 0 || !master->CachedPlayersRemote[ID].Valid)
            {
                add("\x3> Invalid ID of player");
                return;
            }

            CString strDuration = tokenize.getFirstToken(' ');
            int duration = 0;
            if(strDuration != "")
            {
                duration = strDuration.toInt();
                if(duration < 0) duration = 1;
            }

            stCacheBan cb;
            cb.Duration = duration;
            cb.ID = ID;
            sprintf(cb.IP, "%s", master->CachedPlayersRemote[ID].IP);
            sprintf(cb.MAC, "%s", master->CachedPlayersRemote[ID].macAddr);
            sprintf(cb.Nick, "%s", master->CachedPlayersRemote[ID].NickName);
            memcpy(cb.Pass, strPass.s, 7);
            cb.Pass[7] = '\0';

            // we have everything, tell the master server to ban him!
            master->sendPacket((char*)&cb, sizeof(stCacheBan), CACHE_BAN);
        }

        //return;
        // "Unkown command" ;)
    }

    // getinvalidchecksums [offsetFromEnd=50 [number=50]]
    // request number(max 50) of entries from BadChecksums starting from number of entries-offsetFromEnd
    if(command == "getinvalidchecksums")
    {
        if(scene && scene->server)
        {
            int num = -1, offsetFromEnd = 50;
            CString offsetFromEndStr = tokenize.getFirstToken(' ');
            if(offsetFromEndStr != "")
                offsetFromEnd = offsetFromEndStr.toInt();
            CString numStr = tokenize.getFirstToken(' ');
            if(numStr != "")
                num = numStr.toInt();
            //scene->server->sendInvalidChecksums(bbnetID, num, offsetFromEnd);
            std::vector<invalidChecksumEntity> list = scene->server->getInvalidChecksums(bbnetID, num, offsetFromEnd);
            for(int i = 0; i < (int)list.size(); i++)
            {
                console->add(CString("%i) %s, IP: %s", list[i].id, list[i].name, list[i].playerIP));
            }
        }
        return;
    }

    if(command == "deleteinvalidchecksums")
    {
        if(scene && scene->server)
            scene->server->deleteInvalidChecksums();
        return;
    }

    if(command == "invalidchecksumsinfo")
    {

        if(scene && scene->server)
        {
            console->add(CString(">> %i", scene->server->getNumberOfInvalidChecksums()));
        }
        return;
    }

    add(CString("\x3> Unkown command : \"%s\"", command.s));
    add(CString("\x3> Type \"?\" for commands list", command.s));
    return;
}
