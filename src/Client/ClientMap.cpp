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

#include "ClientMap.h"
#include "Helper.h"
#include <Zeven/FileIO.h>
#include "Console.h"
#include "Game.h"
#include "Scene.h"
#include "CRain.h"
#include "CSnow.h"
#include "CLava.h"
#include "ClientHelper.h"
#include "ClientGame.h"
#include <direct.h>
#include <algorithm>

#ifdef RENDER_LAYER_TOGGLE
int renderToggle = 0;
#endif

extern Scene * scene;

ClientMap::ClientMap(CString mapFilename, Game * _game, unsigned int font, bool editor, int sizeX, int sizeY)
: Map(mapFilename, _game, font, editor, sizeX, sizeY), groundMesh(0), shadowMesh(0), wallMesh(0)
{

    int i, j, gtnum;
    //-- On print le loading screen! (new)
    // On clear les buffers, on init la camera, etc
    cell_dl = nullptr;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CVector2i res = dkwGetResolution();

    glViewport(0, 0, res[0], res[1]);
    dkglSetProjection(60, 1, 50, (float)res[0], (float)res[1]);

    // Truc par default à enabeler
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1, 1, 1);

    dkglPushOrtho(800, 600);

    // Print au millieu
    glColor3f(1, 1, 1);
    dkfBindFont(font);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    printCenterText(400, 268, 64, CString("LOADING"));

    dkglPopOrtho();

    // On swap les buffers
    dkwSwap();
    zoom = 0;
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
    isEditor = editor;
    mapName = mapFilename;
    flagAnim = 0;
    m_weather = 0;

    if(!isServer)
    {
        // Les textures
        tex_grass = dktCreateTextureFromFile("main/textures/grass.tga", DKT_FILTER_BILINEAR);
        tex_dirt = dktCreateTextureFromFile("main/textures/dirt1.tga", DKT_FILTER_BILINEAR);
        tex_wall = dktCreateTextureFromFile("main/textures/dirt2.tga", DKT_FILTER_BILINEAR);

        // Les models
        dko_flag[0] = dkoLoadFile("main/models/BlueFlag.DKO");
        dko_flagPod[0] = dkoLoadFile("main/models/BlueFlagPod.DKO");
        dko_flag[1] = dkoLoadFile("main/models/RedFlag.DKO");
        dko_flagPod[1] = dkoLoadFile("main/models/RedFlagPod.DKO");

        //  dko_flagPole = dkoLoadFile("main/models/flagpole.dko");

        flagAnims[0] = 25;
        flagAnims[1] = 25;
        flagAngle[0] = 0;
        flagAngle[1] = 0;

        theme = THEME_GRASS;
        weather = WEATHER_NONE;
    }

    // Les flags
    flagState[0] = -2;
    flagState[1] = -2;

    // On essaye d'abords de lire la map
    CString fullName = CString("main/maps/") + mapName + ".bvm";

    // Hosts will load from file since they are also the server
    // Ugly code was required to avoid modification of the remaining code
    FileIO  fileObj(fullName, "rb");
    FileIO* fptr = 0;
    if(scene->server || isEditor)
        fptr = &fileObj;
    else
    {
        auto cgame = static_cast<ClientGame*>(_game);
        fptr = &cgame->mapBuffer;
    }
    FileIO& file = *fptr;

    if(!file.isValid())
    {
        console->add("\x4> Map doesnt exist");
        if(editor)
        {
            isValid = true;
            mapName = mapFilename;
            author_name = gameVar.cl_mapAuthorName;

            // On init ici
            size[0] = sizeX;
            size[1] = sizeY;

            cells = new map_cell[size[0] * size[1]];

            // On cré tout suite les contours de la map
            for(j = 0; j < size[1]; ++j)
            {
                cells[j*size[0] + 0].passable = false;
                cells[j*size[0] + 0].height = 3; // Les bords sont plus haut
                cells[j*size[0] + size[0] - 1].passable = false;
                cells[j*size[0] + size[0] - 1].height = 3;
            }
            for(i = 0; i < size[0]; ++i)
            {
                cells[i].passable = false;
                cells[i].height = 3;
                cells[(size[1] - 1)*size[0] + i].passable = false;
                cells[(size[1] - 1)*size[0] + i].height = 3;
            }
        }
        else
        {
            isValid = false;
            return;
        }
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
            // if opening older map file in editor - fill in the author field
            if(editor)
            {
                author_name = gameVar.cl_mapAuthorName;
            }
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
            // if opening older map file in editor - fill in the author field
            if(editor)
            {
                author_name = gameVar.cl_mapAuthorName;
            }
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
            // if opening older map file in editor - fill in the author field
            if(editor)
            {
                author_name = gameVar.cl_mapAuthorName;
            }
            theme = file.getInt();
            weather = file.getInt();
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
            author_name_buffer[24] = '\0';
            author_name.set("%.24s", author_name_buffer);
            // Note: we DO NOT want to overwrite the author field if it's being edited
            theme = file.getInt();
            weather = file.getInt();
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

    cell_dl = new unsigned int[size[0] * size[1]];

    //--- We load now the minimap thumbnail
    {
        // On cré l'espace pour la texture
        texMap = dktCreateEmptyTexture(size[0], size[1], 3, DKT_FILTER_NEAREST);
        regenTex();
    }

    tex_floor = 0;
    tex_floor_dirt = 0;
    tex_wall_bottom = 0;
    tex_wall_center = 0;
    tex_wall_up = 0;
    tex_wall_top = 0;
    tex_wall_both = 0;

    reloadWeather();
    reloadTheme();

    //--- Rebuild the map
    buildAll();

    //--- Position the camera at the center
    setCameraPos(CVector3f((float)size[0] / 2, (float)size[1] / 2, 0));

    //--- Reset timer
    dkcJumpToFrame(scene->ctx, 0);
}


