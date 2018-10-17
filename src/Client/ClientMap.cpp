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
#include "SceneRender.h"
#include "ClientGame.h"
#include "Console.h"
#include <direct.h>
#include <Windows.h>

void CSnow_Init(CSnow* snow)
{
    snow->pos = new CVector3f[100];
    snow->m_sfxRain = dksCreateSoundFromFile("main/sounds/wind.wav", true);
    snow->tex_snow = dktCreateTextureFromFile("main/textures/snowflake.png", DKT_FILTER_LINEAR);
    snow->channel = dksPlaySound(snow->m_sfxRain, -1, 50);
    snow->nextRain = 0;
    snow->nextIn = 0;
}

void CSnow_Release(CSnow* snow)
{
    delete[] snow->pos;
    FSOUND_StopSound(snow->channel);
    dksDeleteSound(snow->m_sfxRain);
    dktDeleteTexture(&snow->tex_snow);
}

void CSnow_Update(CSnow* snow, float delay, Map* map)
{
    auto cmap = static_cast<ClientMap*>(map);
    --snow->nextIn;
    //--- On crée la neige yé
    if (snow->nextIn <= 0)
    {
        snow->nextIn = 3;
        for (int i=0;i<1;++i)
        {
            snow->pos[snow->nextRain] = rand(cmap->camPos + CVector3f(-3,-3,-2), cmap->camPos + CVector3f(3,3,-2));
            snow->nextRain++;
            if (snow->nextRain == 100) snow->nextRain = 0;
        }
    }

    //--- On anime la plus
    for (int i=0;i<100;++i)
    {
        if (snow->pos[i][2] > 0)
        {
            snow->pos[i][2] -= 2 * delay;
            snow->pos[i] += rand(CVector3f(-1, -1, 0), CVector3f(1, 1, 0)) * delay;
        }
    }
}

void CRain_Init(CRain* rain)
{
    rain->pos = new CVector3f[100];
    rain->m_sfxRain = dksCreateSoundFromFile("main/sounds/rain2.wav", true);

    //--- Start the sound
    rain->channel = dksPlaySound(rain->m_sfxRain, -1, 50);

    rain->nextRain = 0;
}

void CRain_Release(CRain* rain)
{
    delete[] rain->pos;
    FSOUND_StopSound(rain->channel);
    dksDeleteSound(rain->m_sfxRain);
}

void CRain_Update(CRain* rain, float delay, Map* map)
{
    auto cmap = static_cast<ClientMap*>(map);
    int i;
    //--- On crée la pluit yé
    for(i = 0; i < 3; ++i)
    {
        rain->pos[rain->nextRain] = rand(cmap->camPos + CVector3f(-3, -3, 5), cmap->camPos + CVector3f(3, 3, 5));
        rain->nextRain++;
        if(rain->nextRain == 100) rain->nextRain = 0;
    }

    //--- On anime la plus
    for(i = 0; i < 100; ++i)
    {
        if(rain->pos[i][2] > 0)
        {
            rain->pos[i][2] -= 15 * delay;
        }
    }
}

void CLava_Init(CLava* lava)
{
    lava->m_sfxRain = dksCreateSoundFromFile("main/sounds/lava.wav", true);

    //--- Start the sound
    lava->channel = dksPlaySound(lava->m_sfxRain, -1, 50);
}

void CLava_Release(CLava* lava)
{
    FSOUND_StopSound(lava->channel);
    dksDeleteSound(lava->m_sfxRain);
}

void CWeather_Init(CWeather* weather, int type)
{
    weather->type = type;
    switch(weather->type)
    {
    case WEATHER_NONE:      break;
    case WEATHER_FOG:       break;
    case WEATHER_SNOW:      CSnow_Init(&weather->data.snow); break;
    case WEATHER_RAIN:      CRain_Init(&weather->data.rain); break;
    case WEATHER_SANDSTORM: break;
    case WEATHER_LAVA:      CLava_Init(&weather->data.lava); break;
    }
}

void CWeather_Release(CWeather* weather)
{
    switch(weather->type)
    {
    case WEATHER_NONE:      break;
    case WEATHER_FOG:       break;
    case WEATHER_SNOW:      CSnow_Release(&weather->data.snow); break;
    case WEATHER_RAIN:      CRain_Release(&weather->data.rain); break;
    case WEATHER_SANDSTORM: break;
    case WEATHER_LAVA:      CLava_Release(&weather->data.lava); break;
    }
    weather->type = WEATHER_NONE;
}

void CWeather_Update(CWeather* weather, float delay, ClientMap* map)
{
    switch(weather->type)
    {
    case WEATHER_NONE:      break;
    case WEATHER_FOG:       break;
    case WEATHER_SNOW:      CSnow_Update(&weather->data.snow, delay, map); break;
    case WEATHER_RAIN:      CRain_Update(&weather->data.rain, delay, map); break;
    case WEATHER_SANDSTORM: break;
    case WEATHER_LAVA:      break;
    }
}

