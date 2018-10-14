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

#include "ClientWeapon.h"
#include "GameVar.h"
#include "Player.h"
#include "Map.h"
#include "ClientGame.h"
#include "ClientScene.h"
extern Scene * scene;

ClientWeapon::ClientWeapon(CString dkoFilename, CString soundFilename, float pFireDelay, CString pWeaponName, float pDamage, float pImp, int pNbShot, float pReculVel, float pStartImp, int pWeaponID, int pProjectileType)
: Weapon(pFireDelay, pWeaponName, pDamage, pImp, pNbShot, pReculVel, pStartImp, pWeaponID, pProjectileType)
{
    dkoFile = dkoFilename;
    soundFile = soundFilename;
}

ClientWeapon::ClientWeapon(ClientWeapon * pWeapon): Weapon(pWeapon)
{
    int i;
    dkoModel = pWeapon->dkoModel;
    dkoAlternative = pWeapon->dkoAlternative;
    modelAnim = 0;
    firingNuzzle = 0;
    sfx_sound = pWeapon->sfx_sound;
    for (i=0;i<(int)pWeapon->nuzzleFlashes.size();++i)
    {
        nuzzleFlashes.push_back(new NuzzleFlash(pWeapon->nuzzleFlashes[i]));
    }
    for (i=0;i<(int)pWeapon->ejectingBrass.size();++i)
    {
        ejectingBrass.push_back(new NuzzleFlash(pWeapon->ejectingBrass[i]));
    }
}

//
// Destructeur
//
ClientWeapon::~ClientWeapon()
{
int i;
    if (!isInstance)
    {
        dkoDeleteModel(&dkoModel);
        dksDeleteSound(sfx_sound);
        if (dkoAlternative) dkoDeleteModel(&dkoAlternative);
    }
    for ( i=0;i<(int)nuzzleFlashes.size();++i)
    {
        delete nuzzleFlashes[i];
    }
    nuzzleFlashes.clear();
    for (i=0;i<(int)ejectingBrass.size();++i)
    {
        delete ejectingBrass[i];
    }
    ejectingBrass.clear();
}


void ClientWeapon::loadModels()
{
    firingNuzzle = 0;
    dkoModel = dkoLoadFile(dkoFile.s);
    dkoAlternative = 0;
    if (weaponID == WEAPON_SHIELD)
    {
        dkoAlternative = dkoLoadFile("main/models/ShieldMagnet.DKO");
    }
    modelAnim = 0;

    sfx_sound = dksCreateSoundFromFile(soundFile.s, false);

    int i=0;
    unsigned int flashID;
    CVector3f pos;
    CMatrix3x3f mat;
    do
    {
        i++;
        flashID = dkoGetDummy(CString("flash%i",i).s, dkoModel);
        if (flashID)
        {
            dkoGetDummyPosition(flashID, dkoModel, pos.s);
            dkoGetDummyMatrix(flashID, dkoModel, mat.s);
            nuzzleFlashes.push_back(new NuzzleFlash(mat, pos));
        }
    } while (flashID);

    i = 0;
    do
    {
        i++;
        flashID = dkoGetDummy(CString("eject%i",i).s, dkoModel);
        if (flashID)
        {
            dkoGetDummyPosition(flashID, dkoModel, pos.s);
            dkoGetDummyMatrix(flashID, dkoModel, mat.s);
            ejectingBrass.push_back(new NuzzleFlash(mat, pos));
        }
    } while (flashID);
}