void ClientMap::update(float delay, Player * thisPlayer)
{
#ifdef RENDER_LAYER_TOGGLE
    if(dkiGetState(DIK_F5) == 1)
    {
        renderToggle++;
        if(renderToggle > 16)
        {
            renderToggle = 0;
        }
    }
    if(dkiGetState(DIK_F6) == 1)
    {
        renderToggle = 16;
    }
#endif

    if(m_weather) m_weather->update(delay, this);


    // Snipers should be able to scope at map edges
    if(isEditor || (thisPlayer && thisPlayer->weapon && thisPlayer->weapon->weaponID != WEAPON_SNIPER) || (thisPlayer && thisPlayer->teamID == PLAYER_TEAM_SPECTATOR))
    {
        if(camLookAt[0] < 5) camLookAt[0] = 5;
        if(camLookAt[0] > (float)size[0] - 5) camLookAt[0] = (float)size[0] - 5;
        if(camLookAt[1] < 4) camLookAt[1] = 4;
        if(camLookAt[1] > (float)size[1] - 4) camLookAt[1] = (float)size[1] - 4;
    }

    camDest = camLookAt + CVector3f(0, 0, 7);
    if(thisPlayer)
    {
        if(thisPlayer->status == PLAYER_STATUS_ALIVE)
        {
            if(thisPlayer->weapon)
            {
                if(thisPlayer->weapon->weaponID == WEAPON_SNIPER)
                {
                    //--- Dépendament de la distance entre la mouse et le player
                    float dis = distance(thisPlayer->currentCF.mousePosOnMap, thisPlayer->currentCF.position) * 2;
                    if(dis > 12) dis = 12;
                    if(dis < 5) dis = 5;
                    camDest = camLookAt + CVector3f(0, 0, dis); // On voit de plus loin pour mieux sniper ;)
                }
            }
        }
        else if(thisPlayer->status == PLAYER_STATUS_DEAD)
        {
        }

        if(thisPlayer->teamID == PLAYER_TEAM_SPECTATOR)
        {
            // Update zoom
            int longestSide = (size[0] > size[1]) ? size[0] : size[1];;
            zoom += -dkiGetMouseWheelVel()*0.01f;
            zoom = (zoom < -8) ? -8 : zoom;
            zoom = (zoom > longestSide / 2) ? longestSide / 2 : zoom;

            // Set camera
            camDest = camLookAt + CVector3f(0, 0, 14 + zoom); // On voit de plus loin pour mieux bombarder lol (joke)
        }
    }
    else if(isEditor)
    {
        // Update zoom
        int longestSide = (size[0] > size[1]) ? size[0] : size[1];;
        zoom += -dkiGetMouseWheelVel()*0.01f;
        zoom = (zoom < -8) ? -8 : zoom;
        zoom = (zoom > longestSide / 2) ? longestSide / 2 : zoom;

        // Set camera
        camDest = camLookAt + CVector3f(0, 0, 14 + zoom); // On voit de plus loin pour mieux bombarder lol (joke)
    }
    else
    {
        // Hum, on est dans l'editeur
        camDest = camLookAt + CVector3f(0, 0, 14);
    }
    CVector3f lastCamPos = camPos;
    camPos += (camDest - camPos) * 2.5f * delay;

    // On anim les flag
    flagAnim += delay * 10;
    while(flagAnim >= 10) flagAnim -= 10;

    if(flagState[0] == -2) flagPos[0] = flagPodPos[0];
    if(flagState[1] == -2) flagPos[1] = flagPodPos[1];
    if(flagState[0] >= 0) if(game->players[flagState[0]]) flagPos[0] = game->players[flagState[0]]->currentCF.position;
    if(flagState[1] >= 0) if(game->players[flagState[1]]) flagPos[1] = game->players[flagState[1]]->currentCF.position;
}

