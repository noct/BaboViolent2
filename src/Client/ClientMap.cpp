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
#include "ClientHelper.h"
#include "ClientGame.h"
#include "Console.h"
#include "Scene.h"
#include <direct.h>
#include <glad/glad.h>

#ifdef RENDER_LAYER_TOGGLE
int renderToggle = 0;
#endif

extern Scene * scene;

struct CWeather
{
    //--- Constructor
    CWeather() {}

    //--- Destructor
    virtual ~CWeather() {}

    //--- Update
    virtual void update(float delay, Map* map) {}

    //--- Render
    virtual void render() {}
};

struct SSnow
{
    CVector3f pos;
    SSnow();
    void update(float delay);
    void render();
};

struct CSnow : public CWeather
{
    //--- Weather sound
    FSOUND_SAMPLE * m_sfxRain;
    int channel;

    //--- La rain
    SSnow rains[100];
    int nextRain;

    //--- Flocon
    unsigned int tex_snow;

    int nextIn;

    //--- Constructor
    CSnow();

    //--- Destructor
    virtual ~CSnow();

    //--- Update
    void update(float delay, Map* map);

    //--- Render
    void render();
};

struct SRain
{
    CVector3f pos;
    SRain();
    void update(float delay);
    void render();
};

struct CRain : public CWeather
{
    //--- Weather sound
    FSOUND_SAMPLE * m_sfxRain;
    int channel;

    //--- La rain
    SRain rains[100];
    int nextRain;

    //--- Constructor
    CRain();

    //--- Destructor
    virtual ~CRain();

    //--- Update
    void update(float delay, Map* map);

    //--- Render
    void render();
};


struct CLava : public CWeather
{
    //--- Weather sound
    FSOUND_SAMPLE * m_sfxRain;
    int channel;

    //--- Constructor
    CLava();

    //--- Destructor
    virtual ~CLava();

    //--- Update
    void update(float delay, Map* map);

    //--- Render
    void render();
};


//--- SVertex: A single vertex with a normal, single texture, and colour
struct SVertex
{
    float x,y,z;
    float nx,ny,nz;
    float u,v;
    float r,g,b,a;
};


//--- CMaterial: A set of settings used to render a vertex buffer
class CMaterial
{
public:
    typedef unsigned int texture_t;

    //--- No texture constant
    static const texture_t no_texture = static_cast<texture_t>(-1);

    //--- Blend modes
    enum blend_t
    {
        BLEND_NONE,
        BLEND_ALPHA,
    };

    //--- Constructor/Destructor
    CMaterial(texture_t tex = no_texture, blend_t blend = BLEND_NONE, bool diffuse = true, bool lit = false);
    virtual ~CMaterial();

    //--- Enable/Disable (sets OpenGL states)
    void enable(SVertex* first) const;
    void disable() const;

    //--- Equality operator
    bool operator==(const CMaterial &rhs) const;

protected:
    texture_t   m_tex;
    blend_t     m_blend;
    bool        m_lit;
    bool        m_diffuse;
};


//--- CVertexBuffer: A set of tris rendered using the same texture
class CVertexBuffer
{
public:
    typedef SVertex                 vertex_t;
    typedef std::vector<vertex_t>   vertex_buf_t;

    //--- Constructor/Destructor
    CVertexBuffer(const CMaterial& mat);
    virtual ~CVertexBuffer();

    //--- Adds a tri
    void add(const SVertex& a, const SVertex& b, const SVertex& c);

    //--- Gets the material
    const CMaterial& material() { return m_mat; }

    //--- Gets a pointer to the first vertex
    SVertex*        first() { return m_vb.size() > 0 ? &m_vb[0] : 0; }

    //--- Size
    size_t size() { return m_vb.size(); }

protected:
    vertex_buf_t    m_vb;
    CMaterial       m_mat;
};

//--- CMesh: A renderable set of vertex buffers
class CMesh
{
public:
    typedef std::vector<CVertexBuffer>  vb_list_t;

    //--- Constructor/Destructor
    CMesh(vb_list_t& vbs);
    virtual ~CMesh();

    //--- Render
    void render();
    void renderSubMesh(size_t index);

    //--- Size
    size_t size() { return m_vbs.size(); }

protected:
    vb_list_t   m_vbs;
};


