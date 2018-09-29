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

#include <Zeven/Core.h>
#include <SDL2/SDL.h>

struct dkContext
{
    struct dkc
    {
        int      framePerSecond;
        uint64_t frequency;
        float    perSecond;
        int32_t  frame;
        float    fps;

        uint64_t lastFrameCount;
        float    elapsedf;
        float    currentFrameDelay;
        int      oneSecondFrameCound;
        float    oneSecondElapsedcpt;
    } time;
};

void dkcInit(dkContext* ctx, dkConfig config)
{
    ctx->time.framePerSecond = config.framePerSecond;
    ctx->time.frequency = SDL_GetPerformanceFrequency();
    ctx->time.perSecond = 1.0f / (float)(ctx->time.framePerSecond);
    ctx->time.frame = 0;
    ctx->time.fps = 0;

    ctx->time.lastFrameCount = 0;
    ctx->time.elapsedf = 0;
    ctx->time.currentFrameDelay = 0;
    ctx->time.oneSecondFrameCound = 0;
    ctx->time.oneSecondElapsedcpt = 0;
}

float dkcGetElapsedf(dkContext* ctx)
{
    return ctx->time.perSecond;
}

float dkcGetFPS(dkContext* ctx)
{
    return ctx->time.fps;
}

int32_t dkcGetFrame(dkContext* ctx)
{
    return ctx->time.frame;
}

void dkcJumpToFrame(dkContext* ctx, int frame)
{
    ctx->time.frame = frame;
    ctx->time.currentFrameDelay = 0;
    ctx->time.lastFrameCount=0;
    ctx->time.elapsedf=0;
    ctx->time.currentFrameDelay=0;
    ctx->time.fps = 0;
    ctx->time.oneSecondFrameCound = 0;
    ctx->time.oneSecondElapsedcpt = 0;
}

int32_t dkcUpdateTimer(dkContext* ctx)
{
    // On prend le nombre de tick du CPU
    uint64_t lGetTickCount = SDL_GetPerformanceCounter();

    // On update le timer
    double elapsedd = (double)((ctx->time.lastFrameCount) ? lGetTickCount - ctx->time.lastFrameCount : 0) / (double)ctx->time.frequency;
    ctx->time.lastFrameCount = lGetTickCount;
    ctx->time.elapsedf = (float)elapsedd;
    ctx->time.currentFrameDelay += ctx->time.elapsedf;
    int32_t nbFrameAdded=0;
    while (ctx->time.currentFrameDelay >= ctx->time.perSecond)
    {
        ctx->time.currentFrameDelay -= ctx->time.perSecond;
        ctx->time.frame++;
        nbFrameAdded++;
    }

    // On update pour le fps
    ctx->time.oneSecondElapsedcpt += ctx->time.elapsedf;
    ctx->time.oneSecondFrameCound++;
    while (ctx->time.oneSecondElapsedcpt >= 1)
    {
        ctx->time.oneSecondElapsedcpt -= 1;
        ctx->time.fps = (float)ctx->time.oneSecondFrameCound;
        ctx->time.oneSecondFrameCound = 0;
    }

    // On retourne le nombre de frame �animer
    return nbFrameAdded;
}

void dkcSleep(dkContext* ctx, int32_t ms)
{
    ZEVEN_UNUSED(ctx);
    SDL_Delay(ms);
}

dkContext* dkInit(dkConfig config)
{
    dkContext* ctx = new dkContext;

    dkcInit(ctx, config);

    return ctx;
}

void dkFini(dkContext* ctx)
{
    delete ctx;
}
