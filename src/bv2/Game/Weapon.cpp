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

#include "Weapon.h"
#include "Game.h"

//
// Constructeur
//
Weapon::Weapon(float pFireDelay, CString pWeaponName, float pDamage, float pImp, int pNbShot, float pReculVel, float pStartImp, int pWeaponID, int pProjectileType)
{
    currentFireDelay = 0;
    charge = 0;
    m_owner = 0;
    justCharged = 0;
    isInstance = false;

    chainOverHeat = 1;
    overHeated = false;

    shotInc = 0;
    fullReload = true;
    weaponName = pWeaponName;

    fireDelay = pFireDelay;
    currentFireDelay = 0; // On est ready à tirer au début

    damage = pDamage;
    impressision = pImp;
    nbShot = pNbShot;
    reculVel = pReculVel;
    startImp = pStartImp;
    weaponID = pWeaponID;
    projectileType = pProjectileType;
}

Weapon::Weapon(Weapon * pWeapon)
{
    int i;

    shotInc = 0;
    fullReload = true;
    isInstance = true;
    currentFireDelay = 0;

    chainOverHeat = 1;
    overHeated = false;
    m_owner = 0;
    justCharged = 0;
    charge = 0;

    weaponName = pWeapon->weaponName;
    fireDelay = pWeapon->fireDelay;
    currentFireDelay = 0;
    damage = pWeapon->damage;
    impressision = pWeapon->impressision;
    nbShot = pWeapon->nbShot;
    reculVel = pWeapon->reculVel;
    currentImp = startImp = pWeapon->startImp;
    weaponID = pWeapon->weaponID;
    projectileType = pWeapon->projectileType;
}

//
// Destructeur
//
Weapon::~Weapon()
{
}

// Le server shot le player Melee
void Weapon::shootMeleeSV(Player * owner)
{
    currentFireDelay = fireDelay;
    owner->fireFrameDelay = 2;
    switch (weaponID)
    {
    case WEAPON_KNIVES:
        //--- On tue toute dans un rayon de 1 :D
        owner->game->radiusHit(owner->currentCF.position, 1, owner->playerID, weaponID, true);
        break;
    case WEAPON_SHIELD:
        //--- Protect this player for 2 seconde
        owner->protection = 2;
        break;
    }
}

//
// On l'update
//
void Weapon::update(float delay)
{
    if (justCharged > 0) justCharged -= delay;
    if (justCharged < 0) justCharged = 0;
    if (m_owner) if (m_owner->status != PLAYER_STATUS_ALIVE) return;
    if (currentImp > startImp)
    {
        currentImp -= delay * 10;
        if (currentImp < startImp)
        {
            currentImp = startImp;
        }
    }
    //--- flame thrower need a tite flame bleu
    if (weaponID == WEAPON_FLAME_THROWER)
    {
        //--- On va oublier ça pour tout suite
    }
    if (currentFireDelay > 0)
    {
        currentFireDelay -= delay;
        if (weaponID == WEAPON_KNIVES)
        {
            if (currentFireDelay > fireDelay - .10f)
            {
                modelAnim = (1 - ((currentFireDelay - (fireDelay - .10f)) / .10f)) * 10.0f;
                if (modelAnim > 10) modelAnim = 10;
            }
            else if (currentFireDelay < .25f)
            {
                modelAnim = (currentFireDelay / .25f) * 10.0f;
                if (modelAnim < 0) modelAnim = 0;
            }
            else
            {
                modelAnim = 10;
            }
        }
        if (weaponID == WEAPON_SHIELD)
        {
            modelAnim = (3 - currentFireDelay) / 3.0f * 20.0f;
            if (modelAnim > 20) modelAnim = 20;
        }
    }
    else
    {
        modelAnim = 0;
    }

    chainOverHeat += delay * .25f;
    if (chainOverHeat > 1)
    {
        chainOverHeat = 1;
        overHeated = false;
    }

    if (chainOverHeat > 0.5)
    {
        overHeated = false;
    }
}
