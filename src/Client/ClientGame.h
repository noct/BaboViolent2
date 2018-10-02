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

#ifndef CLIENT_GAME_H
#define CLIENT_GAME_H

#include "ClientGameVar.h"
#include "Game.h"
#include <glad/glad.h>
#define MAX_FLOOR_MARK 500

// Projectiles (rocket, grenade, etc)
struct ClientProjectile : public Projectile
{
    // La rocket tourne sur elle même
    float rotation;
    float rotateVel;

    bool isClientOnly;

    int spawnParticleTime;

    ClientProjectile(CVector3f & position, CVector3f & vel, char pFromID, int pProjectileType, bool pRemoteEntity, long pUniqueProjectileID);
    void update(float delay, Map * map);

    void onGrenadeRebound(CVector3f p);

    // Pour l'afficher (client Only)
    void render();
    void renderShadow();
};

struct NukeFlash
{
public:
    CVector3f position;
    float life;
    float fadeSpeed;
    float density;
    float radius;
    NukeFlash()
    {
        life = 1;
        density = 2;
        fadeSpeed = .50f;
        radius = 16;
    }
    void update(float pdelay)
    {
        life -= pdelay * fadeSpeed;
    }
    void render()
    {
        glPushMatrix();
            glTranslatef(position[0], position[1], position[2]);
            glScalef(radius, radius, radius);
            glColor4f(1, 1, 1, density*life);
            glBegin(GL_QUADS);
                glTexCoord2i(0,1);
                glVertex2i(-1,1);
                glTexCoord2i(0,0);
                glVertex2i(-1,-1);
                glTexCoord2i(1,0);
                glVertex2i(1,-1);
                glTexCoord2i(1,1);
                glVertex2i(1,1);
            glEnd();
        glPopMatrix();
    }
};

struct Drip
{
    CVector3f position;
    float life;
    float size;
    float fadeSpeed;
    Drip()
    {
        life = 1;
        size = .15f;
        fadeSpeed = 2;
    }
    void update(float pdelay)
    {
        life -= pdelay * fadeSpeed;
    }
    void render()
    {
        glPushMatrix();
            glTranslatef(position[0], position[1], position[2]);
            float _size = (1 - life) * size;
            glScalef(_size, _size, _size);
            glColor4f(.25f, .7f, .3f, life*2);
            glBegin(GL_QUADS);
                glTexCoord2i(0,1);
                glVertex2i(-1,1);
                glTexCoord2i(0,0);
                glVertex2i(-1,-1);
                glTexCoord2i(1,0);
                glVertex2i(1,-1);
                glTexCoord2i(1,1);
                glVertex2i(1,1);
            glEnd();
        glPopMatrix();
    }
};

struct FloorMark
{
    CVector3f position;
    float angle;
    float size;
    float delay; // Sa vie restante
    float startDelay;
    unsigned int texture;
    CVector4f color;
    FloorMark()
    {
        delay = 0;
    }
    void set(CVector3f & pposition, float pangle, float psize, float pdelay, float pstartDelay, unsigned int ptexture, CVector4f pcolor)
    {
        position = pposition;
        angle = pangle;
        size = psize;
        delay = pdelay;
        startDelay = pstartDelay;
        texture = ptexture;
        color = pcolor;
    }
    void update(float pdelay)
    {
        if (startDelay > 0)
        {
            startDelay -= pdelay;
        }
        else
        {
            delay -= pdelay;
        }
    }
    void render()
    {
        if (startDelay <= 0)
        {
            glBindTexture(GL_TEXTURE_2D, texture);
            glPushMatrix();
                glTranslatef(position[0], position[1], position[2] + .025f);
                glRotatef(angle, 0, 0, 1);
                glScalef(size, size, size);
                if (delay < 10)
                    glColor4f(color[0], color[1], color[2], color[3] * ((delay)*0.1f));
                else
                    glColor4fv(color.s);
                glBegin(GL_QUADS);
                    glTexCoord2i(0,1);
                    glVertex2i(-1,1);
                    glTexCoord2i(0,0);
                    glVertex2i(-1,-1);
                    glTexCoord2i(1,0);
                    glVertex2i(1,-1);
                    glTexCoord2i(1,1);
                    glVertex2i(1,1);
                glEnd();
            glPopMatrix();
        }
    }
};

