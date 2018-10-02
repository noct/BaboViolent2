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

#include "Map.h"
#include "Helper.h"
#include <Zeven/FileIO.h>
#include "Console.h"
#include "Game.h"
#include "Scene.h"

extern Scene * scene;

//
// Constructeur
//
Map::Map(CString mapFilename, Game * _game, unsigned int font, bool editor, int sizeX, int sizeY)
{
    int i, j, gtnum;
    //-- On print le loading screen! (new)
    // On clear les buffers, on init la camera, etc
    game = _game;

    if(game)
    {
        isServer = game->isServerGame;
    }
    else
    {
        isServer = false;
    }

    if(mapFilename.len() > 15) mapFilename.resize(15);

    isValid = true;
    mapName = mapFilename;

    // Les flags
    flagState[0] = -2;
    flagState[1] = -2;

    // On essaye d'abords de lire la map
    CString fullName = CString("main/maps/") + mapName + ".bvm";

    FileIO file(fullName, "rb");

    if(!file.isValid())
    {
        console->add("\x4> Map doesnt exist");
        isValid = false;
        return;
    }
    else
    {
        CString mapStr = "\x3> Map file Exist :";
        mapStr += mapName;
        console->add(mapStr);
        // On le load ici
        unsigned long mapVersion = file.getULong();

        switch(mapVersion)
        {
        case 10010:
        {
            size[0] = file.getInt();
            size[1] = file.getInt();
            cells = new map_cell[size[0] * size[1]];
            for(j = 0; j < size[1]; ++j)
            {
                for(i = 0; i < size[0]; ++i)
                {
                    unsigned char data = file.getUByte();
                    cells[j*size[0] + i].passable = (data & 128) ? true : false;
                    cells[j*size[0] + i].height = (data & 127);
                    data = file.getUByte();
                    setTileDirt(i, j, ((float)data) / 255.0f);
                }
            }
            break;
        }
        case 10011:
        {
            size[0] = file.getInt();
            size[1] = file.getInt();
            cells = new map_cell[size[0] * size[1]];
            for(j = 0; j < size[1]; ++j)
            {
                for(i = 0; i < size[0]; ++i)
                {
                    unsigned char data = file.getUByte();
                    cells[j*size[0] + i].passable = (data & 128) ? true : false;
                    cells[j*size[0] + i].height = (data & 127);
                    data = file.getUByte();
                    setTileDirt(i, j, ((float)data) / 255.0f);
                }
            }

            // Les flag
            flagPodPos[0] = file.getVector3f();
            flagPodPos[1] = file.getVector3f();

            // Les ojectifs
            file.getVector3f();
            file.getVector3f();

            // Les spawn point
            int nbSpawn = file.getInt();
            for(i = 0; i < nbSpawn; ++i)
            {
                dm_spawns.push_back(file.getVector3f());
            }
            nbSpawn = file.getInt();
            for(i = 0; i < nbSpawn; ++i)
            {
                blue_spawns.push_back(file.getVector3f());
            }
            nbSpawn = file.getInt();
            for(i = 0; i < nbSpawn; ++i)
            {
                red_spawns.push_back(file.getVector3f());
            }
            break;
        }
        case 20201:
        {
            int theme = file.getInt();
            int weather = file.getInt();
            size[0] = file.getInt();
            size[1] = file.getInt();
            cells = new map_cell[size[0] * size[1]];
            for(j = 0; j < size[1]; ++j)
            {
                for(i = 0; i < size[0]; ++i)
                {
                    unsigned char data = file.getUByte();
                    cells[j*size[0] + i].passable = (data & 128) ? true : false;
                    cells[j*size[0] + i].height = (data & 127);
                    data = file.getUByte();
                    setTileDirt(i, j, ((float)data) / 255.0f);
                }
            }

            // Les flag
            flagPodPos[0] = file.getVector3f();
            flagPodPos[1] = file.getVector3f();

            // Les ojectifs
            file.getVector3f();
            file.getVector3f();

            // Les spawn point
            int nbSpawn = file.getInt();
            for(i = 0; i < nbSpawn; ++i)
            {
                dm_spawns.push_back(file.getVector3f());
            }
            nbSpawn = file.getInt();
            for(i = 0; i < nbSpawn; ++i)
            {
                blue_spawns.push_back(file.getVector3f());
            }
            nbSpawn = file.getInt();
            for(i = 0; i < nbSpawn; ++i)
            {
                red_spawns.push_back(file.getVector3f());
            }
            break;
        }
        case 20202:
        {
            // Common map data
            char * author_name_buffer = file.getByteArray(25);
            int theme = file.getInt();
            int weather = file.getInt();
            delete[] author_name_buffer;
            author_name_buffer = 0;

            // for gcc compliant code

            int i = 0;

            size[0] = file.getInt();
            size[1] = file.getInt();
            cells = new map_cell[size[0] * size[1]];
            for(j = 0; j < size[1]; ++j)
            {
                for(i = 0; i < size[0]; ++i)
                {
                    unsigned char data = file.getUByte();
                    cells[j*size[0] + i].passable = (data & 128) ? true : false;
                    cells[j*size[0] + i].height = (data & 127);
                    data = file.getUByte();
                    setTileDirt(i, j, ((float)data) / 255.0f);
                }
            }
            // common spawns
            int nbSpawn = file.getInt();
            for(i = 0; i < nbSpawn; ++i)
            {
                dm_spawns.push_back(file.getVector3f());
            }

            // read game-type specific data
            // there must always be one game-type specific section per one supported game type
            for(gtnum = 0; gtnum < GAME_TYPE_COUNT; ++gtnum)
            {
                int id = file.getInt();
                switch(id)
                {
                case GAME_TYPE_DM:
                case GAME_TYPE_TDM:
                    break; // nothing to do for DM and TDM
                case GAME_TYPE_CTF:
                {
                    // flags
                    flagPodPos[0] = file.getVector3f();
                    flagPodPos[1] = file.getVector3f();
                }
                break;
                default:
                    break;
                }
            }
            break;
        }
        }
    }

    if(isServer)
    {
        return;
    }

    //--- Reset timer
    dkcJumpToFrame(scene->ctx, 0);
}

