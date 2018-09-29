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
#include <Zeven/CThread.h>
#include <Zeven/Core.h>
#include "Scene.h"
#include "Console.h"
#include "CMaster.h"
#include <exception>

// notre scene
Scene * scene = 0;

int resW = 800;
int resH = 600;
bool fullscreen = false;

char * bbNetVersion;

bool quit = false;
void MainLoopForceQuit()
{
    quit = true;
}

class StringInterface : public CStringInterface
{
public:
    virtual void updateString(CString* string, char * newValue)
    {
        *string = newValue;
    }
    StringInterface()
        {
            //printf("Constructor: 0x%x\n", this);
        }
    virtual ~StringInterface()
        {
        }
} stringInterface;

class CMainLoopConsole : public CThread
{
public:
    bool locked;
    bool internalLock;

    dkContext* ctx;

public:
    CMainLoopConsole()
    {
        locked = false;
        internalLock = false;
    }

    void execute(void* pArg)
    {
        while (!quit)
        {
            // On va updater notre timer
            int nbFrameElapsed = dkcUpdateTimer(ctx);

            // On va chercher notre delay
            float delay = dkcGetElapsedf(ctx);

            // On passe le nombre de frame �animer
            while (nbFrameElapsed)
            {
                // Update la console
                console->update(delay);

                // On appel nos fonction pour animer ici
                scene->update(delay);

                // On d�r�ente pour le prochain frame
                nbFrameElapsed--;
            }

            //--- On check si on n'est pas lock�avant de continuer
            if (locked)
            {
                internalLock = true;
            }

            while (internalLock)
            {
                dkcSleep(ctx, 1);
            }

            dkcSleep(ctx, 1);
        }
    }

    void lock()
    {
        locked = true;
        while (!internalLock)
        {
            dkcSleep(ctx, 1);
        }
    }

    void unlock()
    {
        locked = false;
        internalLock = false;
    }
};


int main(int argc, const char* argv[])
{
    // lil print out so that people now know that its working
    printf("***************************************\n");
    printf("*   Babo Violent 2 Dedicated Server   *\n");
    printf("*   Version 2.11d                     *\n");
    printf("*                                     *\n");
    printf("* check the /main/LaunchScript files  *\n");
    printf("* to configure your server            *\n");
    printf("***************************************\n\n\n");

    dkConfig config = {};
    config.framePerSecond = 30;

    dkContext* ctx = dkInit(config);

    // PREMI�E CHOSE �FAIRE, on load les config
    dksvarInit(&stringInterface);
    dksvarLoadConfig("main/bv2.cfg");
    dksvarSaveConfig("main/bv2.cfg"); // On cre8 le config file aussi


    // On init la network
    if (bb_init() == 1)
    {
        printf(NULL, "Error initiating BaboNet", "Error", 0);
        dkFini(ctx);
        return 0;
    }
    bbNetVersion = bb_getVersion();
    if(CString("%s", bbNetVersion) != "4.0")
    {
        // Error
        bb_peerShutdown();
        bb_shutdown();
        printf(NULL, "Wrong version of BaboNet", "Error", 0);
        dkFini(ctx);
        return 0;
    }

    // On init la console
    console = new Console();
    console->init();

    //--- On cr�le master
    master = new CMaster(nullptr);

    // On cr�notre scene
    scene = new Scene(ctx);

    // La loop principal
    CMainLoopConsole mainLoopConsole;
    mainLoopConsole.ctx = ctx;

    //--- On start la thread
    mainLoopConsole.start();

    //--- Get the arguments and send that to console
    if (argc > 1)
    {
        CString executeCmd = "execute ";
        executeCmd += (char*)(argv[1]);
        console->sendCommand(executeCmd);
    }

    char input[256];
    while (!quit)
    {
        std::cin.getline(input,256);

        mainLoopConsole.lock();
        if(std::cin.gcount())
        {
            console->sendCommand(input);
        }
        mainLoopConsole.unlock();

        dkcSleep(ctx, 1);
    };

    // On efface la scene et ses amis
    delete scene;
    delete master;
    delete console;
    console = 0;
    master = 0;
    scene = 0;

    dksvarSaveConfig("main/bv2.cfg");

    // On shutdown le tout (L'ordre est assez important ici)
    bb_peerShutdown();
    bb_shutdown();

    dkFini(ctx);
    return 0;
}
