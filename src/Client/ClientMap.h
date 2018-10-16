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

#ifndef CLIENT_MAP_H
#define CLIENT_MAP_H

#include "Map.h"
#include <Zeven/Gfx.h>

struct CSnow
{
    CVector3f * pos;
    FSOUND_SAMPLE * m_sfxRain;
    int channel;
    int nextRain;
    unsigned int tex_snow;
    int nextIn;
};

struct CRain
{
    CVector3f * pos;
    FSOUND_SAMPLE * m_sfxRain;
    int channel;
    int nextRain;
};

struct CLava
{
    FSOUND_SAMPLE * m_sfxRain;
    int channel;
};

struct CWeather
{
    int type;
    union Data
    {
        CRain rain;
        CLava lava;
        CSnow snow;
    } data;
    CWeather() {}
};

struct SVertex
{
    float x,y,z;
    float nx,ny,nz;
    float u,v;
    float r,g,b,a;
};

struct CMaterial
{
    typedef unsigned int texture_t;

    //--- No texture constant
    static const texture_t no_texture = static_cast<texture_t>(-1);

    //--- Blend modes
    enum blend_t
    {
        BLEND_NONE,
        BLEND_ALPHA,
    };

    texture_t   m_tex;
    blend_t     m_blend;
    bool        m_lit;
    bool        m_diffuse;
};

struct MeshPart
{
    typedef SVertex                 vertex_t;
    typedef std::vector<vertex_t>   vertex_buf_t;
    vertex_buf_t    buffer;
    CMaterial       material;
};

struct CMesh
{
    typedef std::vector<MeshPart>  vb_list_t;
    vb_list_t parts;
};

struct CMeshBuilder;

#define SHADOW_DETAIL 32

#define THEME_START    0
// Classic themes
#define THEME_GRASS    (THEME_START +  0)
#define THEME_SNOW     (THEME_START +  1)
#define THEME_SAND     (THEME_START +  2)
#define THEME_CITY     (THEME_START +  3)
#define THEME_MODERN   (THEME_START +  4)
#define THEME_LAVA     (THEME_START +  5)
#define THEME_ANIMAL   (THEME_START +  6)
#define THEME_ORANGE   (THEME_START +  7)
// Pacifist's themes (WOW!)
#define THEME_CORE     (THEME_START +  8)
#define THEME_FROZEN   (THEME_START +  9)
#define THEME_GRAIN    (THEME_START + 10)
#define THEME_MEDIEVAL (THEME_START + 11)
#define THEME_METAL    (THEME_START + 12)
#define THEME_RAINY    (THEME_START + 13)
#define THEME_REAL     (THEME_START + 14)
#define THEME_ROAD     (THEME_START + 15)
#define THEME_ROCK     (THEME_START + 16)
#define THEME_SAVANA   (THEME_START + 17)
#define THEME_SOFT     (THEME_START + 18)
#define THEME_STREET   (THEME_START + 19)
#define THEME_TROPICAL (THEME_START + 20)
#define THEME_WINTER   (THEME_START + 21)
#define THEME_WOODEN   (THEME_START + 22)
// THEME_END must always be the same as the last theme number!
#define THEME_END      THEME_WOODEN

#define THEME_GRASS_STR    "grass"
#define THEME_SNOW_STR     "snow"
#define THEME_SAND_STR     "sand"
#define THEME_CITY_STR     "city"
#define THEME_MODERN_STR   "modern"
#define THEME_LAVA_STR     "lava"
#define THEME_ANIMAL_STR   "animal"
#define THEME_ORANGE_STR   "orange"
#define THEME_CORE_STR     "core"
#define THEME_FROZEN_STR   "frozen"
#define THEME_GRAIN_STR    "grain"
#define THEME_MEDIEVAL_STR "medieval"
#define THEME_METAL_STR    "metal"
#define THEME_RAINY_STR    "rainy"
#define THEME_REAL_STR     "real"
#define THEME_ROAD_STR     "road"
#define THEME_ROCK_STR     "rock"
#define THEME_SAVANA_STR   "savana"
#define THEME_SOFT_STR     "soft"
#define THEME_STREET_STR   "street"
#define THEME_TROPICAL_STR "tropical"
#define THEME_WINTER_STR   "winter"
#define THEME_WOODEN_STR   "wooden"

#define WEATHER_NONE      0
#define WEATHER_FOG       1
#define WEATHER_SNOW      2
#define WEATHER_RAIN      3
#define WEATHER_SANDSTORM 4
#define WEATHER_LAVA      5

struct ClientMap : public Map
{
    float flagAnim;
    unsigned int dko_flag[2];
    unsigned int dko_flagPod[2];
    unsigned int dko_flagPole;
    float flagAnims[2];
    float flagAngle[2];

    //--- Meshes
    CMesh* groundMesh;
    CMesh* shadowMesh;
    CMesh* wallMesh;

    // La position de la camera
    CVector3f camPos;

    // Où elle regarde
    CVector3f camLookAt;

    // La destination de la camera
    CVector3f camDest;

    CVector4f fogColor;
    float fogDensity;
    float fogStart;
    float fogEnd;

    // Textures
    unsigned int tex_grass;
    unsigned int tex_dirt;
    unsigned int tex_wall;

    unsigned int tex_floor;
    unsigned int tex_floor_dirt;
    unsigned int tex_wall_bottom;
    unsigned int tex_wall_center;
    unsigned int tex_wall_up;
    unsigned int tex_wall_top;
    unsigned int tex_wall_both;

    // La texture de la map (la meme en tant que tel !)
    unsigned int texMap;
    CVector2f texMapSize;

    // [PM] Author name
    CString author_name;

    //--- Son thème
    int theme;
    int weather;
    CWeather m_weather;

    // Spec Cam Controls
    float       zoom;

    ClientMap(CString mapFilename, Game * game, unsigned int font, bool editor=false, int sizeX=32, int sizeY=32);
    virtual ~ClientMap();

    void update(float delay, Player * thisPlayer);
    void performCollision(CoordFrame & lastCF, CoordFrame & coordFrame, float radius);
    void collisionClip(CoordFrame & coordFrame, float radius);

    void buildAll();

    void buildGround();
    void buildGroundLayer(CMeshBuilder& builder, bool splat = false);
    void buildShadow();
    void buildWalls();

    // builds a wall block at the specified x,y coordinates with height h
    void buildWallBlock(CMeshBuilder& builder,int x,int y,int h);
    void buildWallTop(CMeshBuilder& builder,float* vert1, float* vert2, float* vert3, float* vert4, bool top, bool left, bool diag);

    //given 4 vertices, a brightness indicator, and the height of the wall side, builds the quad
    void buildWallSide(CMeshBuilder& builder,float*,float*,float*,float*,float brightness,float h);

    // Pour seter la position de la cam
    void setCameraPos(const CVector3f & pCamPos);
    void regenTex();
    void reloadTheme();
    void reloadWeather();
};

// Returns the names of all existing maps
void GetMapList(std::vector< CString > & maps);

// Returns basic info about the map
bool GetMapData(CString name, unsigned int & texture, CVector2i & textureSize, CVector2i & size, CString & author);


#endif
