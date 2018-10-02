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

#ifndef PLAYER_H
#define PLAYER_H

#include <Zeven/Core.h>
#include "netPacket.h"
#include "Helper.h"
#include "GameVar.h"
#include <list>

#define PLAYER_STATUS_ALIVE 0
#define PLAYER_STATUS_DEAD 1
#define PLAYER_STATUS_LOADING 2

#define PLAYER_TEAM_SPECTATOR -1
#define PLAYER_TEAM_BLUE 0
#define PLAYER_TEAM_RED 1
#define PLAYER_TEAM_AUTO_ASSIGN 2


struct Map;
struct Game;

// Notre coordframe
struct CoordFrame
{
    CVector3f position;
    CVector3f vel;
    long frameID;
    float angle;
    CVector3f mousePosOnMap;
    float camPosZ;

    CoordFrame()
    {
        reset();
    }

    void reset()
    {
        frameID = 0;
        angle = 0;
    }

    void operator=(const CoordFrame & coordFrame)
    {
        position = coordFrame.position;
        vel = coordFrame.vel;
        angle = coordFrame.angle;
        frameID = coordFrame.frameID;
        mousePosOnMap = coordFrame.mousePosOnMap;
    }

    void interpolate(long & cFProgression, CoordFrame & from, CoordFrame & to, float delay)
    {
        cFProgression++; // On incrclientVar.dkpp_ent
        long size = to.frameID - from.frameID;
        if (cFProgression > size)
        {
            if (cFProgression < 15)
            {
                // On le fait avancer normalement avec sa velocity
                position += vel * delay;
            }
            else
            {
                position = to.position;
            }
        }
        else if (cFProgression >= 0)
        {
            // On pogne le temps t en float de 0 clientVar.dkpp_1 de sa progression
            float t = (float)cFProgression / (float)size;
            float animTime = (float)size / (30.0f*3.0f);

            if (gameVar.cl_cubicMotion)
            {
                position = cubicSpline(
                    from.position,
                    from.position + from.vel * animTime,
                    to.position - to.vel * animTime,
                    to.position,
                    t);
                mousePosOnMap = cubicSpline(
                    from.mousePosOnMap,
                    from.mousePosOnMap + (to.mousePosOnMap-from.mousePosOnMap) * animTime,
                    to.mousePosOnMap - (from.mousePosOnMap-to.mousePosOnMap) * animTime,
                    to.mousePosOnMap,
                    t);
            }
            else
            {
                position = to.position;
                vel = to.vel;
            }
        }
    }
};

// Structure for holding stats of disconnected players
struct PlayerStats
{
    PlayerStats(const Player* player);

    void MergeStats(PlayerStats* mergeWith);

    CString name;
    int userID;
    int teamID;
    int kills;
    int deaths;
    float dmg;
    int score;
    int returns;
    int flagAttempts;
    float timePlayedCurGame;
};

#define PING_LOG_SIZE 60

struct Player
{
    //--- Anti-cheat code
    long frameSinceLast;
    long lastFrame;
    long currentFrame;
    long speedHackCount;
    float shotsPerSecond;
    long shotCount;
    float secondPassed;

    CVector3f shootShakeDis;

    int spawnSlot;

    // anti cheat for projectiles and shots
    float mfElapsedSinceLastShot;

    // anti cheat for teleport ack
    int   miNbCoord; //number foc oord frame packets received
    float mfCFTimer; //timer that we use to check for speed average every 3 second
    float mfCumulativeVel; //keep the total of every max(vel) from received coordframes

    // time since the player is dead ( for last second nade / molotov )
    float timeDead;

    // time since player is alive , if < 3, collisions with other baboes are diabled
    float timeAlive;

    //time since player joined the server
    float timeInServer;

    // how long player has been idle
    float timeIdle;

    //--- Si ce joueur est admin.
    //    clientVar.dkpp_ veut dire quil peut envoyer des commandes au server.
    //    Et tout les messages server vont lui clientVar.dkpp_re envoyclientVar.dkpp_
    bool isAdmin;

    // Son nom
    CString name;

    char playerIP[16];

    // La babonetID
    uint32_t babonetID;

    // Le ID dans le jeu
    char playerID;

    // Son team
    char teamID;