void ClientMap::reloadWeather()
{
    ZEVEN_SAFE_DELETE(m_weather);

    fogDensity = 1;
    fogStart = -3;
    fogEnd = -3;
    fogColor.set(1, 1, 1, 1);

    if(!gameVar.r_weatherEffects) {
        weather = WEATHER_NONE;
        fogDensity = 0;
        return;
    }

    fogDensity = 0;

    // Set weather to match theme
    if(theme == THEME_GRAIN)
        weather = WEATHER_FOG;
    if((theme == THEME_SNOW) || (theme == THEME_FROZEN) || (theme == THEME_WINTER))
        weather = WEATHER_SNOW;
    else if((theme == THEME_SAND) || (theme == THEME_STREET))
        weather = WEATHER_SANDSTORM;
    else if((theme == THEME_CITY) || (theme == THEME_RAINY) || (theme == THEME_ROAD))
        weather = WEATHER_RAIN;
    else if((theme == THEME_LAVA) || (theme == THEME_CORE) || (theme == THEME_ROCK))
        weather = WEATHER_LAVA;
    else weather = WEATHER_NONE;

    if(weather == WEATHER_RAIN)
    {
        fogStart = 4;
        fogEnd = -3;
        fogColor.set(.15f, .25f, .25f, 1);
        m_weather = new CRain();
    }
    if(weather == WEATHER_FOG)
    {
        fogStart = 1;
        fogEnd = -.25f;
        fogColor.set(.3f, .4f, .4f, 1);
    }
    if(weather == WEATHER_SNOW)
    {
        fogDensity = 0;
        m_weather = new CSnow();
    }
    if(weather == WEATHER_LAVA)
    {
        fogDensity = 0;
        m_weather = new CLava();
    }
}

