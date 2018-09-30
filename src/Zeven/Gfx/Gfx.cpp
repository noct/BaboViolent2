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
/* TCE (c) All rights reserved */

#include <Zeven/Gfx.h>
#include <SDL2/SDL.h>

struct dkGfxContext
{
    int temp;
};

int dkwInit(int width, int height, char* mTitle, CMainLoopInterface *mMainLoopObject, bool fullScreen, int refreshRate);
int dkiInit(SDL_Window* appHandle);
int dkglCreateContext(SDL_GLContext mDC);
bool dksInit(int mixrate, int maxsoftwarechannels);

void dkglShutDown();
void dkiShutDown();
void dkoShutDown();
void dktShutDown();
void dkpShutDown();
void dkwShutDown();
void dksShutDown();
void dkfShutDown();

SDL_GLContext dkwGetDC();
SDL_Window* dkwGetHandle();

void dkwSwap()
{
    SDL_GL_SwapWindow(dkwGetHandle());
}

dkGfxContext* dkGfxInit(dkContext* ctx, dkGfxConfig config)
{
    dkGfxContext* gtx = new dkGfxContext;

    if(dkwInit(config.width, config.height, config.title, config.mMainLoopObject, config.fullScreen, config.refreshRate))
    {
        if(dkiInit(dkwGetHandle()))
        {
            if(dkglCreateContext(dkwGetDC()))
            {
                // On init les textures
                dktInit();

                // On init les objets 3D
                dkoInit();

                // On init les particles
                dkpInit();
                return gtx;
            }
            dkiShutDown();
        }
        dkwShutDown();
    }
    return nullptr;
}

void dkGfxFini(dkGfxContext* gtx)
{
    dksShutDown();
    dkpShutDown();
    dkoShutDown();
    dkfShutDown();
    dktShutDown();
    dkglShutDown();
    dkiShutDown();
    dkwShutDown();
    delete gtx;
}
