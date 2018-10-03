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
#include "CSnow.h"
#include "Map.h"
#include "ClientMap.h"
#include <glad/glad.h>

SSnow::SSnow()
{
}
void SSnow::update(float delay)
{
    pos[2] -= 2 * delay;
    pos += rand(CVector3f(-1,-1,0), CVector3f(1,1,0)) * delay;
}
void SSnow::render()
{
    glColor4f(1, 1, 1,((pos[2] > 2)?2:pos[2]) / 2.0f);
    glTexCoord2f(0,1);
    glVertex3f(pos[0]-.05f,pos[1]+.05f,pos[2]);
    glTexCoord2f(0,0);
    glVertex3f(pos[0]-.05f,pos[1]-.05f,pos[2]);
    glTexCoord2f(1,0);
    glVertex3f(pos[0]+.05f,pos[1]-.05f,pos[2]);
    glTexCoord2f(1,1);
    glVertex3f(pos[0]+.05f,pos[1]+.05f,pos[2]);
}
//
//--- Constructor
//
CSnow::CSnow()
{
    m_sfxRain = dksCreateSoundFromFile("main/sounds/wind.wav", true);
    tex_snow = dktCreateTextureFromFile("main/textures/snowflake.png", DKT_FILTER_LINEAR);

    //--- Start the sound
    channel = dksPlaySound(m_sfxRain, -1, 50);

    nextRain = 0;

    nextIn = 0;
}



//
//--- Destructor
//
CSnow::~CSnow()
{
    FSOUND_StopSound(channel);
    dksDeleteSound(m_sfxRain);
    dktDeleteTexture(&tex_snow);
}



//
//--- Update
//
void CSnow::update(float delay, Map* map)
{
    auto cmap = static_cast<ClientMap*>(map);
    --nextIn;
    //--- On crée la neige yé
    if (nextIn <= 0)
    {
        nextIn = 3;
        for (int i=0;i<1;++i)
        {
            rains[nextRain].pos = rand(cmap->camPos + CVector3f(-3,-3,-2), cmap->camPos + CVector3f(3,3,-2));
            nextRain++;
            if (nextRain == 100) nextRain = 0;
        }
    }

    //--- On anime la plus
    for (int i=0;i<100;++i)
    {
        if (rains[i].pos[2] > 0)
        {
            rains[i].update(delay);
        }
    }
}



//
//--- Render
//
void CSnow::render()
{
    glPushAttrib(GL_ENABLE_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, tex_snow);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
            for (int i=0;i<100;++i)
            {
                if (rains[i].pos[2] > 0)
                {
                    rains[i].render();
                }
            }
        glEnd();
    glPopAttrib();
}
