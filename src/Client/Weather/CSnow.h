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
#ifndef CSNOW_H_INCLUDED
#define CSNOW_H_INCLUDED

#include "CWeather.h"
#include <Zeven/Gfx.h>

struct SSnow
{
public:
    CVector3f pos;
public:
    SSnow();
    void update(float delay);
    void render();
};


class CSnow : public CWeather
{
public:
    //--- Weather sound
    FSOUND_SAMPLE * m_sfxRain;
    int channel;

    //--- La rain
    SSnow rains[100];
    int nextRain;

    //--- Flocon
    unsigned int tex_snow;

    int nextIn;

public:
    //--- Constructor
    CSnow();

    //--- Destructor
    virtual ~CSnow();

    //--- Update
    void update(float delay, Map* map);

    //--- Render
    void render();
};

#endif