void ClientMap::regenCell(int i, int j)
{
    if(i < 0 || j < 0 || i >= size[0] || j >= size[1]) return;
    int pos = j * size[0] + i;
    if(cell_dl[pos]) glDeleteLists(cell_dl[pos], 1);
    cell_dl[pos] = 0;
    if(!cells[pos].passable)
    {
        cell_dl[pos] = glGenLists(1);
        glNewList(cell_dl[pos], GL_COMPILE);
        if(cells[pos].height == 1)
        {
            glBindTexture(GL_TEXTURE_2D, tex_wall_both);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, tex_wall_bottom);
        }

        if(j > 0)
        {
            if(cells[(j - 1)*size[0] + (i)].passable)
            {
                glColor3f(.3f, .3f, .3f);
                glBegin(GL_QUADS);
                glTexCoord2i(0, 1);
                glVertex3i(i, j, 1);
                glTexCoord2i(0, 0);
                glVertex3i(i, j, 0);
                glTexCoord2i(1, 0);
                glVertex3i(i + 1, j, 0);
                glTexCoord2i(1, 1);
                glVertex3i(i + 1, j, 1);
                glEnd();
            }
        }

        if(i < size[0] - 1)
        {
            if(cells[(j)*size[0] + (i + 1)].passable)
            {
                glColor3f(.4f, .4f, .4f);
                glBegin(GL_QUADS);
                glTexCoord2i(0, 1);
                glVertex3i(i + 1, j, 1);
                glTexCoord2i(0, 0);
                glVertex3i(i + 1, j, 0);
                glTexCoord2i(1, 0);
                glVertex3i(i + 1, j + 1, 0);
                glTexCoord2i(1, 1);
                glVertex3i(i + 1, j + 1, 1);
                glEnd();
            }
        }

        if(j < size[1] - 1)
        {
            if(cells[(j + 1)*size[0] + (i)].passable)
            {
                glColor3f(.7f, .7f, .7f);
                glBegin(GL_QUADS);
                glTexCoord2i(0, 1);
                glVertex3i(i + 1, j + 1, 1);
                glTexCoord2i(0, 0);
                glVertex3i(i + 1, j + 1, 0);
                glTexCoord2i(1, 0);
                glVertex3i(i, j + 1, 0);
                glTexCoord2i(1, 1);
                glVertex3i(i, j + 1, 1);
                glEnd();
            }
        }

        if(i > 0)
        {
            if(cells[(j)*size[0] + (i - 1)].passable)
            {
                glColor3f(.8f, .8f, .8f);
                glBegin(GL_QUADS);
                glTexCoord2i(0, 1);
                glVertex3i(i, j + 1, 1);
                glTexCoord2i(0, 0);
                glVertex3i(i, j + 1, 0);
                glTexCoord2i(1, 0);
                glVertex3i(i, j, 0);
                glTexCoord2i(1, 1);
                glVertex3i(i, j, 1);
                glEnd();
            }
        }

        //--- Les partie central
        if(cells[pos].height > 2)
        {
            int h = 1;
            glBindTexture(GL_TEXTURE_2D, tex_wall_center);
            if(j > 0)
            {
                if(cells[(j - 1)*size[0] + (i)].passable || cells[pos].height - 1 > cells[(j - 1)*size[0] + (i)].height)
                {
                    if(!cells[(j - 1)*size[0] + (i)].passable)
                    {
                        h = cells[(j - 1)*size[0] + (i)].height;
                    }
                    glColor3f(.3f, .3f, .3f);
                    glBegin(GL_QUADS);
                    glTexCoord2i(0, cells[pos].height - 1 - h);
                    glVertex3i(i, j, cells[pos].height - 1);
                    glTexCoord2i(0, 0);
                    glVertex3i(i, j, h);
                    glTexCoord2i(1, 0);
                    glVertex3i(i + 1, j, h);
                    glTexCoord2i(1, cells[pos].height - 1 - h);
                    glVertex3i(i + 1, j, cells[pos].height - 1);
                    glEnd();
                }
            }

            if(i < size[0] - 1)
            {
                if(cells[(j)*size[0] + (i + 1)].passable || cells[pos].height > cells[(j)*size[0] + (i + 1)].height)
                {
                    if(!cells[(j)*size[0] + (i + 1)].passable)
                    {
                        h = cells[(j)*size[0] + (i + 1)].height;
                    }
                    glColor3f(.4f, .4f, .4f);
                    glBegin(GL_QUADS);
                    glTexCoord2i(0, cells[pos].height - 1 - h);
                    glVertex3i(i + 1, j, cells[pos].height - 1);
                    glTexCoord2i(0, 0);
                    glVertex3i(i + 1, j, h);
                    glTexCoord2i(1, 0);
                    glVertex3i(i + 1, j + 1, h);
                    glTexCoord2i(1, cells[pos].height - 1 - h);
                    glVertex3i(i + 1, j + 1, cells[pos].height - 1);
                    glEnd();
                }
            }

            if(j < size[1] - 1)
            {
                if(cells[(j + 1)*size[0] + (i)].passable || cells[pos].height > cells[(j + 1)*size[0] + (i)].height)
                {
                    if(!cells[(j + 1)*size[0] + (i)].passable)
                    {
                        h = cells[(j + 1)*size[0] + (i)].height;
                    }
                    glColor3f(.7f, .7f, .7f);
                    glBegin(GL_QUADS);
                    glTexCoord2i(0, cells[pos].height - 1 - h);
                    glVertex3i(i + 1, j + 1, cells[pos].height - 1);
                    glTexCoord2i(0, 0);
                    glVertex3i(i + 1, j + 1, h);
                    glTexCoord2i(1, 0);
                    glVertex3i(i, j + 1, h);
                    glTexCoord2i(1, cells[pos].height - 1 - h);
                    glVertex3i(i, j + 1, cells[pos].height - 1);
                    glEnd();
                }
            }

            if(i > 0)
            {
                if(cells[(j)*size[0] + (i - 1)].passable || cells[pos].height > cells[(j)*size[0] + (i - 1)].height)
                {
                    if(!cells[(j)*size[0] + (i - 1)].passable)
                    {
                        h = cells[(j)*size[0] + (i - 1)].height;
                    }
                    glColor3f(.8f, .8f, .8f);
                    glBegin(GL_QUADS);
                    glTexCoord2i(0, cells[pos].height - 1 - h);
                    glVertex3i(i, j + 1, cells[pos].height - 1);
                    glTexCoord2i(0, 0);
                    glVertex3i(i, j + 1, h);
                    glTexCoord2i(1, 0);
                    glVertex3i(i, j, h);
                    glTexCoord2i(1, cells[pos].height - 1 - h);
                    glVertex3i(i, j, cells[pos].height - 1);
                    glEnd();
                }
            }
        }

        //--- La partie du haut
        if(cells[pos].height > 1)
        {
            glBindTexture(GL_TEXTURE_2D, tex_wall_up);
            if(j > 0)
            {
                if(cells[(j - 1)*size[0] + (i)].passable || cells[pos].height > cells[(j - 1)*size[0] + (i)].height)
                {
                    glColor3f(.3f, .3f, .3f);
                    glBegin(GL_QUADS);
                    glTexCoord2i(0, 1);
                    glVertex3i(i, j, cells[pos].height);
                    glTexCoord2i(0, 0);
                    glVertex3i(i, j, cells[pos].height - 1);
                    glTexCoord2i(1, 0);
                    glVertex3i(i + 1, j, cells[pos].height - 1);
                    glTexCoord2i(1, 1);
                    glVertex3i(i + 1, j, cells[pos].height);
                    glEnd();
                }
            }

            if(i < size[0] - 1)
            {
                if(cells[(j)*size[0] + (i + 1)].passable || cells[pos].height > cells[(j)*size[0] + (i + 1)].height)
                {
                    glColor3f(.4f, .4f, .4f);
                    glBegin(GL_QUADS);
                    glTexCoord2i(0, 1);
                    glVertex3i(i + 1, j, cells[pos].height);
                    glTexCoord2i(0, 0);
                    glVertex3i(i + 1, j, cells[pos].height - 1);
                    glTexCoord2i(1, 0);
                    glVertex3i(i + 1, j + 1, cells[pos].height - 1);
                    glTexCoord2i(1, 1);
                    glVertex3i(i + 1, j + 1, cells[pos].height);
                    glEnd();
                }
            }

            if(j < size[1] - 1)
            {
                if(cells[(j + 1)*size[0] + (i)].passable || cells[pos].height > cells[(j + 1)*size[0] + (i)].height)
                {
                    glColor3f(.7f, .7f, .7f);
                    glBegin(GL_QUADS);
                    glTexCoord2i(0, 1);
                    glVertex3i(i + 1, j + 1, cells[pos].height);
                    glTexCoord2i(0, 0);
                    glVertex3i(i + 1, j + 1, cells[pos].height - 1);
                    glTexCoord2i(1, 0);
                    glVertex3i(i, j + 1, cells[pos].height - 1);
                    glTexCoord2i(1, 1);
                    glVertex3i(i, j + 1, cells[pos].height);
                    glEnd();
                }
            }

            if(i > 0)
            {
                if(cells[(j)*size[0] + (i - 1)].passable || cells[pos].height > cells[(j)*size[0] + (i - 1)].height)
                {
                    glColor3f(.8f, .8f, .8f);
                    glBegin(GL_QUADS);
                    glTexCoord2i(0, 1);
                    glVertex3i(i, j + 1, cells[pos].height);
                    glTexCoord2i(0, 0);
                    glVertex3i(i, j + 1, cells[pos].height - 1);
                    glTexCoord2i(1, 0);
                    glVertex3i(i, j, cells[pos].height - 1);
                    glTexCoord2i(1, 1);
                    glVertex3i(i, j, cells[pos].height);
                    glEnd();
                }
            }
        }

        //--- Le top
        glBindTexture(GL_TEXTURE_2D, tex_wall_top);
        glColor3f(1, 1, 1);
        glBegin(GL_QUADS);
        glTexCoord2i(0, 1);
        glVertex3i(i, j + 1, cells[pos].height);
        glTexCoord2i(0, 0);
        glVertex3i(i, j, cells[pos].height);
        glTexCoord2i(1, 0);
        glVertex3i(i + 1, j, cells[pos].height);
        glTexCoord2i(1, 1);
        glVertex3i(i + 1, j + 1, cells[pos].height);
        glEnd();
        glEndList();
    }
}


