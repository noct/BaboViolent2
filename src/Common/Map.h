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

#ifndef MAP_H
#define MAP_H

#include <Zeven/Core.h>
#include <vector>
#include "GameVar.h"
#include "Player.h"

#define COLLISION_EPSILON 0.05f
#define BOUNCE_FACTOR 0.45f

#define MAP_VERSION 20202

#define FLAG_BLUE 0
#define FLAG_RED 1

struct Game;
struct mapInfo
{
    CString mapName;
    int mapArea;
    int lastPlayed;
};

struct map_cell
{
    // La liste d'objet que cette cellule contient
    bool passable;

    // Notre splater
    float splater[4];

    // Sa hauteur
    int height;

    map_cell()
    {
        splater[0] = 0;
        splater[1] = 0;
        splater[2] = 0;
        splater[3] = 0;
        passable = true;
        height = 1; // Par default un wall est 1 de haut
    }
};

struct Map
{
    // Sa grosseur
    CVector2i size;

    // Ses cells
    map_cell * cells;
    CString mapName;

    bool isValid;
    bool isServer;

    // Les deux flag point
    CVector3f flagPodPos[2];

    CVector3f flagPos[2];
    CVector3f flagLastPos[2];
    CVector3f flagLastAccel[2];
    CVector3f flagBendOffset[2];
    char flagState[2]; // -1 = par terre, -2 = on pod, 0+ on player

    // Les spawn point
    std::vector<CVector3f> dm_spawns;
    std::vector<CVector3f> blue_spawns;
    std::vector<CVector3f> red_spawns;

    // Constructeur
    Map() {}
    Map(CString mapFilename, bool isServerGame, unsigned int font, bool editor=false, int sizeX=32, int sizeY=32);

    // Destructeur
    virtual ~Map();

    // Pour faire un ray tracing
    bool rayTest(CVector3f & p1, CVector3f & p2, CVector3f & normal);

    // Pour ajouter du dirt
    void setTileDirt(int x, int y, float value);
    void addTileDirt(int x, int y, float value);
    void removeTileDirt(int x, int y, float value);
    bool rayTileTest(int x, int y, CVector3f & p1, CVector3f & p2, CVector3f & normal);
};

// Tells if a map has everything that is needed for a given game-type
bool IsMapValid(const Map & map, int gameType);

#endif


