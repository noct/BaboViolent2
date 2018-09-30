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

#ifndef GAME_H
#define GAME_H

#include "Player.h"
#include "Map.h"
#include <vector>
#include <map>
#include "netPacket.h"
#include <Zeven/MemIO.h>

class Client;

#define MAX_PLAYER 32
#ifndef DEDICATED_SERVER
#define MAX_FLOOR_MARK 500
#endif
#define GAME_TYPE_COUNT 4
#define GAME_TYPE_DM 0
#define GAME_TYPE_TDM 1
#define GAME_TYPE_CTF 2

#define SERVER_TYPE_NORMAL 0
#define SERVER_TYPE_PRO 1

#define SPAWN_TYPE_NORMAL 0
#define SPAWN_TYPE_LADDER 1

#define GAME_PLAYING -1
#define GAME_BLUE_WIN 0
#define GAME_RED_WIN 1
#define GAME_DRAW 2
#define GAME_DONT_SHOW 3
#define GAME_MAP_CHANGE 4

#define ROUND_BLUE_WIN 5
#define ROUND_RED_WIN 6

#define ITEM_LIFE_PACK 1
#define ITEM_WEAPON 2
#define ITEM_GRENADE 3

// Projectiles (rocket, grenade, etc)
struct Projectile
{
    // Le type (rocket, grenade, etc)
    int projectileType;

    // Son (ses) coordframes
    CoordFrame currentCF; // Celui qu'on affiche
    CoordFrame lastCF; // Le key frame de sauvegarde du frame courant
    CoordFrame netCF0; // L'avant dernier keyframe reçu du net
    CoordFrame netCF1; // Le dernier keyframe reçu du net

    // Sa progression sur la courbe
    long cFProgression;

    // Si c'est un entity controllé par le server
    bool remoteEntity;

    // On en a fini avec, on l'efface
    bool needToBeDeleted;

    // quick hack to give the molotov one more frame before it gets deleted ( to prevent invisi flame bug )
    bool reallyNeedToBeDeleted;

#ifndef DEDICATED_SERVER
    // La rocket tourne sur elle même
    float rotation;
    float rotateVel;

    bool isClientOnly;
#endif

    // Il a une duration limite
    float duration;

    // Hey, de qui ça vient ça?
    char fromID;
    bool movementLock;

    // Pour savoir quand shooter le data au client
    long whenToShoot;
#ifndef DEDICATED_SERVER
    int spawnParticleTime;
#endif
    int damageTime;

    int stickToPlayer;
    float stickFor;

    // determines how long the flame has been thrown, usefull so that when you are the thrower, you can only stick on flames after 1 second
    float timeSinceThrown;

    // Son ID dans le vector
    short projectileID;

    // Notre ID unique pour identifier nos projectiles
    long uniqueID;
    static long uniqueProjectileID;

    // Constructeur
    Projectile(CVector3f & position, CVector3f & vel, char pFromID, int pProjectileType, bool pRemoteEntity, long pUniqueProjectileID);

    // Son update
    void update(float delay, Map * map);
#ifndef DEDICATED_SERVER
    // Pour l'afficher (client Only)
    void render();
    void renderShadow();
#endif
    // pour updater le coordFrame avec celui du server
    void setCoordFrame(net_svcl_projectile_coord_frame & projectileCoordFrame);
};

class Game
{
public:
    // Sa liste de player
    Player ** players;

    bool isServerGame;

    // Notre map
    Map * map;

    // Le seed
    long mapSeed;

    // notre liste de projectile (très important de toujours les garder dans l'ordre
    std::vector<Projectile*> projectiles;

    CString mapName;

    // Le type de jeu et les scores
    int gameType;
   int spawnType;
   int subGameType;
    int blueScore;
    int redScore;
    int blueWin;
    int redWin;
    float gameTimeLeft;
    float roundTimeLeft;
    bool bombPlanted;
    float bombTime;

    // key: teamid, value: list of userids
    typedef std::map<int, std::vector<int> > ApprovedPlayers;
    // list of approved players for each team
    ApprovedPlayers approvedPlayers;
    // flag for each team
    std::map<int, bool> teamApproveAll;

    // La moyenne des ping dans chaque team
    int bluePing;
    int redPing;
    int ffaPing;
    int spectatorPing;
    // On est en mode fin de round
    int roundState;

    // Notre unique Projectile ID

    //--- Voting
    struct SVoting
    {
        bool votingInProgress;
        CString votingFrom;
        CString votingWhat;
        int votingResults[2]; // Yes/No
        float remaining;
        bool voted;
        std::vector<int> activePlayersID;
    } voting;

    virtual void castVote(const net_clsv_svcl_vote_request & voteRequest);
    bool votingUpdate(float delay);
public:
    // Constructeur
    Game(CString pMapName="");

    // Destructeur
    virtual ~Game();

    // Pour l'updater
    virtual void update(float delay);

    void UpdateProSettings();

    // Create map
    virtual void createMap();

    // pour donner un team à un player
    int assignPlayerTeam(int playerID, char teamRequested, Client * client = 0);

    // let the player join selected team
    bool approvePlayer(int userID, char team);

    // approve all players to join selected team
    void approveAll(int team);

    // approve all players to join
    void approveAll();

    bool isApproved(int userID, int team);

    // removes player from the list of approved players of all teams
    // only works if player was approved to some team before
    void rejectPlayer(int userID);

    // removes player from the list of approved players of selected team
    // only works if player was approved to some team before
    void rejectPlayer(int userID, char team);

    void rejectAllPlayers();

    // Spawner un player
    bool spawnPlayer(int playerID);

    // Pour ajouter un nouveau joueur
    int createNewPlayerSV(int babonetID); // Ça c'est côté server

    // Quand un client shot, mais que le server le vérifie puis le shoot aux autres joueurs
    void shootSV(net_clsv_player_shoot & playerShoot);

    // On spawn un projectile!
    bool spawnProjectile(net_clsv_svcl_player_projectile & playerProjectile, bool imServer);

    // Pour toucher les joueurs dans un rayon
    void radiusHit(CVector3f & position, float radius, char fromID, char weaponID, bool sameDmg=false);

    // Pour savoir s'il y a un joueur dans le radius, last parameter used to ignore a specific player ( -1 = not ignoring anyone )
    Player * playerInRadius(CVector3f position, float radius, int ignore = -1 );

    // Pour starter un nouveau type de game
    void resetGameType(int pGameType);

    // Pour starter un new round
    virtual void resetRound();

    // Number of players

    int numPlayers();

private:
    void shootSV(int playerID, int nuzzleID, float imp, CVector3f p1, CVector3f p2);
};

#endif