void ClientMap::regenDL()
{
    //---We create now all the wall's display list (HEaDShOt)
    int i, j;
    for(j = 0; j < size[1]; ++j)
    {
        for(i = 0; i < size[0]; ++i)
        {
            regenCell(i, j);
        }
    }

}


//--- To reload the theme
void ClientMap::reloadTheme()
{
    if(tex_grass) dktDeleteTexture(&tex_grass);
    if(tex_dirt) dktDeleteTexture(&tex_dirt);
    if(tex_wall) dktDeleteTexture(&tex_wall);

    CString themeStr = "";

    switch(theme)
    {
    case THEME_GRASS:    themeStr = THEME_GRASS_STR;    break;
    case THEME_SNOW:     themeStr = THEME_SNOW_STR;     break;
    case THEME_SAND:     themeStr = THEME_SAND_STR;     break;
    case THEME_CITY:     themeStr = THEME_CITY_STR;     break;
    case THEME_MODERN:   themeStr = THEME_MODERN_STR;   break;
    case THEME_LAVA:     themeStr = THEME_LAVA_STR;     break;
    case THEME_ANIMAL:   themeStr = THEME_ANIMAL_STR;   break;
    case THEME_ORANGE:   themeStr = THEME_ORANGE_STR;   break;
    case THEME_CORE:     themeStr = THEME_CORE_STR;     break;
    case THEME_FROZEN:   themeStr = THEME_FROZEN_STR;   break;
    case THEME_GRAIN:    themeStr = THEME_GRAIN_STR;    break;
    case THEME_MEDIEVAL: themeStr = THEME_MEDIEVAL_STR; break;
    case THEME_METAL:    themeStr = THEME_METAL_STR;    break;
    case THEME_RAINY:    themeStr = THEME_RAINY_STR;    break;
    case THEME_REAL:     themeStr = THEME_REAL_STR;     break;
    case THEME_ROAD:     themeStr = THEME_ROAD_STR;     break;
    case THEME_ROCK:     themeStr = THEME_ROCK_STR;     break;
    case THEME_SAVANA:   themeStr = THEME_SAVANA_STR;   break;
    case THEME_SOFT:     themeStr = THEME_SOFT_STR;     break;
    case THEME_STREET:   themeStr = THEME_STREET_STR;   break;
    case THEME_TROPICAL: themeStr = THEME_TROPICAL_STR; break;
    case THEME_WINTER:   themeStr = THEME_WINTER_STR;   break;
    case THEME_WOODEN:   themeStr = THEME_WOODEN_STR;   break;
    }
    if(themeStr == "" || gameVar.cl_grassTextureForAllMaps)
    {
        theme = THEME_GRASS;
        themeStr = THEME_GRASS_STR;
    }

    tex_grass = dktCreateTextureFromFile(CString("main/textures/themes/%s/tex_floor.tga", themeStr.s).s, DKT_FILTER_BILINEAR);
    if(tex_grass == 0)
        tex_grass = dktCreateTextureFromFile(CString("main/textures/themes/grass/tex_floor.tga").s, DKT_FILTER_BILINEAR);
    tex_dirt = dktCreateTextureFromFile(CString("main/textures/themes/%s/tex_floor_dirt.tga", themeStr.s).s, DKT_FILTER_BILINEAR);
    if(tex_dirt == 0)
        tex_dirt = dktCreateTextureFromFile(CString("main/textures/themes/grass/tex_floor_dirt.tga").s, DKT_FILTER_BILINEAR);
    tex_wall = dktCreateTextureFromFile(CString("main/textures/themes/%s/tex_wall_center.tga", themeStr.s).s, DKT_FILTER_BILINEAR);
    if(tex_wall == 0)
        tex_wall = dktCreateTextureFromFile(CString("main/textures/themes/grass/tex_wall_center.tga").s, DKT_FILTER_BILINEAR);
    buildAll();
}



