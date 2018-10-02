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

#ifndef CLIENT_PLAYER_H
#define CLIENT_PLAYER_H

#include "Player.h"
#include "ClientWeapon.h"

struct ClientPlayer : public Player
{
    // Si on est le joueur controllclientVar.dkpp_
    bool isThisPlayer;
    // Son shadow
    unsigned int tex_baboShadow;
    unsigned int tex_baboHalo;
    // Pour savoir si on a initialisclientVar.dkpp_le mouse down ici, et non dans le menu
    bool initedMouseClic;

    // Le player qu'on suis
    int followingPlayer;

    // La position du babo on screen
    CVector2i onScreenPos;

    // Est-ce qu'on est en mode scope FPS ou pas?
    bool scopeMode;

    unsigned int tex_skin;
    unsigned int tex_skinOriginal;

    ClientPlayer(char pPlayerID, Map * pMap, Game * pGame);

    // Destructeur
    virtual ~ClientPlayer();

    void update(float delay);
    void spawn(const CVector3f & spawnPoint);
    void kill(bool silenceDeath);
    void switchWeapon(int newWeaponID, bool forceSwitch=false);
    void switchMeleeWeapon(int newWeaponID, bool forceSwitch=false);
    void setThisPlayerInfo();

    // Pour le controller
    void controlIt(float delay);

    // Pour l'afficher
    void render();
    void renderName();

    void hit(ClientWeapon * fromWeapon, ClientPlayer * from, float damage=-1);

    // Pour updater la skin texture
    void updateSkin();

    void onShotgunReload();
    void onSpawnRequest();
    void onDeath(Player* from, Weapon * fromWeapon, bool friendlyFire);
};

#endif
