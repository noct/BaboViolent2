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

#ifndef CLIENT_HELPER_H
#define CLIENT_HELPER_H

#include <Zeven/Core.h>

// Pour �rire du texte centr�
void printCenterText(float x, float y, float size, const CString & text);
void printLeftText(float x, float y, float size, const CString & text);
void printRightText(float x, float y, float size, const CString & text);

// Pour afficher un quad truc simple de meme
void renderTexturedQuad(int x, int y, int w, int h, unsigned int texture);
void renderTexturedQuadSmooth(int x, int y, int w, int h, unsigned int texture);
//void renderMenuQuad(int x, int y, int w, int h);
void renderMenuQuad(int x, int y, int w, int h);

#endif