//
// Destructeur
//
Map::~Map()
{
    if(isValid)
    {
        ZEVEN_SAFE_DELETE_ARRAY(cells);
    }

    dm_spawns.clear();
    blue_spawns.clear();
    red_spawns.clear();
}

#define TEST_DIR_X 0
#define TEST_DIR_X_NEG 1
#define TEST_DIR_Y 2
#define TEST_DIR_Y_NEG 3

//
// Pour faire un ray tracing
//
bool Map::rayTest(CVector3f & p1, CVector3f & p2, CVector3f & normal)
{
    // On pogne notre cell de départ
    int i = (int)p1[0];
    int j = (int)p1[1];

    // On check que notre tuile n'est pas déjà occupé
    if(i >= 0 &&
        i < size[0] &&
        j >= 0 &&
        j < size[1])
    {
        if(!cells[j*size[0] + i].passable && p1[2] < cells[j*size[0] + i].height)
        {
            p2 = p1;
            return true;
        }
    }
    else
    {
        return false;
    }


    // On défini dans quel sens on va voyager (4 sens)
    int sens;
    if(fabsf(p2[0] - p1[0]) > fabsf(p2[1] - p1[1]))
    {
        if(p2[0] > p1[0])
        {
            sens = TEST_DIR_X;
        }
        else
        {
            sens = TEST_DIR_X_NEG;
        }
    }
    else
    {
        if(p2[1] > p1[1])
        {
            sens = TEST_DIR_Y;
        }
        else
        {
            sens = TEST_DIR_Y_NEG;
        }
    }

    // On while tant qu'on ne l'a pas trouvé
    float percent;
    while(true)
    {
        // On check qu'on n'a pas dépassé
        if(i < 0 ||
            i >= size[0] ||
            j < 0 ||
            j >= size[1] ||
            (sens == TEST_DIR_X && i > (int)p2[0]) ||
            (sens == TEST_DIR_X_NEG && i < (int)p2[0]) ||
            (sens == TEST_DIR_Y && j > (int)p2[1]) ||
            (sens == TEST_DIR_Y_NEG && j < (int)p2[1]))
        {
            return false;
        }

        switch(sens)
        {
        case TEST_DIR_X:
            // On test nos 3 tuiles
            if(rayTileTest(i, j, p1, p2, normal)) return true;
            if(rayTileTest(i, j - 1, p1, p2, normal)) return true;
            if(rayTileTest(i, j + 1, p1, p2, normal)) return true;

            // On incrémente à la prochaine tuile
            i++;
            percent = ((float)i - p1[0]) / fabsf(p2[0] - p1[0]);
            j = (int)(p1[1] + (p2[1] - p1[1]) * percent);
            break;

        case TEST_DIR_X_NEG:
            // On test nos 3 tuiles
            if(rayTileTest(i, j, p1, p2, normal)) return true;
            if(rayTileTest(i, j - 1, p1, p2, normal)) return true;
            if(rayTileTest(i, j + 1, p1, p2, normal)) return true;

            // On incrémente à la prochaine tuile
            i--;
            percent = (p1[0] - (float)(i + 1)) / fabsf(p2[0] - p1[0]);
            j = (int)(p1[1] + (p2[1] - p1[1]) * percent);
            break;

        case TEST_DIR_Y:
            // On test nos 3 tuiles
            if(rayTileTest(i, j, p1, p2, normal)) return true;
            if(rayTileTest(i - 1, j, p1, p2, normal)) return true;
            if(rayTileTest(i + 1, j, p1, p2, normal)) return true;

            // On incrémente à la prochaine tuile
            j++;
            percent = ((float)j - p1[1]) / fabsf(p2[1] - p1[1]);
            i = (int)(p1[0] + (p2[0] - p1[0]) * percent);
            break;

        case TEST_DIR_Y_NEG:
            // On test nos 3 tuiles
            if(rayTileTest(i, j, p1, p2, normal)) return true;
            if(rayTileTest(i - 1, j, p1, p2, normal)) return true;
            if(rayTileTest(i + 1, j, p1, p2, normal)) return true;

            // On incrémente à la prochaine tuile
            j--;
            percent = (p1[1] - (float)(j + 1)) / fabsf(p2[1] - p1[1]);
            i = (int)(p1[0] + (p2[0] - p1[0]) * percent);
            break;
        }
    }

    return false;
}