CMaterial CMaterial_Create(CMaterial::texture_t tex = CMaterial::no_texture, CMaterial::blend_t blend = CMaterial::BLEND_NONE, bool diffuse = true, bool lit = false)
{
    CMaterial material;
    material.m_tex = tex;
    material.m_blend = blend;
    material.m_diffuse = diffuse;
    material.m_lit = lit;
    return material;
}

bool CMaterial_Equal(CMaterial a, CMaterial b)
{
    return (a.m_tex == b.m_tex && a.m_blend == b.m_blend && a.m_diffuse == b.m_diffuse && a.m_lit == b.m_lit);
}

void MeshPart_Add(MeshPart* part, const SVertex& a, const SVertex& b, const SVertex& c)
{
    part->buffer.push_back(a);
    part->buffer.push_back(b);
    part->buffer.push_back(c);
}

//--- CMeshBuilder: Builds a set of vertex buffers a vertex at a time
struct CMeshBuilder
{
    typedef MeshPart::vertex_buf_t vb_t;
    typedef CMesh::vb_list_t            vb_list_t;

    //--- List of vertex buffers
    vb_list_t   parts;

    //--- Temporary buffer for incomplete tris
    vb_t        m_tempBuf;

    //--- Current normal & colour
    CVector3f   m_normal;
    CVector4f   m_colour;

    //--- Index to current buffer
    size_t      m_i;

    CMeshBuilder(const CMaterial& material): m_i(0), m_colour(1,1,1,1)
    {
        //--- Start the first buffer
        MeshPart part;
        part.material = material;
        parts.push_back( part );
    }

    ~CMeshBuilder() {}

    void bind(CMaterial material, bool forceNew = false)
    {
        //--- Empty the temporary buffer
        m_tempBuf.resize(0);

        //--- Check if this texture/mode combo is used already
        if(!forceNew)
        {
            for(size_t i = 0; i < parts.size(); ++i)
            {
                if(CMaterial_Equal(parts[i].material, material))
                {
                    m_i = i;
                    return;
                }
            }
        }

        //--- Start a new buffer
        MeshPart part;
        part.material = material;
        parts.push_back( part );
        m_i = parts.size() - 1;
    }

    void vertex(float x, float y, float z, float u = 0.f, float v = 0.f)
    {
        //--- Create the vertex
        SVertex vtx = { x, y, z,
            m_normal[0], m_normal[1], m_normal[2],
            u, v, m_colour[0], m_colour[1], m_colour[2], m_colour[3]
        };

        //--- Add to the temporary buffer
        m_tempBuf.push_back( vtx );

        //--- Do we have enough for a tri?
        if(m_tempBuf.size() > 2)
        {
            MeshPart_Add(&parts[m_i], m_tempBuf[0], m_tempBuf[1], m_tempBuf[2] );

            //--- Empty the temporary buffer
            m_tempBuf.resize(0);
        }
    }

    void normal(float x, float y, float z)
    {
        m_normal.set(x,y,z);
    }

    void colour(float r, float g, float b, float a = 1.f)
    {
        m_colour.set(r,g,b,a);
    }

    CMesh* compile()
    {
        //--- Empty the temporary buffer
        m_tempBuf.resize(0);

        //--- Return a CMesh (this will clear parts)
        CMesh* mesh = new CMesh;
        mesh->parts.swap(parts);
        return mesh;
    }
};