    // Notre status (mort, spectateur)
    char status;

    // Account ID
    int userID;

    float protection;

    // Son ping
    int ping;
    int pingSum;
    int avgPing;
    float pingOverMax;
    int currentPingFrame;

    float pingLogInterval;
    float nextPingLogTime;
    int pingLogID;
    int pingLog[PING_LOG_SIZE];
    float babySitTime;

    // Sa vie
    float life;

    // Stats
    float dmg;
    int kills;
    int deaths;
    int score;
    int returns;
    int damage;
    int flagAttempts;
    bool rocketInAir;
    bool detonateRocket;

    // Time played(total time alive), reset on round start
    float timePlayedCurGame;

    // Son socket ID (en string) (clientVar.dkpp_ y a juste le server qui le sait)
    CString socketNumber;

    // Ses coord frames
    CoordFrame currentCF; // Celui qu'on affiche
    CoordFrame lastCF; // Le key frame de sauvegarde du frame courant
    CoordFrame netCF0; // L'avant dernier keyframe reclientVar.dkpp_ du net
    CoordFrame netCF1; // Le dernier keyframe reclientVar.dkpp_ du net

    // Sa progression sur la courbe
    long cFProgression;

    // sa matrice d'orientation (clientVar.dkpp_ c'est client side only)
    CMatrix3x3f matrix;

    // Server only clientVar.dkpp_
    bool waitForPong;

    // ON attends de spawner
    bool spawnRequested;

    // To send the position at each x frame
    int sendPosFrame;

    // On attends 10 sec avant de spawner
    float timeToSpawn;

    float immuneTime;

    // Si c'est un entity controllclientVar.dkpp_par le server
    bool remoteEntity;

    // Le gun avec lequel je vais spawner
    int nextSpawnWeapon;
    int nextMeleeWeapon;

    Weapon * weapon;
    Weapon * meleeWeapon;

    // On va garder un pointeur sur la map
    Map * map;

    // Notre game
    Game * game;
    // Il vient de tirer, on le voit pendant 2sec sur la minimap
    float firedShowDelay;
    float secondsFired;

    // On compteur qui dit clientVar.dkpp_ fait combient de temps que je suis dead
    float deadSince;

    // Un timer qui dit que le player vient de se faire toucher
    float screenHit;

    // Le delait pour shooter les grenade (2sec)
    float grenadeDelay;
    float meleeDelay;
    int nbGrenadeLeft;
    int nbMolotovLeft;

    // Il est en connection interrupted, il a besoin de recevoir les game state
    bool connectionInterrupted;

    long fireFrameDelay; // This is for the shoty bug

    bool lastShootWasNade;

    //--- Notre skin
    CString skin;
    CString lastSkin;

    //--- Les couleurs custom du babo
    CColor3f redDecal;
    CColor3f greenDecal;
    CColor3f blueDecal;

    //--- Player has already voted
    bool voted;

    //--- This player has shot a photon rifle
    unsigned int incShot;
    CVector3f p1;
    CVector3f p2;

    // Constructeur
    Player(char pPlayerID, Map * pMap, Game * pGame);

    // Destructeur
    virtual ~Player();

    // Pour l'updater
    virtual void update(float delay);

    void updatePing(float delay);

    // Pour remettre ses stats clientVar.dkpp_0
    void reinit();

    // Pour lui donner les info de notre joueur
    virtual void setThisPlayerInfo();

    // Pour le forcer clientVar.dkpp_crever (suposons quil change de team)
    virtual void kill(bool silenceDeath);

    // Pour spawner un joueur yclientVar.dkpp_
    virtual void spawn(const CVector3f & spawnPoint);

    // Pour setter le coordframe du player
    void setCoordFrame(net_clsv_svcl_player_coord_frame & playerCoordFrame);

    // Si on se fait toucher !
    void hitSV(Weapon * fromWeapon, Player * from, float damage=-1);

    // Pour changer son gun!
    virtual void switchWeapon(int newWeaponID, bool forceSwitch=false);
    virtual void switchMeleeWeapon(int newWeaponID, bool forceSwitch=false);


    virtual void onShotgunReload() {}
    virtual void onSpawnRequest() {}
    virtual void onDeath(Player* from, Weapon * fromWeapon, bool friendlyFire);
};

#endif