bool IsMapValid(const Map & map, int gameType)
{
    bool isGoodMap = true;
    //--- game type specific map-check
    switch(gameType)
    {
    case GAME_TYPE_DM:
    case GAME_TYPE_TDM:
        // there must be at least 1 item in dm_spawns
        isGoodMap = (map.dm_spawns.size() >= 1);
        break;
    case GAME_TYPE_CTF:
        // there must be at least 1 item in dm_spawns,
        // both flags must be set
        isGoodMap = (map.dm_spawns.size() >= 1)
            && (map.flagPodPos[0] != CVector3f(0.0f, 0.0f, 0.0f))
            && (map.flagPodPos[1] != CVector3f(0.0f, 0.0f, 0.0f));
        break;
    }
    return isGoodMap;
}

// Pour ajouter du dirt
void Map::setTileDirt(int x, int y, float value)
{
    if (x < 0) return;
    if (y < 0) return;
    if (x >= size[0]) return;
    if (y >= size[1]) return;

    cells[y*size[0]+x].splater[1] = value;

    // Ses voisins qui sharent ce vertex
    if (x > 0)
    {
        cells[y*size[0]+x-1].splater[2] = cells[y*size[0]+x].splater[1];
        if (y > 0)
        {
            cells[(y-1)*size[0]+x-1].splater[3] = cells[y*size[0]+x].splater[1];
        }
    }
    if (y > 0)
    {
        cells[(y-1)*size[0]+x].splater[0] = cells[y*size[0]+x].splater[1];
    }
}

void Map::addTileDirt(int x, int y, float value)
{
    if (x < 0) return;
    if (y < 0) return;
    if (x >= size[0]) return;
    if (y >= size[1]) return;

    cells[y*size[0]+x].splater[1] += value;
    if (cells[y*size[0]+x].splater[1] > 1) cells[y*size[0]+x].splater[1] = 1;

    // Ses voisins qui sharent ce vertex
    if (x > 0)
    {
        cells[y*size[0]+x-1].splater[2] = cells[y*size[0]+x].splater[1];
        if (y > 0)
        {
            cells[(y-1)*size[0]+x-1].splater[3] = cells[y*size[0]+x].splater[1];
        }
    }
    if (y > 0)
    {
        cells[(y-1)*size[0]+x].splater[0] = cells[y*size[0]+x].splater[1];
    }
}

