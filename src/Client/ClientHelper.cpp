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

#include "Helper.h"
#include "ClientHelper.h"
#include <Zeven/Gfx.h>
#include <GameVar.h>
#include <glad/glad.h>
bool enableShadow = true;

//
// Pour �rire du texte centr�
//
void printCenterText(float x, float y, float size, const CString & text)
{
    float width = dkfGetStringWidth(size, text.s);

    if ((enableShadow) && (gameVar.r_simpleText != true))
    {
        int shadowDis = (size > 32) ? 2 : 1;
        float curColor[4];

        glGetFloatv(GL_CURRENT_COLOR, curColor);
        glPushAttrib(GL_CURRENT_BIT);
            glColor4f(0,0,0, curColor[3]);
            dkfPrint(size,x-width/2+shadowDis,y+shadowDis,0,textColorLess(text).s);
        glPopAttrib();
        glColor4fv(curColor);
    }
    dkfPrint(size,x-width/2,y,0,text.s);
}

void printLeftText(float x, float y, float size, const CString & text)
{
    if ((enableShadow) && (gameVar.r_simpleText != true))
    {
        int shadowDis = (size > 32) ? 2 : 1;
        float curColor[4];

        glGetFloatv(GL_CURRENT_COLOR, curColor);
        glPushAttrib(GL_CURRENT_BIT);
            glColor4f(0,0,0, curColor[3]);
            dkfPrint(size,x/*+shadowDis*/,y+1,0,textColorLess(text).s);
        glPopAttrib();
        glColor4fv(curColor);
    }
    dkfPrint(size,x,y,0,text.s);
}

void printRightText(float x, float y, float size, const CString & text)
{

    float width = dkfGetStringWidth(size, text.s);


    if ((enableShadow) && (gameVar.r_simpleText != true))
    {
        float curColor[4];
        int shadowDis = (size > 32) ? 2 : 1;

        glGetFloatv(GL_CURRENT_COLOR, curColor);
        glPushAttrib(GL_CURRENT_BIT);
            glColor4f(0,0,0, curColor[3]);
            dkfPrint(size,x-width+shadowDis,y+shadowDis,0,textColorLess(text).s);
        glPopAttrib();
        glColor4fv(curColor);
    }

    dkfPrint(size,x-width,y,0,text.s);
}

//
// Pour afficher un quad truc simple de meme
//
void renderTexturedQuad(int x, int y, int w, int h, unsigned int texture)
{
    glPushAttrib(GL_ENABLE_BIT);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBegin(GL_QUADS);
            glTexCoord2i(0,1);
            glVertex2i(x,y);
            glTexCoord2i(0,0);
            glVertex2i(x,y+h);
            glTexCoord2i(1,0);
            glVertex2i(x+w,y+h);
            glTexCoord2i(1,1);
            glVertex2i(x+w,y);
        glEnd();
    glPopAttrib();
}

void renderTexturedQuadSmooth(int x, int y, int w, int h, unsigned int texture)
{
    float curColor[4];
    glGetFloatv(GL_CURRENT_COLOR, curColor);

    glPushAttrib(GL_ENABLE_BIT);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBegin(GL_QUAD_STRIP);
            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,1);
            glVertex2i(x,y);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,1);
            glVertex2i(x,y+10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,1);
            glVertex2i(x+10,y);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(0,1);
            glVertex2i(x+10,y+10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,1);
            glVertex2i(x+w-10,y);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(1,1);
            glVertex2i(x+w-10,y+10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,1);
            glVertex2i(x+w,y);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,1);
            glVertex2i(x+w,y+10);
        glEnd();
        glBegin(GL_QUAD_STRIP);
            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,1);
            glVertex2i(x,y+10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,0);
            glVertex2i(x,y+h-10);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(0,1);
            glVertex2i(x+10,y+10);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(0,0);
            glVertex2i(x+10,y+h-10);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(1,1);
            glVertex2i(x+w-10,y+10);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(1,0);
            glVertex2i(x+w-10,y+h-10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,1);
            glVertex2i(x+w,y+10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,0);
            glVertex2i(x+w,y+h-10);
        glEnd();
        glBegin(GL_QUAD_STRIP);
            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,0);
            glVertex2i(x,y+h-10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,0);
            glVertex2i(x,y+h);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(0,0);
            glVertex2i(x+10,y+h-10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(0,0);
            glVertex2i(x+10,y+h);

            glColor4f(curColor[0],curColor[1],curColor[2],1);
            glTexCoord2i(1,0);
            glVertex2i(x+w-10,y+h-10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,0);
            glVertex2i(x+w-10,y+h);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,0);
            glVertex2i(x+w,y+h-10);

            glColor4f(curColor[0],curColor[1],curColor[2],0);
            glTexCoord2i(1,0);
            glVertex2i(x+w,y+h);
        glEnd();
    glPopAttrib();
}

void renderMenuQuad(int x, int y, int w, int h)
{
    glPushAttrib(GL_CURRENT_BIT);

        //--- Round corner of 5 units
        CVector4f color4;
        glGetFloatv(GL_CURRENT_COLOR, color4.s);

        if (gameVar.r_highDetailMenu)
        {
            glBegin(GL_QUADS);
                glColor4f(0, 0, 0, .25f);

                glVertex2i(x - 1, y);
                glVertex2i(x - 1, y + h);
                glVertex2i(x + w, y + h);
                glVertex2i(x + w, y);

                glVertex2i(x, y - 1);
                glVertex2i(x, y + h);
                glVertex2i(x + w, y + h);
                glVertex2i(x + w, y - 1);

                glVertex2i(x, y);
                glVertex2i(x, y + h + 1);
                glVertex2i(x + w, y + h + 1);
                glVertex2i(x + w, y);

                glVertex2i(x, y);
                glVertex2i(x, y + h);
                glVertex2i(x + w + 1, y + h);
                glVertex2i(x + w + 1, y);
            glEnd();

            glBegin(GL_QUADS);
                glColor4fv((color4*1.2f).s);
                glVertex2i(x,y);
                glColor4fv((color4 * 1).s);
                glVertex2i(x,y+h / 2);
                glColor4fv((color4 * 1).s);
                glVertex2i(x+w,y+h / 2);
                glColor4fv((color4*1.2f).s);
                glVertex2i(x+w,y);

                glColor4fv((color4*.9f).s);
                glVertex2i(x,y + h / 2);
                glColor4fv((color4*0.6f).s);
                glVertex2i(x,y+h);
                glColor4fv((color4*0.6f).s);
                glVertex2i(x+w,y+h);
                glColor4fv((color4*.9f).s);
                glVertex2i(x+w,y + h / 2);
            glEnd();
        }
        else
        {
            glBegin(GL_QUADS);
                glColor4fv((color4*1.2f).s);
                glVertex2i(x,y);
                glColor4fv((color4 * 0.6f).s);
                glVertex2i(x,y+h);
                glColor4fv((color4 * 0.6f).s);
                glVertex2i(x+w,y+h);
                glColor4fv((color4*1.2f).s);
                glVertex2i(x+w,y);
            glEnd();
        }
    glPopAttrib();
}
