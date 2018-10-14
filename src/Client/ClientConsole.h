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
#ifndef CLIENT_CONSOLE_H
#define CLIENT_CONSOLE_H

#include <Console.h>

#include "Writting.h"
#define CONSOLE_MAX_RECOGNITION_VAR 10

struct ClientConsole: public Console
{
    // Sa font
    unsigned int m_font;

    // Si la console est active ou non
    bool m_isActive;

    // Sa position verticale
    float m_vPos;

    // Le string courrant, qu'on est entrein de taper
    Writting * m_currentText;

    // Pour tenir les 10 premier trouvêdans les var recognition
    char ** recognitionVar;
    bool showRecognitionVar;
    CString lastRecognitionVar;
    int curRecognitionVar;
    CString m_lastToken;

    // Pour pas qu'on utilise la console (meton qu'on est apres chatter
    bool locked;

    ClientConsole();
    virtual ~ClientConsole();

    // Pour ajouter une ligne
    void add(CString message, bool fromServer=false, bool isEvent=true);

    // Pour updater la console (meton taper dedans, etc)
    void update(float delay);

    // Pour initialiser la console
    void init();

    // Pour envoyer une commande ala console
    void sendCommand(CString commandLine, bool admin=false, unsigned long bbnetID = -1);

    // Pour savoir si la console est active ou non, si cest le cas, on block les inputs ailleur
    bool isActive() {return m_isActive;}

    // Pour la locker
    void lock() {locked = true;}
    void unlock() {locked = false;}
};

#endif