//
// Destructeur
//
ClientMap::~ClientMap()
{
    if(isValid && cell_dl)
    {
        for(int i = 0; i < size[0] * size[1]; ++i) {
            if (cell_dl[i]) glDeleteLists(cell_dl[i], 1);
        }
        ZEVEN_SAFE_DELETE_ARRAY(cell_dl);
    }
    if(!isServer)
    {
        ZEVEN_SAFE_DELETE(m_weather);

        dktDeleteTexture(&texMap);
        dktDeleteTexture(&tex_grass);
        dktDeleteTexture(&tex_dirt);
        dktDeleteTexture(&tex_wall);

        //  dktDeleteTexture(&tex_floor);
        //  dktDeleteTexture(&tex_floor_dirt);
        //  dktDeleteTexture(&tex_wall_bottom);
        //  dktDeleteTexture(&tex_wall_center);
        //  dktDeleteTexture(&tex_wall_up);
        //  dktDeleteTexture(&tex_wall_top);

        dkoDeleteModel(&dko_flag[0]);
        dkoDeleteModel(&dko_flagPod[0]);
        dkoDeleteModel(&dko_flag[1]);
        dkoDeleteModel(&dko_flagPod[1]);

        if(groundMesh) delete groundMesh;
        if(shadowMesh) delete shadowMesh;
        if(wallMesh) delete wallMesh;
    }
}

