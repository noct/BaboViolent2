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

#ifndef CLIENT_WEAPON_H
#define CLIENT_WEAPON_H

#include "Weapon.h"
#include <Zeven/Gfx.h>

#define NUZZLE_DELAY .10f

struct NuzzleFlash
{
    CMatrix3x3f matrix;
    CVector3f position;
    float delay; // le delait d'affichage du feu
    float angle; // l'angle du nuzzle flash, ça c random
    NuzzleFlash(CMatrix3x3f & pMatrix, CVector3f & pPosition)
    {
        delay = 0;
        angle = 0;
        matrix = pMatrix;
        position = pPosition;
    }
    NuzzleFlash(NuzzleFlash * nuzzleFlash)
    {
        delay = 0;
        angle = 0;
        matrix = nuzzleFlash->matrix;
        position = nuzzleFlash->position;
    }
    void update(float pDelay)
    {
        delay -= pDelay;
        if (delay <= 0) delay = 0;
    }
    void shoot()
    {
        delay = NUZZLE_DELAY;
        angle = rand(0.0f, 360.0f);
    }
    void render();
};

struct ClientWeapon : public Weapon
{
    // Son model DKO
    unsigned int dkoModel;
    unsigned int dkoAlternative;

    // Sa liste de nuzzle (il peut en avoir plusieurs)
    std::vector<NuzzleFlash*> nuzzleFlashes;
    std::vector<NuzzleFlash*> ejectingBrass;

    // À quel nuzzle on est rendu à tirer
    int firingNuzzle;
    // For delayed loading of models
    CString dkoFile;
    CString soundFile;

    // Le son
    FSOUND_SAMPLE * sfx_sound;

    ClientWeapon(CString dkoFilename, CString soundFilename, float pFireDelay, CString pWeaponName, float pDamage, float pImp, int pNbShot, float pReculVel, float pStartImp, int pWeaponID, int pProjectileType);
    ClientWeapon(ClientWeapon * pWeapon);
    virtual ~ClientWeapon();

    // On l'update
    void update(float delay);

    // On tire
    void loadModels();

    void shoot(Player * owner);
    void shoot(net_svcl_player_shoot & playerShoot, Player * owner);
    void shootMelee(Player * owner);

    // On l'affiche
    void render();
};
#endif
