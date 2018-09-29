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

#ifndef KEYMANAGER_H
#define KEYMANAGER_H


#include <Zeven/Gfx.h>


class KeyManager
{
private:
public:
    // Pour obtenir une touche à partir du nom
    int getKeyByName(CString name);

    // Pour obtenir le nom d'une touche
    CString getKeyName(int key);
};


extern KeyManager keyManager;


#endif

