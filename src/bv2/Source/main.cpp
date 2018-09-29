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
#ifndef DEDICATED_SERVER
    #include "CLobby.h"
#endif
#include <exception>

// notre scene
Scene * scene = 0;

int resW = 800;
int resH = 600;
bool fullscreen = false;

char * bbNetVersion;

#ifdef DEDICATED_SERVER
bool quit = false;
void dkwForceQuit()
{
    quit = true;
}
#else
CVector2i mousePos_xbox;
CVector2f mousePos_xboxVel;
void updateXBoxMouse_main(float delay)
{
    CVector3f joy = dkiGetJoyR();

    joy[2] = 0;

    if (fabsf(joy[0]) < .25f) joy[0] = 0;
    if (fabsf(joy[1]) < .25f) joy[1] = 0;

    CVector2i res = dkwGetResolution();

    mousePos_xboxVel[0] += joy[0] * (30 + fabsf(mousePos_xboxVel[0]) * 0.0f) * delay;
    mousePos_xboxVel[1] += joy[1] * (30 + fabsf(mousePos_xboxVel[1]) * 0.0f) * delay;
    if (mousePos_xboxVel[0] > 2.5f) mousePos_xboxVel[0] = 2.5f;
    if (mousePos_xboxVel[0] < -2.5f) mousePos_xboxVel[0] = -2.5f;
    if (mousePos_xboxVel[1] > 2.5f) mousePos_xboxVel[1] = 2.5f;
    if (mousePos_xboxVel[1] < -2.5f) mousePos_xboxVel[1] = -2.5f;

    mousePos_xbox[0] += (int)(mousePos_xboxVel[0] * 600 * delay);
    mousePos_xbox[1] += (int)(mousePos_xboxVel[1] * 600 * delay);
    if (mousePos_xbox[0] < 0)
    {
        mousePos_xbox[0] = 0;
        mousePos_xboxVel[0] = 0;
    }
    if (mousePos_xbox[1] < 0)
    {
        mousePos_xbox[1] = 0;
        mousePos_xboxVel[1] = 0;
    }
    if (mousePos_xbox[0] > res[0])
    {
        mousePos_xbox[0] = res[0] - 1;
        mousePos_xboxVel[0] = 0;
    }
    if (mousePos_xbox[1] > res[1])
    {
        mousePos_xbox[1] = res[1] - 1;
        mousePos_xboxVel[1] = 0;
    }

    if (mousePos_xboxVel[0] > 0)
    {
        mousePos_xboxVel[0] -= delay * 25;
        if (mousePos_xboxVel[0] < 0) mousePos_xboxVel[0] = 0;
    }
    if (mousePos_xboxVel[0] < 0)
    {
        mousePos_xboxVel[0] += delay * 25;
        if (mousePos_xboxVel[0] > 0) mousePos_xboxVel[0] = 0;
    }
    if (mousePos_xboxVel[1] > 0)
    {
        mousePos_xboxVel[1] -= delay * 25;
        if (mousePos_xboxVel[1] < 0) mousePos_xboxVel[1] = 0;
    }
    if (mousePos_xboxVel[1] < 0)
    {
        mousePos_xboxVel[1] += delay * 25;
        if (mousePos_xboxVel[1] > 0) mousePos_xboxVel[1] = 0;
    }
}

CVector2i dkwGetCursorPos_main()
{
    if (gameVar.cl_enableXBox360Controller)
    {
        return mousePos_xbox;
    }
    else
    {
        return dkwGetCursorPos();
    }
}
#endif

#ifndef DEDICATED_SERVER
class MainLoopInterface : public CMainLoopInterface
{
public:

    dkContext* ctx;

    // Les fonctions obligatoire du MainLoopInterface de la dll dkw
    void paint()
    {

        // On va updater notre timer
        int nbFrameElapsed = dkcUpdateTimer(ctx);

#ifdef _DEBUG
        // LAG GENERATOR , use it to bind a key and test in lag conditions
        if (dkiGetState(KeyBackspace) == DKI_DOWN)
        {
            Sleep(300);
        }
#endif


        // On va chercher notre delay
        float delay = dkcGetElapsedf(ctx);

        // On passe le nombre de frame �animer
        while (nbFrameElapsed)
        {
            // On update nos input
            dkiUpdate(delay, resW, resH);

            // Xbox mouse pos
            updateXBoxMouse_main(delay);

            // On update le writing
            if (writting) writting->updateWritting(delay);

            // Update la console
            console->update(delay);

            // On appel nos fonction pour animer ici
            scene->update(delay);

            // On d�r�ente pour le prochain frame
            nbFrameElapsed--;
        }

        // On render le tout
        scene->render();
    }