//--- CMeshBuilder: Builds a set of vertex buffers a vertex at a time
class CMeshBuilder
{
public:
    typedef CVertexBuffer::vertex_buf_t vb_t;
    typedef CMesh::vb_list_t            vb_list_t;

    //--- Constructor/Destructor
    CMeshBuilder(const CMaterial& material);
    virtual ~CMeshBuilder();

    //--- Set current material
    void bind(const CMaterial& material, bool forceNew = false);

    //--- Adds a vertex
    void vertex(float x, float y, float z, float u = 0, float v = 0);

    //--- Set current normal
    void normal(float x, float y, float z);

    //--- Set current colour
    void colour(float r, float g, float b, float a = 1);

    //--- Compile the mesh (delete when finished)
    CMesh* compile();

protected:
    //--- List of vertex buffers
    vb_list_t   m_vbs;

    //--- Temporary buffer for incomplete tris
    vb_t        m_tempBuf;

    //--- Current normal & colour
    CVector3f   m_normal;
    CVector4f   m_colour;

    //--- Index to current buffer
    size_t      m_i;
};

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


void ClientMap::buildAll()
{
    buildGround();
    buildShadow();
    buildWalls();
}

void ClientMap::buildGround()
{
    if(groundMesh) delete groundMesh;

    CMaterial base(tex_dirt);
    CMaterial base_weather(tex_dirt, CMaterial::BLEND_ALPHA);
    CMaterial splat(tex_grass, CMaterial::BLEND_ALPHA);

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

    CMaterial shadow(CMaterial::no_texture, CMaterial::BLEND_ALPHA, true);

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

    CMaterial wall(tex_wall, CMaterial::BLEND_NONE, true);
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

void ClientMap::renderGround()
{
    glPushAttrib(GL_ENABLE_BIT);
    glDepthMask(GL_FALSE);

    //--- Render the map
    groundMesh->render();

    // render the grid for the editor
    if(isEditor)
    {
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glLineWidth(2);
        glColor3f(0.0f, 0.0f, 0.0f);
        for(int j = 0; j < size[1]; ++j)
        {
            for(int i = 0; i < size[0]; ++i)
            {
                glBegin(GL_LINE_LOOP);
                glVertex2i(i, j + 1);
                glVertex2i(i, j);
                glVertex2i(i + 1, j);
                glVertex2i(i + 1, j + 1);
                glEnd();
            }
        }

        float currentRedDist = 0;
        float farthestRedSpawn = 0;
        float currentBlueDist = 0;
        float farthestBlueSpawn = 0;
        for(int i = 0; i < (int)dm_spawns.size(); i++)
        {//draw projected spawn
            int dist = 1000000;
            for(int j = 0; j < (int)blue_spawns.size(); j++)
            {
                if(distanceSquared(dm_spawns[i], blue_spawns[j]) < dist)
                    dist = (int)distanceSquared(dm_spawns[i], blue_spawns[j]);
            }
            if(dist > currentRedDist)
            {
                currentRedDist = (float)dist;
                farthestRedSpawn = (float)i;
            }
            dist = 1000000;
            for(int j = 0; j < (int)red_spawns.size(); j++)
            {
                if(distanceSquared(dm_spawns[i], red_spawns[j]) < dist)
                    dist = (int)distanceSquared(dm_spawns[i], red_spawns[j]);
            }
            if(dist > currentBlueDist)
            {
                currentBlueDist = (float)dist;
                farthestBlueSpawn = (float)i;
            }
        }
        if(!dm_spawns.empty())
        {
            for(int i = 0; i < (int)blue_spawns.size(); i++)
            {
                glColor3f(1.0, 0.0, 0.0);
                glLineWidth(2);
                glBegin(GL_LINE_LOOP);
                {
                    glVertex2f(dm_spawns[(int)farthestRedSpawn][0], dm_spawns[(int)farthestRedSpawn][1]);
                    glVertex2f(blue_spawns[i][0], blue_spawns[i][1]);
                }
                glEnd();
            }
            for(int i = 0; i < (int)red_spawns.size(); i++)
            {
                glColor3f(0.0, 0.0, 1.0);
                glLineWidth(2);
                glBegin(GL_LINE_LOOP);
                {
                    glVertex2f(dm_spawns[(int)farthestBlueSpawn][0], dm_spawns[(int)farthestBlueSpawn][1]);
                    glVertex2f(red_spawns[i][0], red_spawns[i][1]);
                }
                glEnd();
            }
        }
    }

    glDepthMask(GL_TRUE);
    glPopAttrib();
}

void ClientMap::renderShadow()
{
    if(gameVar.r_shadowQuality == 0) return;

    shadowMesh->render();
}

void ClientMap::renderWalls()
{
    wallMesh->render();

    // Tout est fini, on peut maintenant renderer le plancher dans le zbuffer
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glBegin(GL_QUADS);
    glVertex2i(0, size[1] + 1);
    glVertex2i(0, 0);
    glVertex2i(size[0] + 1, 0);
    glVertex2i(size[0] + 1, size[1] + 1);
    glEnd();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
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

void ClientMap::renderFlag(int i)
{
    glPushMatrix();
    glTranslatef(flagPos[i][0], flagPos[i][1], flagPos[i][2]);
    glRotatef(flagAngle[i], 0, 0, 1);
    glScalef(.005f, .005f, .005f);
    dkoRender(dko_flag[i], flagAnim);
    glPopMatrix();
}

void ClientMap::renderMisc()
{
    int i;
    if(game && (game->gameType != GAME_TYPE_CTF))
        return;

    glPushMatrix();
    glTranslatef(flagPodPos[0][0], flagPodPos[0][1], flagPodPos[0][2]);
    glScalef(.005f, .005f, .005f);
    dkoRender(dko_flagPod[0]);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(flagPodPos[1][0], flagPodPos[1][1], flagPodPos[1][2]);
    glScalef(.005f, .005f, .005f);
    dkoRender(dko_flagPod[1]);
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_COLOR_MATERIAL);

    // Les spawn si on est en editor
    if(isEditor)
    {
        for(i = 0; i < (int)dm_spawns.size(); ++i)
        {
            glColor3f(1, 0, 1);
            glPushMatrix();
            glTranslatef(dm_spawns[i][0], dm_spawns[i][1], dm_spawns[i][2]);
            //gluSphere(qObj, .25f, 8, 4);
            dkglDrawSphere(0.25f, 8, 4, GL_TRIANGLES);
            glPopMatrix();
        }
        for(i = 0; i < (int)blue_spawns.size(); ++i)
        {
            glColor3f(0, 0, 1);
            glPushMatrix();
            glTranslatef(blue_spawns[i][0], blue_spawns[i][1], blue_spawns[i][2]);
            //gluSphere(qObj, .25f, 8, 4);
            dkglDrawSphere(0.25f, 8, 4, GL_TRIANGLES);
            glPopMatrix();
        }
        for(i = 0; i < (int)red_spawns.size(); ++i)
        {
            glColor3f(1, 0, 0);
            glPushMatrix();
            glTranslatef(red_spawns[i][0], red_spawns[i][1], red_spawns[i][2]);
            //gluSphere(qObj, .25f, 8, 4);
            dkglDrawSphere(0.25f, 8, 4, GL_TRIANGLES);
            glPopMatrix();
        }
    }

    if(((game) && (game->gameType == GAME_TYPE_CTF)) || isEditor)
    {
        float redAnim = flagAnim + 5.0f;
        while(redAnim >= 10) redAnim -= 10;
        // Les flags
        renderFlag(0);
        renderFlag(1);
    }
}

void ClientMap::renderWeather()
{
    if(m_weather) m_weather->render();
}

SSnow::SSnow()
{
}
void SSnow::update(float delay)
{
    pos[2] -= 2 * delay;
    pos += rand(CVector3f(-1,-1,0), CVector3f(1,1,0)) * delay;
}
void SSnow::render()
{
    glColor4f(1, 1, 1,((pos[2] > 2)?2:pos[2]) / 2.0f);
    glTexCoord2f(0,1);
    glVertex3f(pos[0]-.05f,pos[1]+.05f,pos[2]);
    glTexCoord2f(0,0);
    glVertex3f(pos[0]-.05f,pos[1]-.05f,pos[2]);
    glTexCoord2f(1,0);
    glVertex3f(pos[0]+.05f,pos[1]-.05f,pos[2]);
    glTexCoord2f(1,1);
    glVertex3f(pos[0]+.05f,pos[1]+.05f,pos[2]);
}
//
//--- Constructor
//
CSnow::CSnow()
{
    m_sfxRain = dksCreateSoundFromFile("main/sounds/wind.wav", true);
    tex_snow = dktCreateTextureFromFile("main/textures/snowflake.png", DKT_FILTER_LINEAR);

    //--- Start the sound
    channel = dksPlaySound(m_sfxRain, -1, 50);

    nextRain = 0;

    nextIn = 0;
}



//
//--- Destructor
//
CSnow::~CSnow()
{
    FSOUND_StopSound(channel);
    dksDeleteSound(m_sfxRain);
    dktDeleteTexture(&tex_snow);
}



//
//--- Update
//
void CSnow::update(float delay, Map* map)
{
    auto cmap = static_cast<ClientMap*>(map);
    --nextIn;
    //--- On crée la neige yé
    if (nextIn <= 0)
    {
        nextIn = 3;
        for (int i=0;i<1;++i)
        {
            rains[nextRain].pos = rand(cmap->camPos + CVector3f(-3,-3,-2), cmap->camPos + CVector3f(3,3,-2));
            nextRain++;
            if (nextRain == 100) nextRain = 0;
        }
    }

    //--- On anime la plus
    for (int i=0;i<100;++i)
    {
        if (rains[i].pos[2] > 0)
        {
            rains[i].update(delay);
        }
    }
}



//
//--- Render
//
void CSnow::render()
{
    glPushAttrib(GL_ENABLE_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, tex_snow);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
            for (int i=0;i<100;++i)
            {
                if (rains[i].pos[2] > 0)
                {
                    rains[i].render();
                }
            }
        glEnd();
    glPopAttrib();
}

SRain::SRain() {}

void SRain::update(float delay)
{
    pos[2] -= 15 * delay;
}

void SRain::render()
{
    glColor4f(.25f, .7f, .3f,((pos[2] > 2)?2:pos[2]) / 2.0f * .3f);
    glVertex3fv(pos.s);
    glVertex3f(pos[0],pos[1],pos[2]-.5f);
}

//
//--- Constructor
//
CRain::CRain()
{
    m_sfxRain = dksCreateSoundFromFile("main/sounds/rain2.wav", true);

    //--- Start the sound
    channel = dksPlaySound(m_sfxRain, -1, 50);

    nextRain = 0;
}



//
//--- Destructor
//
CRain::~CRain()
{
    FSOUND_StopSound(channel);
    dksDeleteSound(m_sfxRain);
}



//
//--- Update
//
void CRain::update(float delay, Map* map)
{
    auto cmap = static_cast<ClientMap*>(map);
    int i;
    //--- On crée la pluit yé
    for(i = 0; i < 3; ++i)
    {
        rains[nextRain].pos = rand(cmap->camPos + CVector3f(-3, -3, 5), cmap->camPos + CVector3f(3, 3, 5));
        nextRain++;
        if(nextRain == 100) nextRain = 0;
    }

    //--- On anime la plus
    for(i = 0; i < 100; ++i)
    {
        if(rains[i].pos[2] > 0)
        {
            rains[i].update(delay);
        }
    }
}



//
//--- Render
//
void CRain::render()
{
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2);
    glBegin(GL_LINES);
    for(int i = 0; i < 100; ++i)
    {
        if(rains[i].pos[2] > 0)
        {
            rains[i].render();
        }
    }
    glEnd();
    glPopAttrib();
}


