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
#include "Game.h"

#ifdef RENDER_LAYER_TOGGLE
extern int renderToggle;
#endif

//
// Affichage
//

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