void ClientMap::regenTex()
{
    int i, j;

    texMapSize[0] = 1.0f;
    texMapSize[1] = 1.0f;
    int width = 1;
    while(width < size[0])
    {
        width *= 2;
    }
    if(width != size[0])
    {
        texMapSize[0] = float(size[0]) / float(width);
    }
    int height = 1;
    while(height < size[1])
    {
        height *= 2;
    }
    if(height != size[1])
    {
        texMapSize[1] = float(size[1]) / float(height);
    }
    unsigned char * textureData = new unsigned char[width*height * 3];
    memset(textureData, 0, width*height * 3);
    // On rempli le data dans la texture
    for(j = 0; j < height; ++j)
    {
        for(i = 0; i < width; ++i)
        {
            if((i < size[0]) && (j < size[1]))
            {
                if(!cells[j*size[0] + i].passable)
                {
                    textureData[(j*width + i) * 3 + 0] = 255;
                    textureData[(j*width + i) * 3 + 1] = 255;
                    textureData[(j*width + i) * 3 + 2] = 255;
                }
            }
        }
    }

    dktCreateTextureFromBuffer(&texMap, textureData, width, height, 3, DKT_FILTER_NEAREST);

    // On efface notre data tempon
    delete[] textureData;

    //--- Rebuild map
    buildAll();
}

void ClientMap::setCameraPos(const CVector3f & pCamPos)
{
    camLookAt = pCamPos;
    camDest = camLookAt + CVector3f(0, 0, 7);
    camPos = camDest;
}

void GetMapList(std::vector< CString > & maps)
{
    maps.clear();
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char DirSpec[MAX_PATH];  // directory specification
    char appPath[MAX_PATH];

    // Chercher le path du "current working directory".
    _getcwd(appPath, MAX_PATH);

    strncpy(DirSpec, appPath, strlen(appPath) + 1);
    strncat(DirSpec, "\\main\\maps\\*.bvm", strlen("\\main\\maps\\*.bvm") + 1);

    hFind = FindFirstFile(DirSpec, &FindFileData);
    if(hFind != INVALID_HANDLE_VALUE)
    {
        CString filename("%s", FindFileData.cFileName);
        // Drop the extension
        filename.resize(filename.len() - 4);
        maps.push_back(filename);
        while(FindNextFile(hFind, &FindFileData) != 0)
        {
            CString filename("%s", FindFileData.cFileName);
            // Drop the extension
            filename.resize(filename.len() - 4);
            maps.push_back(filename);
        }
        FindClose(hFind);
    }
}