//
//--- Constructor
//
CLava::CLava()
{
    m_sfxRain = dksCreateSoundFromFile("main/sounds/lava.wav", true);

    //--- Start the sound
    channel = dksPlaySound(m_sfxRain, -1, 50);
}



//
//--- Destructor
//
CLava::~CLava()
{
    FSOUND_StopSound(channel);
    dksDeleteSound(m_sfxRain);
}



//
//--- Update
//
void CLava::update(float delay, Map* map)
{
}



//
//--- Render
//
void CLava::render()
{
}

CMeshBuilder::CMeshBuilder(const CMaterial& material): m_i(0), m_colour(1,1,1,1)
{
    //--- Start the first buffer
    m_vbs.push_back( CVertexBuffer(material) );
}

CMeshBuilder::~CMeshBuilder() {}

void CMeshBuilder::bind(const CMaterial& material, bool forceNew)
{
    //--- Empty the temporary buffer
    m_tempBuf.resize(0);

    //--- Check if this texture/mode combo is used already
    if(!forceNew)
    {
        for(size_t i = 0; i < m_vbs.size(); ++i)
        {
            if(m_vbs[i].material() == material)
            {
                m_i = i;
                return;
            }
        }
    }

    //--- Start a new buffer
    m_vbs.push_back( CVertexBuffer(material) );
    m_i = m_vbs.size() - 1;
}