#define DOUILLE_TYPE_DOUILLE 0
#define DOUILLE_TYPE_GIB 1

// Nos douilles, ça on gère pas ça sur le net
struct Douille
{
    CVector3f position;
    CVector3f vel;
    float delay;
    bool soundPlayed;
    int type;
    Douille(CVector3f pPosition, CVector3f pDirection, CVector3f right, int in_type=DOUILLE_TYPE_DOUILLE)
    {
        type = in_type;
        vel = pDirection * 1.5f;
        if (type == DOUILLE_TYPE_DOUILLE)
        {
            delay = 2; // Ça dure 2sec ça, en masse
            vel = rotateAboutAxis(vel, rand(-30.0f, 30.0f), right);
            vel = rotateAboutAxis(vel, rand(0.0f, 360.0f), pDirection);
        }
        else if (type == DOUILLE_TYPE_GIB)
        {
            delay = 2; // 5 sec haha malade
        }
        position = pPosition;
        soundPlayed = false;
    }
    void update(float pDelay, Map * map);
    void render()
    {
        glPushMatrix();
            glTranslatef(position[0], position[1], position[2]);
            glRotatef(delay*90,vel[0], vel[1],0);
            glScalef(.005f,.005f,.005f);
            if (type == DOUILLE_TYPE_DOUILLE) dkoRender(clientVar.dko_douille);
            else if (type == DOUILLE_TYPE_GIB) dkoRender(clientVar.dko_gib);
        glPopMatrix();
    }
};

// Pour nos trail (smoke, rocket, etc)
struct Trail
{
    CVector3f p1;
    CVector3f p2;
    float dis;
    float delay;
    float size;
    float delaySpeed;
    float offset;
    int trailType;
    CVector4f color;
    CVector3f right;
    Trail(CVector3f & pP1, CVector3f & pP2, float pSize, CVector4f & pColor, float duration, int in_trailType=0)
    {
        trailType = in_trailType;
        dis = distance(pP1, pP2);
        delay = 0;
        delaySpeed = 1.0f / (duration);
        p1 = pP1;
        p2 = pP2;
        size = pSize;
        color = pColor;
        right = cross(p2 - p1, CVector3f(0,0,1));
        normalize(right);
        offset = rand(0.0f, 1.0f);
    }
    void render()
    {
        glColor4f(.7f, .7f, .7f, (1-delay)*.5f);
        if (trailType == 1) glColor4f(color[0], color[1], color[2],(1-delay));
        glBegin(GL_QUADS);
            glTexCoord2f(0,dis);
            glVertex3fv((p2-right*delay*size).s);
            glTexCoord2f(0,0);
            glVertex3fv((p1-right*delay*size).s);
            glTexCoord2f(1,0);
            glVertex3fv((p1+right*delay*size).s);
            glTexCoord2f(1,dis);
            glVertex3fv((p2+right*delay*size).s);
        glEnd();
    }
    void renderBullet()
    {
        float progress = ((delay/delaySpeed)*40 + offset*1) / dis;
        if (progress < 1)
        {
            CVector3f dir = p2 - p1;
            float x = p1[0]+dir[0]*progress;
            float y = p1[1]+dir[1]*progress;

            glColor4f(color[0], color[1], color[2],.1f);
            glBegin(GL_QUADS);
                glTexCoord2f(0,1);
                glVertex3f(x-1.0f,y+1.0f,0);
                glTexCoord2f(0,0);
                glVertex3f(x-1.0f,y-1.0f,0);
                glTexCoord2f(1,0);
                glVertex3f(x+1.0f,y-1.0f,0);
                glTexCoord2f(1,1);
                glVertex3f(x+1.0f,y+1.0f,0);
            glEnd();

            glColor4f(color[0], color[1], color[2], 1);
            glBegin(GL_QUADS);
                glTexCoord2f(0,1);
                glVertex3fv((p1+dir*progress+dir/dis-right*.05f).s);
                glTexCoord2f(0,0);
                glVertex3fv((p1+dir*progress-right*.05f).s);
                glTexCoord2f(1,0);
                glVertex3fv((p1+dir*progress+right*.05f).s);
                glTexCoord2f(1,1);
                glVertex3fv((p1+dir*progress+dir/dis+right*.05f).s);
            glEnd();
        }
    }
    void update(float pDelay)
    {
        if (delay > 0)
        {
            delay += pDelay * delaySpeed;
        }
        else
        {
            delay = .001f;
        }
    }
};