ClientMap::ClientMap(CString mapName, FileIO* mapData, bool isServerGame, unsigned int font, int sizeX, int sizeY)
: Map(), groundMesh(0), shadowMesh(0), wallMesh(0)
{
    int i, j, gtnum;
    //-- On print le loading screen! (new)
    // On clear les buffers, on init la camera, etc

    renderLoadingScreen(font);

    zoom = 0;
    isServer = isServerGame;

    if(mapName.len() > 15) mapName.resize(15);

    isValid = true;
    this->mapName = mapName;
    flagAnim = 0;
    CWeather_Init(&m_weather, WEATHER_NONE);

    if(!isServer)
    {
        // Les textures
        tex_grass = dktCreateTextureFromFile("main/textures/grass.png", DKT_FILTER_BILINEAR);
        tex_dirt = dktCreateTextureFromFile("main/textures/dirt1.png", DKT_FILTER_BILINEAR);
        tex_wall = dktCreateTextureFromFile("main/textures/dirt2.png", DKT_FILTER_BILINEAR);

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

    if(!mapData->isValid())
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
        unsigned long mapVersion = mapData->getULong();

        switch(mapVersion)
        {
        case 10010:
        {
            size[0] = mapData->getInt();
            size[1] = mapData->getInt();
            cells = new map_cell[size[0] * size[1]];
            for(j = 0; j < size[1]; ++j)
            {
                for(i = 0; i < size[0]; ++i)
                {
                    unsigned char data = mapData->getUByte();
                    cells[j*size[0] + i].passable = (data & 128) ? true : false;
                    cells[j*size[0] + i].height = (data & 127);
                    data = mapData->getUByte();
                    setTileDirt(i, j, ((float)data) / 255.0f);
                }
            }
            break;
        }
        case 10011:
        {
            size[0] = mapData->getInt();
            size[1] = mapData->getInt();
            cells = new map_cell[size[0] * size[1]];
            for(j = 0; j < size[1]; ++j)
            {
                for(i = 0; i < size[0]; ++i)
                {
                    unsigned char data = mapData->getUByte();
                    cells[j*size[0] + i].passable = (data & 128) ? true : false;
                    cells[j*size[0] + i].height = (data & 127);
                    data = mapData->getUByte();
                    setTileDirt(i, j, ((float)data) / 255.0f);
                }
            }

            // Les flag
            flagPodPos[0] = mapData->getVector3f();
            flagPodPos[1] = mapData->getVector3f();

            // Les ojectifs
            mapData->getVector3f();
            mapData->getVector3f();

            // Les spawn point
            int nbSpawn = mapData->getInt();
            for(i = 0; i < nbSpawn; ++i)
            {
                dm_spawns.push_back(mapData->getVector3f());
            }
            nbSpawn = mapData->getInt();
            for(i = 0; i < nbSpawn; ++i)
            {
                blue_spawns.push_back(mapData->getVector3f());
            }
            nbSpawn = mapData->getInt();
            for(i = 0; i < nbSpawn; ++i)
            {
                red_spawns.push_back(mapData->getVector3f());
            }
            break;
        }
        case 20201:
        {
            theme = mapData->getInt();
            weather = mapData->getInt();
            size[0] = mapData->getInt();
            size[1] = mapData->getInt();
            cells = new map_cell[size[0] * size[1]];
            for(j = 0; j < size[1]; ++j)
            {
                for(i = 0; i < size[0]; ++i)
                {
                    unsigned char data = mapData->getUByte();
                    cells[j*size[0] + i].passable = (data & 128) ? true : false;
                    cells[j*size[0] + i].height = (data & 127);
                    data = mapData->getUByte();
                    setTileDirt(i, j, ((float)data) / 255.0f);
                }
            }

            // Les flag
            flagPodPos[0] = mapData->getVector3f();
            flagPodPos[1] = mapData->getVector3f();

            // Les ojectifs
            mapData->getVector3f();
            mapData->getVector3f();

            // Les spawn point
            int nbSpawn = mapData->getInt();
            for(i = 0; i < nbSpawn; ++i)
            {
                dm_spawns.push_back(mapData->getVector3f());
            }
            nbSpawn = mapData->getInt();
            for(i = 0; i < nbSpawn; ++i)
            {
                blue_spawns.push_back(mapData->getVector3f());
            }
            nbSpawn = mapData->getInt();
            for(i = 0; i < nbSpawn; ++i)
            {
                red_spawns.push_back(mapData->getVector3f());
            }
            break;
        }
        case 20202:
        {
            // Common map data
            char * author_name_buffer = mapData->getByteArray(25);
            author_name_buffer[24] = '\0';
            author_name.set("%.24s", author_name_buffer);
            // Note: we DO NOT want to overwrite the author field if it's being edited
            theme = mapData->getInt();
            weather = mapData->getInt();
            delete[] author_name_buffer;
            author_name_buffer = 0;

            // for gcc compliant code

            int i = 0;

            size[0] = mapData->getInt();
            size[1] = mapData->getInt();
            cells = new map_cell[size[0] * size[1]];
            for(j = 0; j < size[1]; ++j)
            {
                for(i = 0; i < size[0]; ++i)
                {
                    unsigned char data = mapData->getUByte();
                    cells[j*size[0] + i].passable = (data & 128) ? true : false;
                    cells[j*size[0] + i].height = (data & 127);
                    data = mapData->getUByte();
                    setTileDirt(i, j, ((float)data) / 255.0f);
                }
            }
            // common spawns
            int nbSpawn = mapData->getInt();
            for(i = 0; i < nbSpawn; ++i)
            {
                dm_spawns.push_back(mapData->getVector3f());
            }

            // read game-type specific data
            // there must always be one game-type specific section per one supported game type
            for(gtnum = 0; gtnum < GAME_TYPE_COUNT; ++gtnum)
            {
                int id = mapData->getInt();
                switch(id)
                {
                case GAME_TYPE_DM:
                case GAME_TYPE_TDM:
                    break; // nothing to do for DM and TDM
                case GAME_TYPE_CTF:
                {
                    // flags
                    flagPodPos[0] = mapData->getVector3f();
                    flagPodPos[1] = mapData->getVector3f();
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
}


void ClientMap::update(float delay, Player * thisPlayer, Player * flagPlayer0, Player * flagPlayer1)
{
    CWeather_Update(&m_weather, delay, this);

    // Snipers should be able to scope at map edges
    if((thisPlayer && thisPlayer->weapon && thisPlayer->weapon->weaponID != WEAPON_SNIPER) || (thisPlayer && thisPlayer->teamID == PLAYER_TEAM_SPECTATOR))
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

    if(flagState[0] == -2)
    {
        flagPos[0] = flagPodPos[0];
    }
    if(flagState[1] == -2)
    {
        flagPos[1] = flagPodPos[1];
    }
    if(flagState[0] >= 0)
    {
        if(flagPlayer0)
        {
            flagPos[0] = flagPlayer0->currentCF.position;
        }
    }
    if(flagState[1] >= 0)
    {
        if(flagPlayer1)
        {
            flagPos[1] = flagPlayer1->currentCF.position;
        }
    }
}

void ClientMap::reloadWeather()
{
    CWeather_Release(&m_weather);

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

    CWeather_Init(&m_weather, weather);

    if(weather == WEATHER_RAIN)
    {
        fogStart = 4;
        fogEnd = -3;
        fogColor.set(.15f, .25f, .25f, 1);
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
    }
    if(weather == WEATHER_LAVA)
    {
        fogDensity = 0;
    }
}

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

    tex_grass = dktCreateTextureFromFile(CString("main/textures/themes/%s/tex_floor.png", themeStr.s).s, DKT_FILTER_BILINEAR);
    if(tex_grass == 0)
        tex_grass = dktCreateTextureFromFile(CString("main/textures/themes/grass/tex_floor.png").s, DKT_FILTER_BILINEAR);
    tex_dirt = dktCreateTextureFromFile(CString("main/textures/themes/%s/tex_floor_dirt.png", themeStr.s).s, DKT_FILTER_BILINEAR);
    if(tex_dirt == 0)
        tex_dirt = dktCreateTextureFromFile(CString("main/textures/themes/grass/tex_floor_dirt.png").s, DKT_FILTER_BILINEAR);
    tex_wall = dktCreateTextureFromFile(CString("main/textures/themes/%s/tex_wall_center.png", themeStr.s).s, DKT_FILTER_BILINEAR);
    if(tex_wall == 0)
        tex_wall = dktCreateTextureFromFile(CString("main/textures/themes/grass/tex_wall_center.png").s, DKT_FILTER_BILINEAR);
    buildAll();
}



//
// Destructeur
//
ClientMap::~ClientMap()
{
    if(!isServer)
    {
        CWeather_Release(&m_weather);

        dktDeleteTexture(&texMap);
        dktDeleteTexture(&tex_grass);
        dktDeleteTexture(&tex_dirt);
        dktDeleteTexture(&tex_wall);

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


void ClientMap::buildAll()
{
    buildGround();
    buildShadow();
    buildWalls();
}

void ClientMap::buildGround()
{
    if(groundMesh) delete groundMesh;

    CMaterial base = CMaterial_Create(tex_dirt);
    CMaterial base_weather = CMaterial_Create(tex_dirt, CMaterial::BLEND_ALPHA);
    CMaterial splat = CMaterial_Create(tex_grass, CMaterial::BLEND_ALPHA);

    CMeshBuilder builder(base);

    //--- Reflections on?
    if(weather == WEATHER_RAIN || theme == THEME_SNOW)
    {
        //--- Use a blended version
        builder.bind(base_weather);
        builder.colour(1, 1, 1, .6f);
    }

    //--- Base texture
    buildGroundLayer(builder);

    //--- Splat
    builder.bind(splat);
    buildGroundLayer(builder, true);

    groundMesh = builder.compile();
}

void ClientMap::buildGroundLayer(CMeshBuilder& builder, bool splat)
{
    for(int j = 0; j < size[1]; ++j)
    {
        for(int i = 0; i < size[0]; ++i)
        {
            float x = static_cast<float>(i);
            float y = static_cast<float>(j);

            if(splat) builder.colour(1, 1, 1, 1 - cells[j*size[0] + i].splater[0]);
            builder.vertex(x, y + 1, 0, (float)(x)*.5f, (float)(y + 1)*.5f);

            if(splat) builder.colour(1, 1, 1, 1 - cells[j*size[0] + i].splater[1]);
            builder.vertex(x, y, 0, (float)(x)*.5f, (float)(y)*.5f);

            if(splat) builder.colour(1, 1, 1, 1 - cells[j*size[0] + i].splater[2]);
            builder.vertex(x + 1, y, 0, (float)(x + 1)*.5f, (float)(y)*.5f);

            if(splat) builder.colour(1, 1, 1, 1 - cells[j*size[0] + i].splater[2]);
            builder.vertex(x + 1, y, 0, (float)(x + 1)*.5f, (float)(y)*.5f);

            if(splat) builder.colour(1, 1, 1, 1 - cells[j*size[0] + i].splater[3]);
            builder.vertex(x + 1, y + 1, 0, (float)(x + 1)*.5f, (float)(y + 1)*.5f);

            if(splat) builder.colour(1, 1, 1, 1 - cells[j*size[0] + i].splater[0]);
            builder.vertex(x, y + 1, 0, (float)(x)*.5f, (float)(y + 1)*.5f);
        }
    }
}



void ClientMap::buildShadow()
{
    if(shadowMesh) delete shadowMesh;

    CMaterial shadow = CMaterial_Create(CMaterial::no_texture, CMaterial::BLEND_ALPHA, true);

    CMeshBuilder builder(shadow);

    int i, j;

    for(j = 1; j < size[1]; ++j)
    {
        for(i = 0; i < size[0] - 1; ++i)
        {
            if(!cells[j*size[0] + i].passable)
            {
                if(cells[(j - 1)*size[0] + i].passable)
                {
                    builder.colour(0, 0, 0, .7f);

                    builder.vertex((float)i + 1, (float)j, 0);
                    builder.vertex((float)i, (float)j, 0);
                    builder.colour(0, 0, 0, .0f);
                    builder.vertex((float)i + 1, (float)j - 1, 0);

                    builder.vertex((float)i + 1, (float)j - 1, 0);
                    builder.vertex((float)i + 2.0f, (float)j - 1, 0);
                    builder.colour(0, 0, 0, .7f);
                    builder.vertex((float)i + 1, (float)j, 0);
                }

                if(cells[(j)*size[0] + i + 1].passable)
                {
                    builder.colour(0, 0, 0, .7f);
                    builder.vertex((float)i + 1, (float)j + 1, 0);
                    builder.vertex((float)i + 1, (float)j, 0);
                    builder.colour(0, 0, 0, .0f);
                    builder.vertex((float)i + 2.0f, (float)j - 1, 0);

                    builder.vertex((float)i + 2.0f, (float)j - 1, 0);
                    builder.vertex((float)i + 2.0f, (float)j, 0);
                    builder.colour(0, 0, 0, .7f);
                    builder.vertex((float)i + 1, (float)j + 1, 0);
                }
            }
        }
    }

    shadowMesh = builder.compile();
}

void ClientMap::buildWalls()
{
    if(wallMesh) delete wallMesh;

    CMaterial wall = CMaterial_Create(tex_wall, CMaterial::BLEND_NONE, true);
    CMeshBuilder builder(wall);

    int i, j, h;
    for(j = 0; j < size[1]; ++j)
    {
        for(i = 0; i < size[0]; ++i)
        {
            if(!cells[j*size[0] + i].passable)
            {
                h = cells[j*size[0] + i].height;
                buildWallBlock(builder, i, j, h);
            }
        }
    }

    wallMesh = builder.compile();
}


void ClientMap::buildWallBlock(CMeshBuilder& builder, int x, int y, int h)
{//since this calls renderWallSide, it would also require that opengl be set to render quads
    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);
    float fh = static_cast<float>(h);

    //the block's corners...
    float corner000[3] = { fx,fy,0 };
    float corner001[3] = { fx,fy,fh };
    float corner010[3] = { fx,fy + 1,0 };
    float corner011[3] = { fx,fy + 1,fh };
    float corner100[3] = { fx + 1,fy,0 };
    float corner101[3] = { fx + 1,fy,fh };
    float corner110[3] = { fx + 1,fy + 1,0 };
    float corner111[3] = { fx + 1,fy + 1,fh };

    float shadowL = 1.0f;
    float shadowR = 1.0f;
    float shadowT = 1.0f;
    float shadowB = 1.0f;
    if(gameVar.r_shadowQuality > 0)
    {
        shadowL = 0.8f;
        shadowR = 0.4f;
        shadowT = 0.7f;
        shadowB = 0.35f;
    }

    map_cell* cell_top = (y + 1 < size[1] ? &cells[(y + 1)*size[0] + x] : 0);
    map_cell* cell_left = (x - 1 >= 0 ? &cells[y*size[0] + x - 1] : 0);
    map_cell* cell_diag = y + 1 < size[1] && x - 1 >= 0 ? &cells[(y + 1)*size[0] + x - 1] : 0;
    bool bTop = cell_top && !cell_top->passable ? cell_top->height > cells[y*size[0] + x].height : false;
    bool bLeft = cell_left && !cell_left->passable ? cell_left->height > cells[y*size[0] + x].height : false;
    bool bDiag = cell_diag && !cell_diag->passable ? cell_diag->height > cells[y*size[0] + x].height : false;

    //top of wall
    buildWallTop(builder, corner001, corner101, corner111, corner011, bTop, bLeft, bDiag);

    //bottom wall side if visible
    if(y != 0 && (cells[(y - 1)*size[0] + x].passable || cells[(y - 1)*size[0] + x].height < h))
        buildWallSide(builder, corner000, corner100, corner101, corner001, shadowB, fh);

    //right wall side if visible
    if(x < size[0] - 1 && (cells[y*size[0] + (x + 1)].passable || cells[y*size[0] + (x + 1)].height < h))
        buildWallSide(builder, corner100, corner110, corner111, corner101, shadowR, fh);

    //top wall side if visible
    if(y < size[1] - 1 && (cells[(y + 1)*size[0] + x].passable || cells[(y + 1)*size[0] + x].height < h))
        buildWallSide(builder, corner110, corner010, corner011, corner111, shadowT, fh);

    //left wall side if visible
    if(x != 0 && (cells[y*size[0] + (x - 1)].passable || cells[y*size[0] + (x - 1)].height < h))
        buildWallSide(builder, corner010, corner000, corner001, corner011, shadowL, fh);

}

void ClientMap::buildWallTop(CMeshBuilder& builder, float* vert1, float* vert2, float* vert3, float* vert4, bool top, bool left, bool diag)
{//I don't think this will work if opengl isn't set to render quads before it's called
    float s = 0.4f;
    builder.colour(1, 1, 1);

    if(left || diag) builder.colour(s, s, s);
    builder.vertex(vert4[0], vert4[1], vert4[2], 0, 1);
    if(!left && diag) builder.colour(1, 1, 1);
    builder.vertex(vert1[0], vert1[1], vert1[2], 0, 0);
    builder.colour(1, 1, 1);
    builder.vertex(vert2[0], vert2[1], vert2[2], 1, 0);

    if(top) builder.colour(s, s, s);
    builder.vertex(vert3[0], vert3[1], vert3[2], 1, 1);
    if(!top && diag) builder.colour(s, s, s);
    builder.vertex(vert4[0], vert4[1], vert4[2], 0, 1);
    builder.colour(1, 1, 1);
    builder.vertex(vert2[0], vert2[1], vert2[2], 1, 0);

}

void ClientMap::buildWallSide(CMeshBuilder& builder, float* vert1, float* vert2, float* vert3, float* vert4, float brightness, float h)
{//I don't think this will work if opengl isn't set to render quads before it's called
    builder.colour(brightness, brightness, brightness);

    builder.vertex(vert4[0], vert4[1], vert4[2], 0, h);
    builder.vertex(vert1[0], vert1[1], vert1[2], 0, 0);
    builder.vertex(vert2[0], vert2[1], vert2[2], 1, 0);

    builder.vertex(vert3[0], vert3[1], vert3[2], 1, h);
    builder.vertex(vert4[0], vert4[1], vert4[2], 0, h);
    builder.vertex(vert2[0], vert2[1], vert2[2], 1, 0);
}

void ClientMap::performCollision(CoordFrame & lastCF, CoordFrame & CF, float radius)
{
    // Ici c'est super dooper easy
    if(cells)
    {
        int x = (int)CF.position[0];
        int y = (int)CF.position[1];
        // On check en Y first of all
        if(x < 1)
            x = 1;
        if(y < 1)
            y = 1;
        if(x >= size[0] - 1)
            x = size[0] - 2;
        if(y >= size[1] - 1)
            y = size[1] - 2;
        //prevents high velocity objects(minibots) from going into outer walls and causing a crash, they should still bounce correctly
        if(CF.vel[1] < 0)
        {
            if(!cells[(y - 1)*size[0] + (x)].passable)
            {
                // Est-ce qu'on entre en collision avec
                if(lastCF.position[0] - radius <= (float)(x)+1 &&
                    lastCF.position[0] + radius >= (float)(x) &&
                    CF.position[1] - radius <= (float)(y - 1) + 1 &&
                    CF.position[1] + radius >= (float)(y - 1))
                {
                    // On le ramène en Y
                    CF.position[1] = (float)(y - 1) + 1 + radius + COLLISION_EPSILON;
                    CF.vel[1] = -CF.vel[1] * BOUNCE_FACTOR; // On le fait rebondir ! Bedong!
                }
            }
            if(!cells[(y - 1)*size[0] + (x - 1)].passable)
            {
                // Est-ce qu'on entre en collision avec
                if(lastCF.position[0] - radius <= (float)(x - 1) + 1 &&
                    lastCF.position[0] + radius >= (float)(x - 1) &&
                    CF.position[1] - radius <= (float)(y - 1) + 1 &&
                    CF.position[1] + radius >= (float)(y - 1))
                {
                    // On le ramène en Y
                    CF.position[1] = (float)(y - 1) + 1 + radius + COLLISION_EPSILON;
                    CF.vel[1] = -CF.vel[1] * BOUNCE_FACTOR; // On le fait rebondir ! Bedong!
                }
            }
            if(!cells[(y - 1)*size[0] + (x + 1)].passable)
            {
                // Est-ce qu'on entre en collision avec
                if(lastCF.position[0] - radius <= (float)(x + 1) + 1 &&
                    lastCF.position[0] + radius >= (float)(x + 1) &&
                    CF.position[1] - radius <= (float)(y - 1) + 1 &&
                    CF.position[1] + radius >= (float)(y - 1))
                {
                    // On le ramène en Y
                    CF.position[1] = (float)(y - 1) + 1 + radius + COLLISION_EPSILON;
                    CF.vel[1] = -CF.vel[1] * BOUNCE_FACTOR; // On le fait rebondir ! Bedong!
                }
            }
        }
        else if(CF.vel[1] > 0)
        {
            if(!cells[(y + 1)*size[0] + (x)].passable)
            {
                // Est-ce qu'on entre en collision avec
                if(lastCF.position[0] - radius <= (float)(x)+1 &&
                    lastCF.position[0] + radius >= (float)(x) &&
                    CF.position[1] - radius <= (float)(y + 1) + 1 &&
                    CF.position[1] + radius >= (float)(y + 1))
                {
                    // On le ramène en Y
                    CF.position[1] = (float)(y + 1) - radius - COLLISION_EPSILON;
                    CF.vel[1] = -CF.vel[1] * BOUNCE_FACTOR; // On le fait rebondir ! Bedong!
                }
            }
            if(!cells[(y + 1)*size[0] + (x - 1)].passable)
            {
                // Est-ce qu'on entre en collision avec
                if(lastCF.position[0] - radius <= (float)(x - 1) + 1 &&
                    lastCF.position[0] + radius >= (float)(x - 1) &&
                    CF.position[1] - radius <= (float)(y + 1) + 1 &&
                    CF.position[1] + radius >= (float)(y + 1))
                {
                    // On le ramène en Y
                    CF.position[1] = (float)(y + 1) - radius - COLLISION_EPSILON;
                    CF.vel[1] = -CF.vel[1] * BOUNCE_FACTOR; // On le fait rebondir ! Bedong!
                }
            }
            if(!cells[(y + 1)*size[0] + (x + 1)].passable)
            {
                // Est-ce qu'on entre en collision avec
                if(lastCF.position[0] - radius <= (float)(x + 1) + 1 &&
                    lastCF.position[0] + radius >= (float)(x + 1) &&
                    CF.position[1] - radius <= (float)(y + 1) + 1 &&
                    CF.position[1] + radius >= (float)(y + 1))
                {
                    // On le ramène en Y
                    CF.position[1] = (float)(y + 1) - radius - COLLISION_EPSILON;
                    CF.vel[1] = -CF.vel[1] * BOUNCE_FACTOR; // On le fait rebondir ! Bedong!
                }
            }
        }

        // On check en X asteur (sti c sketch comme technique, mais bon, c juste babo là!)
        if(CF.vel[0] < 0)
        {
            if(!cells[(y)*size[0] + (x - 1)].passable)
            {
                // Est-ce qu'on entre en collision avec
                if(CF.position[0] - radius <= (float)(x - 1) + 1 &&
                    CF.position[0] + radius >= (float)(x - 1) &&
                    lastCF.position[1] - radius <= (float)(y)+1 &&
                    lastCF.position[1] + radius >= (float)(y))
                {
                    // On le ramène en Y
                    CF.position[0] = (float)(x - 1) + 1 + radius + COLLISION_EPSILON;
                    CF.vel[0] = -CF.vel[0] * BOUNCE_FACTOR; // On le fait rebondir ! Bedong!
                }
            }
            if(!cells[(y - 1)*size[0] + (x - 1)].passable)
            {
                // Est-ce qu'on entre en collision avec
                if(CF.position[0] - radius <= (float)(x - 1) + 1 &&
                    CF.position[0] + radius >= (float)(x - 1) &&
                    lastCF.position[1] - radius <= (float)(y - 1) + 1 &&
                    lastCF.position[1] + radius >= (float)(y - 1))
                {
                    // On le ramène en Y
                    CF.position[0] = (float)(x - 1) + 1 + radius + COLLISION_EPSILON;
                    CF.vel[0] = -CF.vel[0] * BOUNCE_FACTOR; // On le fait rebondir ! Bedong!
                }
            }
            if(!cells[(y + 1)*size[0] + (x - 1)].passable)
            {
                // Est-ce qu'on entre en collision avec
                if(CF.position[0] - radius <= (float)(x - 1) + 1 &&
                    CF.position[0] + radius >= (float)(x - 1) &&
                    lastCF.position[1] - radius <= (float)(y + 1) + 1 &&
                    lastCF.position[1] + radius >= (float)(y + 1))
                {
                    // On le ramène en Y
                    CF.position[0] = (float)(x - 1) + 1 + radius + COLLISION_EPSILON;
                    CF.vel[0] = -CF.vel[0] * BOUNCE_FACTOR; // On le fait rebondir ! Bedong!
                }
            }
        }
        else if(CF.vel[0] > 0)
        {
            if(!cells[(y)*size[0] + (x + 1)].passable)
            {
                // Est-ce qu'on entre en collision avec
                if(CF.position[0] - radius <= (float)(x + 1) + 1 &&
                    CF.position[0] + radius >= (float)(x + 1) &&
                    lastCF.position[1] - radius <= (float)(y)+1 &&
                    lastCF.position[1] + radius >= (float)(y))
                {
                    // On le ramène en Y
                    CF.position[0] = (float)(x + 1) - radius - COLLISION_EPSILON;
                    CF.vel[0] = -CF.vel[0] * BOUNCE_FACTOR; // On le fait rebondir ! Bedong!
                }
            }
            if(!cells[(y - 1)*size[0] + (x + 1)].passable)
            {
                // Est-ce qu'on entre en collision avec
                if(CF.position[0] - radius <= (float)(x + 1) + 1 &&
                    CF.position[0] + radius >= (float)(x + 1) &&
                    lastCF.position[1] - radius <= (float)(y - 1) + 1 &&
                    lastCF.position[1] + radius >= (float)(y - 1))
                {
                    // On le ramène en Y
                    CF.position[0] = (float)(x + 1) - radius - COLLISION_EPSILON;
                    CF.vel[0] = -CF.vel[0] * BOUNCE_FACTOR; // On le fait rebondir ! Bedong!
                }
            }
            if(!cells[(y + 1)*size[0] + (x + 1)].passable)
            {
                // Est-ce qu'on entre en collision avec
                if(CF.position[0] - radius <= (float)(x + 1) + 1 &&
                    CF.position[0] + radius >= (float)(x + 1) &&
                    lastCF.position[1] - radius <= (float)(y + 1) + 1 &&
                    lastCF.position[1] + radius >= (float)(y + 1))
                {
                    // On le ramène en Y
                    CF.position[0] = (float)(x + 1) - radius - COLLISION_EPSILON;
                    CF.vel[0] = -CF.vel[0] * BOUNCE_FACTOR; // On le fait rebondir ! Bedong!
                }
            }
        }
    }

    lastCF.position = CF.position;
}

void ClientMap::collisionClip(CoordFrame & CF, float radius)
{
    int x = (int)CF.position[0];
    int y = (int)CF.position[1];

    // Là c simple, on check les 8 cases autour, pis on clip (pour éviter de se faire pousser dans le mur
    if(cells)
    {
        if(CF.position[0] + radius + COLLISION_EPSILON > (float)x + 1 && !cells[(y)*size[0] + (x + 1)].passable)
        {
            // On clip
            CF.position[0] = (float)x + 1 - radius - COLLISION_EPSILON;
        }
        if(CF.position[0] - radius - COLLISION_EPSILON < (float)x && !cells[(y)*size[0] + (x - 1)].passable)
        {
            // On clip
            CF.position[0] = (float)x + radius + COLLISION_EPSILON;
        }
        if(CF.position[1] + radius + COLLISION_EPSILON > (float)y + 1 && !cells[(y + 1)*size[0] + (x)].passable)
        {
            // On clip
            CF.position[1] = (float)y + 1 - radius - COLLISION_EPSILON;
        }
        if(CF.position[1] - radius - COLLISION_EPSILON < (float)y && !cells[(y - 1)*size[0] + (x)].passable)
        {
            // On clip
            CF.position[1] = (float)y + radius + COLLISION_EPSILON;
        }
    }

    // Clamp with the universe
    if(x <= 0) CF.position[0] = 1 + radius + COLLISION_EPSILON;
    if(x >= size[0] - 1) CF.position[0] = size[0] - 1 - radius - COLLISION_EPSILON;
    if(y <= 0) CF.position[1] = 1 + radius + COLLISION_EPSILON;
    if(y >= size[1] - 1) CF.position[1] = size[1] - 1 - radius - COLLISION_EPSILON;

    // check if we are in a cell, move to the next allowed cells
    if(!cells[y * size[0] + x].passable)
    {
        bool possible[4] = { false,false,false,false };

        if(cells[(y)* size[0] + (x - 1)].passable)
        {
            possible[0] = true;
        }
        if(cells[(y)* size[0] + (x + 1)].passable)
        {
            possible[1] = true;
        }
        if(cells[(y - 1) * size[0] + (x)].passable)
        {
            possible[2] = true;
        }
        if(cells[(y + 1) * size[0] + (x)].passable)
        {
            possible[3] = true;
        }

        //--- On essaye de pogner le best choice pareil là
        float dis[4];
        dis[0] = CF.position[0] - (float)x;
        dis[1] = 1 - (CF.position[0] - (float)x);
        dis[2] = CF.position[1] - (float)y;
        dis[3] = 1 - (CF.position[1] - (float)y);

        float currentMin = 2;
        if(possible[0] && dis[0] < currentMin)
        {
            CF.position[0] = (float)x - radius - COLLISION_EPSILON;
            currentMin = dis[0];
        }
        if(possible[1] && dis[1] < currentMin)
        {
            CF.position[0] = (float)x + 1 + radius + COLLISION_EPSILON;
            currentMin = dis[1];
        }
        if(possible[2] && dis[2] < currentMin)
        {
            CF.position[1] = (float)y - radius - COLLISION_EPSILON;
            currentMin = dis[2];
        }
        if(possible[3] && dis[3] < currentMin)
        {
            CF.position[1] = (float)y + 1 + radius + COLLISION_EPSILON;
            currentMin = dis[3];
        }
    }
}