bool GetMapData(CString name, unsigned int & texture, CVector2i & textureSize, CVector2i & size, CString & author)
{
    FileIO file(CString("main\\maps\\%s.bvm", name.s), "rb");
    if(file.isValid())
    {
        map_cell * cells = 0;
        unsigned long mapVersion = file.getULong();

        switch(mapVersion)
        {
        case 10010:
            size[0] = file.getInt();
            size[1] = file.getInt();
            cells = new map_cell[size[0] * size[1]];
            for(int j = 0; j < size[1]; ++j)
            {
                for(int i = 0; i < size[0]; ++i)
                {
                    unsigned char data = file.getUByte();
                    cells[j * size[0] + i].passable = (data & 128) ? true : false;
                    cells[j * size[0] + i].height = (data & 127);
                    data = file.getUByte();
                }
            }
            break;
        case 10011:
            size[0] = file.getInt();
            size[1] = file.getInt();
            cells = new map_cell[size[0] * size[1]];
            for(int j = 0; j < size[1]; ++j)
            {
                for(int i = 0; i < size[0]; ++i)
                {
                    unsigned char data = file.getUByte();
                    cells[j * size[0] + i].passable = (data & 128) ? true : false;
                    cells[j * size[0] + i].height = (data & 127);
                    data = file.getUByte();
                }
            }
            break;
        case 20201:
        {
            int theme = file.getInt();
            int weather = file.getInt();
            size[0] = file.getInt();
            size[1] = file.getInt();
            cells = new map_cell[size[0] * size[1]];
            for(int j = 0; j < size[1]; ++j)
            {
                for(int i = 0; i < size[0]; ++i)
                {
                    unsigned char data = file.getUByte();
                    cells[j * size[0] + i].passable = (data & 128) ? true : false;
                    cells[j * size[0] + i].height = (data & 127);
                    data = file.getUByte();
                }
            }
            break;
        }
        case 20202:
        {
            char * author_name_buffer = file.getByteArray(25);
            author_name_buffer[24] = '\0';
            author.set("%s", author_name_buffer);
            delete[] author_name_buffer;
            int theme = file.getInt();
            int weather = file.getInt();
            size[0] = file.getInt();
            size[1] = file.getInt();
            cells = new map_cell[size[0] * size[1]];
            for(int j = 0; j < size[1]; ++j)
            {
                for(int i = 0; i < size[0]; ++i)
                {
                    unsigned char data = file.getUByte();
                    cells[j * size[0] + i].passable = (data & 128) ? true : false;
                    cells[j * size[0] + i].height = (data & 127);
                    data = file.getUByte();
                }
            }
            break;
        }
        }

        textureSize.set(1, 1);
        while(textureSize[0] < size[0])
        {
            textureSize[0] *= 2;
        }
        while(textureSize[1] < size[1])
        {
            textureSize[1] *= 2;
        }
        unsigned char * textureBytes = new unsigned char[textureSize[0] * textureSize[1] * 3];
        memset(textureBytes, 0, textureSize[0] * textureSize[1] * 3);
        for(int j = 0; j < size[1]; ++j)
        {
            for(int i = 0; i < size[0]; ++i)
            {
                unsigned int m = (j * size[0] + i);
                unsigned int t = (j * textureSize[0] + i) * 3;
                if(!cells[m].passable)
                {
                    textureBytes[t + 0] = 200;
                    textureBytes[t + 1] = 200;
                    textureBytes[t + 2] = 200;
                }
            }
        }
        delete[] cells;
        texture = dktCreateEmptyTexture(textureSize[0], textureSize[1], 3, DKT_FILTER_NEAREST);
        dktCreateTextureFromBuffer(&texture, textureBytes, textureSize[0], textureSize[1], 3, DKT_FILTER_NEAREST);
        delete[] textureBytes;
    }
    else
    {
        return false;
    }
    return true;
}