//
// On tire
//
void ClientWeapon::shoot(Player * owner)
{
    auto cgame = static_cast<ClientGame*>(owner->game);
    auto cscene = static_cast<ClientScene*>(scene);
    if (weaponID == WEAPON_BAZOOKA && owner->rocketInAir && currentFireDelay <= fireDelay - 0.25f)
    {
        //we get the direction right so this doesnt create a bug with older servers
        if (nuzzleFlashes.size() > 0)
        {
            owner->currentCF.position[2] = .25f;
            clientVar.dkpp_firingSmoke.positionFrom = nuzzleFlashes[firingNuzzle]->position * .005f;
            clientVar.dkpp_firingSmoke.positionFrom = rotateAboutAxis(clientVar.dkpp_firingSmoke.positionFrom, owner->currentCF.angle, CVector3f(0,0,1)) + owner->currentCF.position;
            clientVar.dkpp_firingSmoke.positionFrom[2] -= owner->currentCF.position[2];
            clientVar.dkpp_firingSmoke.positionTo = clientVar.dkpp_firingSmoke.positionFrom;
            clientVar.dkpp_firingSmoke.direction.set(0,1,0);
            clientVar.dkpp_firingSmoke.direction = rotateAboutAxis(clientVar.dkpp_firingSmoke.direction, owner->currentCF.angle, CVector3f(0,0,1));
            clientVar.dkpp_firingSmoke.pitchTo = 45;
        }
        cgame->shoot(clientVar.dkpp_firingSmoke.positionFrom, clientVar.dkpp_firingSmoke.direction, 0, damage, owner, projectileType);
        return;
    }
    if (currentFireDelay <= 0 && !overHeated)
    {
        if (weaponID == WEAPON_PHOTON_RIFLE && charge < 0.5f)
        {
            if (charge == 0)
            {
                if (justCharged == 0)
                {
                    justCharged = 1;
                    //--- We start the charge, tell the other to play charge sound
                    net_svcl_play_sound playSound;
                    playSound.position[0] = (unsigned char)(owner->currentCF.position[0]);
                    playSound.position[1] = (unsigned char)(owner->currentCF.position[1]);
                    playSound.position[2] = (unsigned char)(owner->currentCF.position[2]);
                    playSound.soundID = SOUND_PHOTON_START;
                    playSound.volume = 150;
                    playSound.range = 5;
                    bb_clientSend(cscene->client->uniqueClientID, (char*)(&playSound), sizeof(net_svcl_play_sound), NET_SVCL_PLAY_SOUND);
                    dksPlay3DSound(clientVar.sfx_photonStart, -1, 5, owner->currentCF.position, 150);
                }
            }
            charge += .033333f;
            return;
        }
        charge = 0;
        modelAnim = 0;
        chainOverHeat -= .052f;
        if (chainOverHeat < 0)
        {
            chainOverHeat = 0;
            if (weaponID == WEAPON_CHAIN_GUN)
            {
                // Un ptit son pour agrémenter tout ça!
                net_svcl_play_sound playSound;
                playSound.position[0] = (unsigned char)(owner->currentCF.position[0]);
                playSound.position[1] = (unsigned char)(owner->currentCF.position[1]);
                playSound.position[2] = (unsigned char)(owner->currentCF.position[2]);
                playSound.soundID = SOUND_OVERHEAT;
                playSound.volume = 150;
                playSound.range = 5;
                bb_clientSend(cscene->client->uniqueClientID, (char*)(&playSound), sizeof(net_svcl_play_sound), NET_SVCL_PLAY_SOUND);
                dksPlay3DSound(clientVar.sfx_overHeat, -1, 5, owner->currentCF.position, 150);
                overHeated = true;

                //--- Un ti peu de fume?
            }
        }

        firingNuzzle++;
        if (firingNuzzle >= (int)nuzzleFlashes.size()) firingNuzzle = 0;
        currentFireDelay = fireDelay;

        //--- Si on est shotgun, pis que ça fait 6 shot, on reload
        shotInc++;
        if (shotInc >= 6 && weaponID == WEAPON_SHOTGUN)
        {
            if (gameVar.sv_enableShotgunReload)
            {
                currentFireDelay = 3;
                fullReload = true;
            }
            else
                shotInc = 0;
        }

        if (nuzzleFlashes.size() > 0)
        {
        /*  if (weaponID == WEAPON_FLAME_THROWER)
            {
                nuzzleFlashes[firingNuzzle]->shoot();
                owner->currentCF.position[2] = .25f;
                clientVar.dkpp_firingSmoke.positionFrom = nuzzleFlashes[firingNuzzle]->position * .005f;
                clientVar.dkpp_firingSmoke.positionFrom = rotateAboutAxis(clientVar.dkpp_firingSmoke.positionFrom, owner->currentCF.angle, CVector3f(0,0,1)) + owner->currentCF.position;
                clientVar.dkpp_firingSmoke.positionFrom[2] -= owner->currentCF.position[2];
                clientVar.dkpp_firingSmoke.positionTo = clientVar.dkpp_firingSmoke.positionFrom;
                clientVar.dkpp_firingSmoke.direction.set(0,1,0);
                clientVar.dkpp_firingSmoke.direction = rotateAboutAxis(clientVar.dkpp_firingSmoke.direction, owner->currentCF.angle, CVector3f(0,0,1));
                clientVar.dkpp_firingSmoke.pitchTo = 45;
            }
            else
            {*/
                if (weaponID != WEAPON_FLAME_THROWER) nuzzleFlashes[firingNuzzle]->shoot();
                owner->currentCF.position[2] = .25f;
                clientVar.dkpp_firingSmoke.positionFrom = nuzzleFlashes[firingNuzzle]->position * .005f;
                clientVar.dkpp_firingSmoke.positionFrom = rotateAboutAxis(clientVar.dkpp_firingSmoke.positionFrom, owner->currentCF.angle, CVector3f(0,0,1)) + owner->currentCF.position;
                clientVar.dkpp_firingSmoke.positionFrom[2] -= owner->currentCF.position[2];
                clientVar.dkpp_firingSmoke.positionTo = clientVar.dkpp_firingSmoke.positionFrom;
                clientVar.dkpp_firingSmoke.direction.set(0,1,0);
                clientVar.dkpp_firingSmoke.direction = rotateAboutAxis(clientVar.dkpp_firingSmoke.direction, owner->currentCF.angle, CVector3f(0,0,1));
                clientVar.dkpp_firingSmoke.pitchTo = 45;
        //  }

            // On subit un recul
            owner->currentCF.vel -= clientVar.dkpp_firingSmoke.direction * reculVel;

            // On entends ça
            if (owner->fireFrameDelay == 0)
            {
                dksPlay3DSound(sfx_sound, -1, 5, owner->currentCF.position,255);
                owner->fireFrameDelay = 2;
            }

            // On en shot le nb qui faut
            owner->shootShakeDis = -clientVar.dkpp_firingSmoke.direction * reculVel * .5f;
            //for (int i=0;i<nbShot;++i)
            {
                cgame->shoot(clientVar.dkpp_firingSmoke.positionFrom, clientVar.dkpp_firingSmoke.direction, 0, damage, owner, projectileType);
            }

            if (projectileType == PROJECTILE_DIRECT && weaponID != WEAPON_FLAME_THROWER && weaponID != WEAPON_PHOTON_RIFLE)
            {
                CVector3f pos = rotateAboutAxis(ejectingBrass[firingNuzzle]->position * .005f, owner->currentCF.angle, CVector3f(0,0,1)) + owner->currentCF.position - CVector3f(0,0,.25f);
                CVector3f dir = rotateAboutAxis(ejectingBrass[firingNuzzle]->matrix.getUp(), owner->currentCF.angle, CVector3f(0,0,1));
                CVector3f right = rotateAboutAxis(ejectingBrass[firingNuzzle]->matrix.getRight(), owner->currentCF.angle, CVector3f(0,0,1));
                clientVar.dkpp_firingSmoke.positionFrom = pos;
                clientVar.dkpp_firingSmoke.positionTo = pos;
                clientVar.dkpp_firingSmoke.direction = dir;
                clientVar.dkpp_firingSmoke.pitchTo = 0;
                dkpCreateParticleExP(clientVar.dkpp_firingSmoke);
                if (gameVar.r_showCasing) cgame->douilles.push_back(new Douille(pos, dir*(damage+1), right));
            }

            dkpCreateParticleExP(clientVar.dkpp_firingSmoke);
        }
        else
        {
            if (weaponID == WEAPON_KNIVES)
            {
                // On shoot melee
                net_clsv_svcl_player_shoot_melee playerShootMelee;
                playerShootMelee.playerID = owner->playerID;
                bb_clientSend(cscene->client->uniqueClientID, (char*)&playerShootMelee, sizeof(net_clsv_svcl_player_shoot_melee), NET_CLSV_SVCL_PLAYER_SHOOT_MELEE);

                // On entends ça
                if (owner->fireFrameDelay == 0)
                {
                    dksPlay3DSound(sfx_sound, -1, 5, owner->currentCF.position,255);
                    owner->fireFrameDelay = 2;
                    modelAnim = 0;
                    currentFireDelay = fireDelay;
                }
            }
        }
    }
}

