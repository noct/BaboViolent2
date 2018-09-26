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


#ifndef DKFI_H
#define DKFI_H

#include "CFont.h"
#include <vector>
#include <Zeven/CVector.h>

// Les fonction du DKT
void            dkfBindFont(unsigned int ID);
unsigned int    dkfCreateFont(char *filename);
void            dkfDeleteFont(unsigned int *ID);
CPoint2f        dkfGetCaracterPos(float size, char *text, int caracter);
int             dkfGetOverStringCaracter(float size, char *text, CPoint2f & onStringPos);
float           dkfGetStringHeight(float size, char *text);
float           dkfGetStringWidth(float size, char *text);
void            dkfPrint(float size, float x, float y, float z, char *text);
void            dkfShutDown();




//
// La liste global des font
//
std::vector<CFont*> fonts;
unsigned int currentIDCount = 0;
CFont *currentBind = 0;



#endif