struct ClientGame : public Game
{
    // Notre player
    Player * thisPlayer;
    // La font
    unsigned int font;

    // Les textures pour dessiner sur la minimap
    unsigned int tex_miniMapAllied;
    unsigned int tex_miniMapEnemy;

    // Render stats
    bool showStats;

    // Map buffer
    MemIO mapBuffer;
    unsigned int mapBytesRecieved;
    // Notre liste de trail à afficher
    std::vector<Trail*> trails;

    // La liste de projectile client
    std::vector<Projectile*> clientProjectiles;
    // Notre liste de douilles
    std::vector<Douille*> douilles;
    bool showMenu;

    // Son shadow
    unsigned int tex_baboShadow;

    // Nos marques sur le plancher (ça c chouette)
    FloorMark floorMarks[MAX_FLOOR_MARK];
    long nextWriteFloorMark;
    Drip drips[MAX_FLOOR_MARK];
    long nextWriteDrip;

    std::vector<NukeFlash*> nikeFlashes;

    // Une explosion nous fait shaker??
    float viewShake;

    float dotAnim;

    // Les sons pour CTF
    FSOUND_SAMPLE * sfx_fcapture;
    FSOUND_SAMPLE * sfx_ecapture;
    FSOUND_SAMPLE * sfx_return;
    FSOUND_SAMPLE * sfx_win;
    FSOUND_SAMPLE * sfx_loose;

    // Constructeur
    ClientGame(CString pMapName="");

    // Destructeur
    virtual ~ClientGame();

    void createMap();
    void resetRound();

    void update(float delay);
    void castVote(const net_clsv_svcl_vote_request & voteRequest);
    void onTeamSwitch(Player* player);
    void onSpawnPlayer(Player* player);

    bool spawnProjectile(net_clsv_svcl_player_projectile & playerProjectile, bool imServer);
    void spawnProjectileSpecific(CVector3f & position, CVector3f & vel, char pFromID, int pProjectileType, bool pRemoteEntity, long pUniqueProjectileID);

    // Pour l'afficher
    void render();

    // Afficher les stats
    void renderStats();
    void renderBlueTeam(std::vector<Player*> & teamOrder, int & vPos);
    void renderRedTeam(std::vector<Player*> & teamOrder, int & vPos);
    void renderFFA(std::vector<Player*> & teamOrder, int & vPos);
    void renderSpectator(std::vector<Player*> & teamOrder, int & vPos);

    void createNewPlayerCL(int playerID, long babonetID); // Ça c'est côté client

    // Pour quand un client shot
    void shoot(const CVector3f & position, const CVector3f & direction, float imp, float damage, Player * from, int projectileType);

    // Pour spawner des particules sur le murs l'hors d'un impact
    void spawnImpact(CVector3f & p1, CVector3f & p2, CVector3f & normal, Weapon*weapon, float damage, int team);
    void spawnBlood(CVector3f & position, float damage);
    void spawnExplosion(CVector3f & position, CVector3f & normal, float size);

    // Pour afficher la minimap (ouff, je mélange pomal les affaires, tk)
    void renderMiniMap();

    // Pour pogner le prochain markFloor
    long getNextFloorMark() {nextWriteFloorMark++;if(nextWriteFloorMark>=MAX_FLOOR_MARK)nextWriteFloorMark=0;return nextWriteFloorMark;}
    long getNextDrip() {nextWriteDrip++;if(nextWriteDrip>=MAX_FLOOR_MARK)nextWriteDrip=0;return nextWriteDrip;}
};

#endif
