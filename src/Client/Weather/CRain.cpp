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
#include "CRain.h"
#include "Map.h"
#include "ClientMap.h"
#include <glad/glad.h>

SRain::SRain() {}

void SRain::update(float delay)
{
    pos[2] -= 15 * delay;
}

void SRain::render()
{
    glColor4f(.25f, .7f, .3f,((pos[2] > 2)?2:pos[2]) / 2.0f * .3f);
    glVertex3fv(pos.s);
    glVertex3f(pos[0],pos[1],pos[2]-.5f);
}

//
//--- Constructor
//
CRain::CRain()
{
    m_sfxRain = dksCreateSoundFromFile("main/sounds/rain2.wav", true);

    //--- Start the sound
    channel = dksPlaySound(m_sfxRain, -1, 50);

    nextRain = 0;
}



//
//--- Destructor
//
CRain::~CRain()
{
    FSOUND_StopSound(channel);
    dksDeleteSound(m_sfxRain);
}



//
//--- Update
//
void CRain::update(float delay, Map* map)
{
    auto cmap = static_cast<ClientMap*>(map);
    int i;
    //--- On crée la pluit yé
    for(i = 0; i < 3; ++i)
    {
        rains[nextRain].pos = rand(cmap->camPos + CVector3f(-3, -3, 5), cmap->camPos + CVector3f(3, 3, 5));
        nextRain++;
        if(nextRain == 100) nextRain = 0;
    }

    //--- On anime la plus
    for(i = 0; i < 100; ++i)
    {
        if(rains[i].pos[2] > 0)
        {
            rains[i].update(delay);
        }
    }
}



//
//--- Render
//
void CRain::render()
{
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2);
    glBegin(GL_LINES);
    for(int i = 0; i < 100; ++i)
    {
        if(rains[i].pos[2] > 0)
        {
            rains[i].render();
        }
    }
    glEnd();
    glPopAttrib();
}

