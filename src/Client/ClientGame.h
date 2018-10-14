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
#include "ClientPlayer.h"
#include "Game.h"
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
};

struct NukeFlash
{
    CVector3f position;
    float life;
    float fadeSpeed;
    float density;
    float radius;
    NukeFlash();
    void update(float pdelay);
};

struct Drip
{
    CVector3f position;
    float life;
    float size;
    float fadeSpeed;
    Drip();
    void update(float pdelay);
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
    FloorMark();
    void set(CVector3f & pposition, float pangle, float psize, float pdelay, float pstartDelay, unsigned int ptexture, CVector4f pcolor);
    void update(float pdelay);
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
    Douille(CVector3f pPosition, CVector3f pDirection, CVector3f right, int in_type = DOUILLE_TYPE_DOUILLE);
    void update(float pDelay, Map * map);
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
    Trail(CVector3f & pP1, CVector3f & pP2, float pSize, CVector4f & pColor, float duration, int in_trailType = 0);
    void update(float pDelay);
};

struct ClientGame : public Game
{
    // Notre player
    ClientPlayer * thisPlayer;
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

    void createNewPlayerCL(int playerID, long babonetID); // Ça c'est côté client

    // Pour quand un client shot
    void shoot(const CVector3f & position, const CVector3f & direction, float imp, float damage, Player * from, int projectileType);

    // Pour spawner des particules sur le murs l'hors d'un impact
    void spawnImpact(CVector3f & p1, CVector3f & p2, CVector3f & normal, Weapon*weapon, float damage, int team);
    void spawnBlood(CVector3f & position, float damage);
    void spawnExplosion(CVector3f & position, CVector3f & normal, float size);


    // Pour pogner le prochain markFloor
    long getNextFloorMark() {nextWriteFloorMark++;if(nextWriteFloorMark>=MAX_FLOOR_MARK)nextWriteFloorMark=0;return nextWriteFloorMark;}
    long getNextDrip() {nextWriteDrip++;if(nextWriteDrip>=MAX_FLOOR_MARK)nextWriteDrip=0;return nextWriteDrip;}
};

#endif