// Le server shot le player Melee
void ClientWeapon::shootMelee(Player * owner)
{
    if(owner->meleeWeapon->weaponID == WEAPON_SHIELD ||
        owner->meleeWeapon->weaponID == WEAPON_KNIVES
        )
    {
        nukeFrameID = 0;
        dksPlay3DSound(sfx_sound, -1, 5, owner->currentCF.position,255);
        owner->fireFrameDelay = 2;
        currentFireDelay = fireDelay;
        modelAnim = 0;
    }
}

void ClientWeapon::shoot(net_svcl_player_shoot & playerShoot, Player * owner)
{
    auto cgame = static_cast<ClientGame*>(owner->game);
    modelAnim = 0;

    if (weaponID != WEAPON_FLAME_THROWER)
    {
        nuzzleFlashes[playerShoot.nuzzleID]->shoot();
    }

    CVector3f p1;
    p1[0] = (float)playerShoot.p1[0] / 100.0f;
    p1[1] = (float)playerShoot.p1[1] / 100.0f;
    p1[2] = (float)playerShoot.p1[2] / 100.0f;
    CVector3f p2;
    p2[0] = (float)playerShoot.p2[0] / 100.0f;
    p2[1] = (float)playerShoot.p2[1] / 100.0f;
    p2[2] = (float)playerShoot.p2[2] / 100.0f;
    CVector3f normal;
    normal[0] = (float)playerShoot.normal[0] / 120.0f;
    normal[1] = (float)playerShoot.normal[1] / 120.0f;
    normal[2] = (float)playerShoot.normal[2] / 120.0f;
    clientVar.dkpp_firingSmoke.positionFrom = p1;
    clientVar.dkpp_firingSmoke.positionTo = clientVar.dkpp_firingSmoke.positionFrom;
    clientVar.dkpp_firingSmoke.direction.set(0,1,0);
    clientVar.dkpp_firingSmoke.direction = rotateAboutAxis(clientVar.dkpp_firingSmoke.direction, owner->currentCF.angle, CVector3f(0,0,1));
    clientVar.dkpp_firingSmoke.pitchTo = 45;

    // Un ptit son pour agrémenter tout ça!
    dksPlay3DSound(sfx_sound, -1, 5, clientVar.dkpp_firingSmoke.positionFrom,150);

    dkpCreateParticleExP(clientVar.dkpp_firingSmoke);

    if (projectileType == PROJECTILE_DIRECT && weaponID != WEAPON_FLAME_THROWER)
    {
        if (weaponID != WEAPON_PHOTON_RIFLE)
        {
            CVector3f pos = rotateAboutAxis(ejectingBrass[playerShoot.nuzzleID]->position * .005f, owner->currentCF.angle, CVector3f(0,0,1)) + owner->currentCF.position - CVector3f(0,0,.25f);
            CVector3f dir = rotateAboutAxis(ejectingBrass[playerShoot.nuzzleID]->matrix.getUp(), owner->currentCF.angle, CVector3f(0,0,1));
            CVector3f right = rotateAboutAxis(ejectingBrass[playerShoot.nuzzleID]->matrix.getRight(), owner->currentCF.angle, CVector3f(0,0,1));
            clientVar.dkpp_firingSmoke.positionFrom = pos;
            clientVar.dkpp_firingSmoke.positionTo = pos;
            clientVar.dkpp_firingSmoke.direction = dir;
            clientVar.dkpp_firingSmoke.pitchTo = 0;
            dkpCreateParticleExP(clientVar.dkpp_firingSmoke);
            if (gameVar.r_showCasing)  cgame->douilles.push_back(new Douille(pos, dir*(damage+1), right));
        }
        cgame->spawnImpact(p1, p2, normal, this, damage, owner->teamID);
    }
    else if (weaponID == WEAPON_FLAME_THROWER)
    {
        //--- Spawn fire
        CVector3f firePos;
        float percent = 0;
        for (float i=0;i<=1;i+=.05f)
        {
            firePos = p1 + (p2 - p1) * i;
            dkpCreateParticle(  (firePos+rand(CVector3f(-i*.3f,-i*.3f,0),CVector3f(i*.3f,i*.3f,0))).s,//float *position,
                                (CVector3f(0,0,1) + normal).s,//float *vel,
                                rand(CVector4f(i,0,1-i,0.0f),CVector4f(i,i*.75f,1-i,0.0f)).s,//float *startColor,
                                CVector4f(i,i*.75f,1-i,1.0f).s,//float *endColor,
                                .6f * (i*.5f + .5f),//float startSize,
                                .0f,//float endSize,
                                i/*.5f*/,//float duration,
                                0,//float gravityInfluence,
                                0,//float airResistanceInfluence,
                                rand(0.0f, 30.0f),//float rotationSpeed,
                                clientVar.tex_smoke1,//unsigned int texture,
                                DKP_SRC_ALPHA,//unsigned int srcBlend,
                                DKP_ONE,//unsigned int dstBlend,
                                0);//int transitionFunc);
            dkpCreateParticle(  (firePos+rand(CVector3f(-i*.3f,-i*.3f,0),CVector3f(i*.3f,i*.3f,0))).s,//float *position,
                                (CVector3f(0,0,1) + normal+rand(CVector3f(-.20f,-.20f,0),CVector3f(.20f,.20f,0))).s,//float *vel,
                                rand(CVector4f(i,0,1-i,1.0f),CVector4f(i,i*.75f,1-i,1.0f)).s,//float *startColor,
                                CVector4f(i,i*.75f,1-i,0.0f).s,//float *endColor,
                                .0f,//float startSize,
                                .6f * (i*.5f + .5f),//float endSize,
                                i/*.5f*/,//float duration,
                                0,//float gravityInfluence,
                                0,//float airResistanceInfluence,
                                rand(0.0f, 30.0f),//float rotationSpeed,
                                clientVar.tex_smoke1,//unsigned int texture,
                                DKP_SRC_ALPHA,//unsigned int srcBlend,
                                DKP_ONE,//unsigned int dstBlend,
                                0);//int transitionFunc);
        }
    }
}

//
// On l'update
//
void ClientWeapon::update(float delay)
{
    Weapon::update(delay);
    for (int i=0;i<(int)nuzzleFlashes.size();++i)
    {
        nuzzleFlashes[i]->update(delay);
    }
}