void Map::removeTileDirt(int x, int y, float value)
{
    if (x < 0) return;
    if (y < 0) return;
    if (x >= size[0]) return;
    if (y >= size[1]) return;

    cells[y*size[0]+x].splater[1] -= value;
    if (cells[y*size[0]+x].splater[1] < 0) cells[y*size[0]+x].splater[1] = 0;

    // Ses voisins qui sharent ce vertex
    if (x > 0)
    {
        cells[y*size[0]+x-1].splater[2] = cells[y*size[0]+x].splater[1];
        if (y > 0)
        {
            cells[(y-1)*size[0]+x-1].splater[3] = cells[y*size[0]+x].splater[1];
        }
    }
    if (y > 0)
    {
        cells[(y-1)*size[0]+x].splater[0] = cells[y*size[0]+x].splater[1];
    }
}

// Pour tester une tuile (inline celle là)
bool Map::rayTileTest(int x, int y, CVector3f & p1, CVector3f & p2, CVector3f & normal)
{
    if (x>=0 && x<size[0] && y>=0 && y<size[1])
    {
        float x1 = (float)x;
        float x2 = (float)x+1;
        float y1 = (float)y;
        float y2 = (float)y+1;
        float percent;
        float height = (float)cells[y*size[0]+x].height;
        CVector3f p;

        if (cells[y*size[0]+x].passable)
        {
            // On check juste si on pogne le plancher !
            if (p1[2] > 0 && p2[2] <= 0)
            {
                percent = p1[2] / fabsf(p2[2] - p1[2]);
                p = p1 + (p2 - p1) * percent;
                if (p[0] >= x1 && p[0] <= x2 &&
                    p[1] >= y1 && p[1] <= y2)
                {
                    p2 = p;
                    normal.set(0,0,1);
                    return true;
                }
                return false;
            }
            else
            {
                return false;
            }
        }

        if (!cells[y*size[0]+x].passable)
        {
            // On check si on pogne le plafond
            if (p1[2] > height && p2[2] <= height)
            {
                percent = (p1[2]-height) / fabsf((p2[2]-height) - (p1[2]-height));
                p = p1 + (p2 - p1) * percent;
                if (p[0] >= x1 && p[0] <= x2 &&
                    p[1] >= y1 && p[1] <= y2)
                {
                    p2 = p;
                    normal.set(0,0,1);
                    return true;
                }
            }
        }

        // Le côté x1 en premier
        if (p1[0] <= x1 && p2[0] > x1)
        {
            percent = fabsf(x1 - p1[0]) / fabsf(p2[0] - p1[0]);
            p = p1 + (p2 - p1) * percent;
            if (p[1] <= y2 && p[1] >= y1 && p[2] < height)
            {
                p2 = p;
                normal.set(-1,0,0);
                return true;
            }
        }

        // Le côté oposé
        if (p1[0] >= x2 && p2[0] < x2)
        {
            percent = fabsf(p1[0] - x2) / fabsf(p2[0] - p1[0]);
            p = p1 + (p2 - p1) * percent;
            if (p[1] <= y2 && p[1] >= y1 && p[2] < height)
            {
                p2 = p;
                normal.set(1,0,0);
                return true;
            }
        }

        // Le côté y1
        if (p1[1] <= y1 && p2[1] > y1)
        {
            percent = fabsf(y1 - p1[1]) / fabsf(p2[1] - p1[1]);
            p = p1 + (p2 - p1) * percent;
            if (p[0] <= x2 && p[0] >= x1 && p[2] < height)
            {
                p2 = p;
                normal.set(0,-1,0);
                return true;
            }
        }

        // Le côté oposé
        if (p1[1] >= y2 && p2[1] < y2)
        {
            percent = fabsf(p1[1] - y2) / fabsf(p2[1] - p1[1]);
            p = p1 + (p2 - p1) * percent;
            if (p[0] <= x2 && p[0] >= x1 && p[2] < height)
            {
                p2 = p;
                normal.set(0,1,0);
                return true;
            }
        }

    }

    return false;
}