void CMeshBuilder::vertex(float x, float y, float z, float u, float v)
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
        m_vbs[m_i].add( m_tempBuf[0], m_tempBuf[1], m_tempBuf[2] );

        //--- Empty the temporary buffer
        m_tempBuf.resize(0);
    }
}

void CMeshBuilder::normal(float x, float y, float z)
{
    m_normal.set(x,y,z);
}

void CMeshBuilder::colour(float r, float g, float b, float a)
{
    m_colour.set(r,g,b,a);
}

CMesh* CMeshBuilder::compile()
{
    //--- Empty the temporary buffer
    m_tempBuf.resize(0);

    //--- Return a CMesh (this will clear m_vbs)
    return new CMesh( m_vbs );
}

CMesh::CMesh(vb_list_t& vbs)
{
    //--- Steal the vbs
    m_vbs.swap(vbs);
}

CMesh::~CMesh() {}

void CMesh::render()
{
    for(size_t i = 0; i < m_vbs.size(); ++i)
        renderSubMesh(i);
}

void CMesh::renderSubMesh(size_t index)
{
    //--- Check for invalid index or empty buffer
    if(index < 0 || index >= m_vbs.size() || m_vbs[index].size() < 3 )
        return;

    //--- Get a reference to the vb & material
    CVertexBuffer&      vb = m_vbs[index];
    const CMaterial&    mat = vb.material();

    //--- Enable material
    mat.enable( vb.first() );

    //--- Draw!
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vb.size()) );

    //--- Disable material
    mat.disable();
}