    void textWrite(unsigned int caracter)
    {
        // Voil�juste un writting peut avoir le focus �la fois
        if (writting) writting->writeText(caracter);
    }
} mainLoopInterface;
#endif



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

#ifdef DEDICATED_SERVER

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
    master = new CMaster();

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

#else

int main(int argc, const char* argv[])
{
    dkConfig config = {};
    config.framePerSecond = 120;

    dkContext* ctx = dkInit(config);

    gameVar.init();

    dksvarInit(&stringInterface);
    dksvarLoadConfig("main/bv2.cfg");
    dksvarSaveConfig("main/bv2.cfg"); // On cre8 le config file aussi

    // On load tout suite le language utilis�par le joueur
    if (!gameVar.isLanguageLoaded())
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Can not load language file\nTry deleting the config file.", nullptr);
        dkFini(ctx);
        return 0;
    }

    if (gameVar.r_bitdepth != 16 && gameVar.r_bitdepth != 32) gameVar.r_bitdepth = 32;

    mainLoopInterface.ctx = ctx;

    //--- Windowed mode requires special handling
    if (!dkwInit(gameVar.r_resolution[0], gameVar.r_resolution[1], gameVar.r_bitdepth, gameVar.lang_gameName.s, &mainLoopInterface, gameVar.r_fullScreen, gameVar.r_refreshRate))
    {
        char * error = dkwGetLastError();
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error, nullptr);
        dkFini(ctx);
        return 0;
    }

    // On init les input
    if (!dkiInit(dkwGetHandle()))
    {
        dkwShutDown();
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating Input", nullptr);
        dkFini(ctx);
        return 0;
    }

    // Set single CPU usage
    if(gameVar.cl_affinityMode > 0)
    {
        ::SetProcessAffinityMask(::GetCurrentProcess(), 0x1);
    }

    // On cr�notre API openGL (This does nothing anymore
    if (!dkglCreateContext(dkwGetDC(), gameVar.r_bitdepth))
    {
        dkiShutDown();
        dkwShutDown();
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating openGL context", nullptr);
        dkFini(ctx);
        return 0;
    }

    // Restore system settings
    if(gameVar.cl_affinityMode == 1)
    {
        DWORD_PTR procMask;
        DWORD_PTR sysMask;
        ::GetProcessAffinityMask(::GetCurrentProcess(), &procMask, &sysMask);
        ::SetProcessAffinityMask(::GetCurrentProcess(), sysMask);
    }

    // On init les textures
    dktInit();

    // On init les objets 3D
    dkoInit();

    // On init les particles
    dkpInit();

    // On init le son
    if (!dksInit(gameVar.s_mixRate, gameVar.s_maxSoftwareChannels))
    {
        dkpShutDown();
        dkoShutDown();
        dkiShutDown();
        dkglShutDown();
        dkwShutDown();
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating fmod", nullptr);
        dkFini(ctx);
        return 0;
    }

    // On init la network
    if (bb_init() == 1)
    {
        dksShutDown();
        dkpShutDown();
        dkoShutDown();
        dkiShutDown();
        dkglShutDown();
        dkwShutDown();
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initiating baboNet", nullptr);
        dkFini(ctx);
        return 0;
    }
    bbNetVersion = bb_getVersion();
    if (CString("%s", bbNetVersion) != "4.0")
    {
        // Error
        bb_peerShutdown();
        bb_shutdown();
        dksShutDown();
        dkpShutDown();
        dkoShutDown();
        dkiShutDown();
        dkglShutDown();
        dkwShutDown();
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Wrong version of BaboNet", nullptr);
        dkFini(ctx);
        return 0;
    }

    // On cr�le lobby
    lobby = new CLobby();

    // On init la console
    console = new Console();
    console->init();

    //--- On cr�le master
    master = new CMaster();

    // On cr�notre scene
    scene = new Scene(ctx);

    ShowCursor(FALSE);

    // check command line options
    for(int i = 0; i < argc - 1; ++i)
    {
        CString str = CString("%s", argv[i]);
        if(str.len() > 1)
        {
            console->sendCommand(str);
        }
    }

    while (dkwMainLoop());

    // On efface la scene
    delete scene;
    scene = 0;

    // Delete master
    delete master;
    master = 0;

    delete console;
    console = 0;

    delete lobby;
    lobby = 0;

    dksvarSaveConfig("main/bv2.cfg");

    // On shutdown le tout (L'ordre est assez important ici)
    bb_peerShutdown();
    bb_shutdown();
    dksShutDown();
    dkpShutDown();
    dkoShutDown();
    dkfShutDown();
    dktShutDown();
    dkglShutDown();
    dkiShutDown();
    dkwShutDown();

    dkFini(ctx);
    return 0;
}

#endif