CMaterial::CMaterial(texture_t tex, blend_t blend, bool diffuse, bool lit): m_tex(tex), m_blend(blend), m_diffuse(diffuse), m_lit(lit)
{

}

CMaterial::~CMaterial()
{

}

void CMaterial::enable(SVertex* first) const
{
    //--- Push enable bit, this causes OpenGL to track and revert glEnable states
    glPushAttrib(GL_ENABLE_BIT);

    //--- Always need verticies
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(SVertex), &(first->x) );

    //--- Texturing
    if(m_tex != no_texture)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture( GL_TEXTURE_2D, m_tex );
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, sizeof(SVertex), &(first->u) );
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
    }

    //--- Lighting
    if(m_lit)
    {
        glEnable(GL_LIGHTING);
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, sizeof(SVertex), &(first->nx) );
    }
    else
    {
        glDisable(GL_LIGHTING);
    }

    //--- Blending
    if(m_blend > BLEND_NONE)
    {
        glEnable(GL_BLEND);

        if(m_blend == BLEND_ALPHA)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
    }
    else
    {
        glDisable(GL_BLEND);
    }

    //--- Diffuse
    if(m_diffuse)
    {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_FLOAT, sizeof(SVertex), &(first->r) );
    }
}

void CMaterial::disable() const
{
    glDisableClientState(GL_VERTEX_ARRAY);

    if(m_tex != no_texture)
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    if(m_lit)
        glDisableClientState(GL_NORMAL_ARRAY);

    if(m_diffuse)
        glDisableClientState(GL_COLOR_ARRAY);

    //--- Return OpenGL to normal
    glPopAttrib();
}

bool CMaterial::operator==(const CMaterial &rhs) const
{
    return (m_tex == rhs.m_tex && m_blend == rhs.m_blend && m_diffuse == rhs.m_diffuse && m_lit == rhs.m_lit);
}

CVertexBuffer::CVertexBuffer(const CMaterial& mat): m_mat(mat) {}

CVertexBuffer::~CVertexBuffer() {}

void CVertexBuffer::add(const SVertex& a, const SVertex& b, const SVertex& c)
{
    m_vb.push_back(a);
    m_vb.push_back(b);
    m_vb.push_back(c);
}
