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
/* TCE (c) All rights reserved */


#include <Zeven/Gfx.h>

#ifdef WIN32
#include <windows.h>
#endif

#include <vector>
#include "dkgl.h"



#include <Zeven/CVector.h>


class CParticle
{
public:
    // Sa texture utilisé
    unsigned int texture;

    // La position de la particle
    CPoint3f position;

    // Sa position au dernier frame
    CPoint3f lastPosition;

    // Sa couleur au début
    CColor4f startColor;

    // Sa couleur à la fin
    CColor4f endColor;

    // Sa velocité
    CVector3f vel;

    // Sa vie (0 à 1) (1 étant mort)
    float life;

    // Sa duré de vie, sec / duré
    float fadeSpeed;

    // Sa grosseur au début
    float startSize;

    // Sa grosseur à la fin
    float endSize;

    // Sa densité (1 = fully attracted by gravity)
    float density;

    // Son angle de rotation
    float angle;

    // La vitesse de rotation de son angle
    float rotationSpeed;

    // La fonction de blending utilisé
    unsigned int srcBlend;
    unsigned int dstBlend;

    // Pour la résistance de l'air
    float airResistanceInfluence;

    // La distance avec la camera
    float camDis;

    // Si c'est un billboard
    bool billboard;

    // Son opacity
    float billboardOpacity;

    // Sa distance avant de fader
    float billboardFadeDis;

    float billboardFadeDelay;

    bool toDelete;

    bool isMultipleTexture;
    unsigned int * textureArray;
    int nbFrame;

public:
    // Constructeur / Destructeur
    CParticle(
        float *position,
        float *vel,
        float *startColor,
        float *endColor,
        float startSize,
        float endSize,
        float duration,
        float density,
        float airResistanceInfluence,
        float rotationSpeed,
        unsigned int texture,
        unsigned int srcBlend,
        unsigned int dstBlend,
        int transitionFunc);
    virtual ~CParticle();

    // Pour l'updater
    int update();

    // Pour le rendre
    void render();
};




/* BlendingFactorDest */
#define DKP_ZERO                           0
#define DKP_ONE                            1
#define DKP_SRC_COLOR                      0x0300
#define DKP_ONE_MINUS_SRC_COLOR            0x0301
#define DKP_SRC_ALPHA                      0x0302
#define DKP_ONE_MINUS_SRC_ALPHA            0x0303
#define DKP_DST_ALPHA                      0x0304
#define DKP_ONE_MINUS_DST_ALPHA            0x0305

/* BlendingFactorSrc */
#define DKP_DST_COLOR                      0x0306
#define DKP_ONE_MINUS_DST_COLOR            0x0307
#define DKP_SRC_ALPHA_SATURATE             0x0308

// La class static pour tenir les informations importante sur les particles
class CDkp
{
public:
    // La liste de toute les particles ouais poupé
    static std::vector<CParticle*> particles;

    // La gravité
    static CVector3f gravity;

    // Le delay
    static float delay;

    // La modelview
    static float modelView[16];

    // La position de la camera
    static CVector3f camPos;

    // La DP pour une particle
//	static unsigned int dpSprite;
    static float* vertexArray;
    static float* normalArray;
    static float* texCoordArray;

    static unsigned int lastTexture;

    // La densité de l'air (en kPa)
    static float airDensity;

    // Si on doit les sorter ou pas
    static bool sorting;

    // Pour sorter où on est rendu
    static std::vector<CParticle*>::size_type currentSortInteration;

public:
};



#include <Zeven/CMatrix.h>

// Les trucs statics
std::vector<CParticle*> CDkp::particles;
CVector3f CDkp::gravity = CVector3f(0,0,-9.8f);
float CDkp::delay = 0;
float CDkp::modelView[16] = {0};
CVector3f CDkp::camPos;
//unsigned int CDkp::dpSprite = 0;
float* CDkp::vertexArray = 0;
float* CDkp::normalArray = 0;
float* CDkp::texCoordArray = 0;
float CDkp::airDensity = 103.4f;
bool CDkp::sorting = false;
std::vector<CParticle*>::size_type CDkp::currentSortInteration = 0;
unsigned int CDkp::lastTexture = 0;



//
// Pour créer une billboard static (mais trié comme les particule) pour faire du gazon à terre surtout
//
void            dkpCreateBillboard( CVector3f & positionFrom,
                                    CVector3f & positionTo,
                                    float fadeSpeed,
                                    float fadeOutDistance,
                                    float size,
                                    CColor4f & color,
                                    unsigned int textureID,
                                    unsigned int srcBlend,
                                    unsigned int dstBlend)
{
    CParticle * newPart = new CParticle(
        rand(positionFrom, positionTo).s, CVector3f().s, color.s, color.s, size, size, 1,
        0, 0, 0, textureID, srcBlend, dstBlend, 0);

    newPart->billboard = true;
    newPart->billboardFadeDis = fadeOutDistance;
    newPart->billboardFadeDelay = 1.0f / fadeSpeed;
    newPart->angle = 0;

    // On l'ajoute à notre liste
    // On va tout suite l'ajouter au bon endroit pour pas avoir des tri important à faire tantot
    if (CDkp::particles.size() > 1 && CDkp::sorting) // Il faut qu'il y en ai au moins 1
    {
        std::vector<CParticle*>::size_type i;
        for (i=0;i<CDkp::particles.size();i++)
        {
            CParticle * p1 = CDkp::particles.at(i);
            if (newPart->camDis > p1->camDis)
            {
                CDkp::particles.insert(CDkp::particles.begin()+i, newPart);
                return;
            }
        }
        if (i==CDkp::particles.size())
        {
            CDkp::particles.push_back(newPart);
        }
    }
    else
        CDkp::particles.push_back(newPart);
}



//
// Pour créer une particle
//
void            dkpCreateParticle(  float *position,
                                    float *vel,
                                    float *startColor,
                                    float *endColor,
                                    float startSize,
                                    float endSize,
                                    float duration,
                                    float density,
                                    float airResistanceInfluence,
                                    float rotationSpeed,
                                    unsigned int texture,
                                    unsigned int srcBlend,
                                    unsigned int dstBlend,
                                    int transitionFunc)
{
    CParticle * newPart = new CParticle(
        position, vel, startColor, endColor, startSize, endSize, duration,
        density, airResistanceInfluence, rotationSpeed, texture, srcBlend, dstBlend, transitionFunc);

    // On l'ajoute à notre liste
    // On va tout suite l'ajouter au bon endroit pour pas avoir des tri important à faire tantot
    if (CDkp::particles.size() > 1 && CDkp::sorting) // Il faut qu'il y en ai au moins 1
    {
        std::vector<CParticle*>::size_type i;
        for (i=0;i<CDkp::particles.size();i++)
        {
            CParticle * p1 = CDkp::particles.at(i);
            if (newPart->camDis > p1->camDis)
            {
                CDkp::particles.insert(CDkp::particles.begin()+i, newPart);
                break;
            }
        }
        if (i==CDkp::particles.size())
        {
            CDkp::particles.push_back(newPart);
        }
    }
    else
        CDkp::particles.push_back(newPart);
}



//
// Pour créer une particle avec plein d'information de random dessus
//
void            dkpCreateParticleEx(const CVector3f & positionFrom,
    const CVector3f & positionTo,
    const CVector3f & direction,
                                    float speedFrom,
                                    float speedTo,
                                    float pitchFrom,
                                    float pitchTo,
                                    float startSizeFrom,
                                    float startSizeTo,
                                    float endSizeFrom,
                                    float endSizeTo,
                                    float durationFrom,
                                    float durationTo,
                                    const CColor4f & startColorFrom,
                                    const CColor4f & startColorTo,
                                    const CColor4f & endColorFrom,
                                    const CColor4f & endColorTo,
                                    float angleFrom,
                                    float angleTo,
                                    float angleSpeedFrom,
                                    float angleSpeedTo,
                                    float gravityInfluence,
                                    float airResistanceInfluence,
                                    unsigned int particleCountFrom,
                                    unsigned int particleCountTo,
                                    unsigned int *texture,
                                    int textureFrameCount,
                                    unsigned int srcBlend,
                                    unsigned int dstBlend)
{
    // On défini combient on en emet
    int total = rand((int)particleCountFrom, (int)particleCountTo);

    // On trouve le right et le up vector (la direction étant notre front)
    CVector3f right, up;
    createRightUpVectors(right, direction, up);

    for (int p=0;p<total;p++)
    {
        CVector3f vel = direction * rand(speedFrom, speedTo);
        vel = rotateAboutAxis(vel, rand(pitchFrom, pitchTo), right);
        vel = rotateAboutAxis(vel, rand((float)0, (float)360), direction);

        // On cré notre particule
        CParticle * newPart = new CParticle(
            rand(positionFrom, positionTo).s,
            vel.s,
            rand(startColorFrom, startColorTo).s,
            rand(endColorFrom, endColorTo).s,
            rand(startSizeFrom, startSizeTo),
            rand(endSizeFrom, endSizeTo),
            rand(durationFrom, durationTo),
            gravityInfluence,
            airResistanceInfluence,
            rand(angleSpeedFrom, angleSpeedTo),
            (texture) ? *texture : 0,
            srcBlend,
            dstBlend,
            0);
        newPart->angle = rand(angleFrom, angleTo);

        if (textureFrameCount > 1 && texture)
        {
            newPart->isMultipleTexture = true;
            newPart->nbFrame = textureFrameCount;
            newPart->textureArray = texture;
        }

        // On l'ajoute à notre liste
        // On va tout suite l'ajouter au bon endroit pour pas avoir des tri important à faire tantot
        if (CDkp::particles.size() > 1 && CDkp::sorting) // Il faut qu'il y en ai au moins 1
        {
            std::vector<CParticle*>::size_type i;
            for (i=0;i<CDkp::particles.size();i++)
            {
                CParticle * p1 = CDkp::particles.at(i);
                if (newPart->camDis > p1->camDis)
                {
                    CDkp::particles.insert(CDkp::particles.begin()+i, newPart);
                    break;
                }
            }
            if (i==CDkp::particles.size())
            {
                CDkp::particles.push_back(newPart);
            }
        }
        else
            CDkp::particles.push_back(newPart);
    }
}



//
// Pour créer une particle ex à partir d'un preset
//
void            dkpCreateParticleExP(dkp_preset & preset)
{
    dkpCreateParticleEx(preset.positionFrom,
                        preset.positionTo,
                        preset.direction,
                        preset.speedFrom,
                        preset.speedTo,
                        preset.pitchFrom,
                        preset.pitchTo,
                        preset.startSizeFrom,
                        preset.startSizeTo,
                        preset.endSizeFrom,
                        preset.endSizeTo,
                        preset.durationFrom,
                        preset.durationTo,
                        preset.startColorFrom,
                        preset.startColorTo,
                        preset.endColorFrom,
                        preset.endColorTo,
                        preset.angleFrom,
                        preset.angleTo,
                        preset.angleSpeedFrom,
                        preset.angleSpeedTo,
                        preset.gravityInfluence,
                        preset.airResistanceInfluence,
                        preset.particleCountFrom,
                        preset.particleCountTo,
                        preset.texture,
                        preset.textureFrameCount,
                        preset.srcBlend,
                        preset.dstBlend);
}



//
// Pour initialiser
//
void            dkpInit()
{
    CDkp::vertexArray = new float [6 * 3];
    CDkp::vertexArray[0] = 0;
    CDkp::vertexArray[1] = 0;
    CDkp::vertexArray[2] = 0;

    CDkp::vertexArray[3] = -1;
    CDkp::vertexArray[4] = 1;
    CDkp::vertexArray[5] = 0;

    CDkp::vertexArray[6] = -1;
    CDkp::vertexArray[7] = -1;
    CDkp::vertexArray[8] = 0;

    CDkp::vertexArray[9] = 1;
    CDkp::vertexArray[10] = -1;
    CDkp::vertexArray[11] = 0;

    CDkp::vertexArray[12] = 1;
    CDkp::vertexArray[13] = 1;
    CDkp::vertexArray[14] = 0;

    CDkp::vertexArray[15] = -1;
    CDkp::vertexArray[16] = 1;
    CDkp::vertexArray[17] = 0;

    CDkp::normalArray = new float [6 * 3];
    CDkp::normalArray[0] = 0;
    CDkp::normalArray[1] = 0;
    CDkp::normalArray[2] = 1;

    CDkp::normalArray[3] = -1;
    CDkp::normalArray[4] = 1;
    CDkp::normalArray[5] = 0;

    CDkp::normalArray[6] = -1;
    CDkp::normalArray[7] = -1;
    CDkp::normalArray[8] = 0;

    CDkp::normalArray[9] = 1;
    CDkp::normalArray[10] = -1;
    CDkp::normalArray[11] = 0;

    CDkp::normalArray[12] = 1;
    CDkp::normalArray[13] = 1;
    CDkp::normalArray[14] = 0;

    CDkp::normalArray[15] = -1;
    CDkp::normalArray[16] = 1;
    CDkp::normalArray[17] = 0;

    CDkp::texCoordArray =  new float [6 * 2];
    CDkp::texCoordArray[0] = .5f;
    CDkp::texCoordArray[1] = .5f;

    CDkp::texCoordArray[2] = 0;
    CDkp::texCoordArray[3] = 1;

    CDkp::texCoordArray[4] = 0;
    CDkp::texCoordArray[5] = 0;

    CDkp::texCoordArray[6] = 1;
    CDkp::texCoordArray[7] = 0;

    CDkp::texCoordArray[8] = 1;
    CDkp::texCoordArray[9] = 1;

    CDkp::texCoordArray[10] = 0;
    CDkp::texCoordArray[11] = 1;
}

//
// Pour afficher le tout
//
void dkpRender()
{
    CDkp::lastTexture = 0;
    int i;
    int j;

    // On pogne la model view matrix
    glGetFloatv(GL_MODELVIEW_MATRIX, CDkp::modelView);

    // On transpose la matrice
    for (i=0;i<4;i++)
    {
        for (j=i;j<4;j++)
        {
            float tmp;
            tmp = CDkp::modelView[i*4+j];
            CDkp::modelView[i*4+j] = CDkp::modelView[j*4+i];
            CDkp::modelView[j*4+i] = tmp;
        }
    }
    CDkp::camPos = CVector3f(CDkp::modelView[3], CDkp::modelView[7], CDkp::modelView[11]);
    CDkp::modelView[3] = 0;
    CDkp::modelView[7] = 0;
    CDkp::modelView[11] = 0;
    CDkp::modelView[12] = 0;
    CDkp::modelView[13] = 0;
    CDkp::modelView[14] = 0;
    CDkp::modelView[15] = 1;

    CMatrix3x3f matView(
        CDkp::modelView[0], CDkp::modelView[1], CDkp::modelView[2],
        CDkp::modelView[4], CDkp::modelView[5], CDkp::modelView[6],
        CDkp::modelView[8], CDkp::modelView[9], CDkp::modelView[10]);

    CDkp::camPos = matView.TransformVectorToParent(CDkp::camPos);
    CDkp::camPos = -CDkp::camPos;

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT);
        glEnable(GL_RESCALE_NORMAL);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_ALWAYS, 0);
        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, CDkp::vertexArray);
        glNormalPointer(GL_FLOAT, 0, CDkp::normalArray);
        glTexCoordPointer(2, GL_FLOAT, 0, CDkp::texCoordArray);
        // On render les particles, apres tout c'est la job de ce API
        for (i=0;i<(int)CDkp::particles.size();i++)
        {
            CParticle * particle = CDkp::particles.at(i);
            particle->render();
        }
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    glPopAttrib();
}



//
// Resetter les particules
//
void dkpReset()
{
    for (int i=0;i<(int)CDkp::particles.size();i++)
    {
        CParticle * particle = CDkp::particles.at(i);
        if (particle) delete particle;
    }
    CDkp::particles.clear();
}



//
// Pour setter la pression atmospherique
//
void dkpSetAirDensity(float airDensity)
{
    CDkp::airDensity = airDensity;
}



//
// Pour setter la direction et la force de la gravité
//
void dkpSetGravity(float *vel)
{
    CDkp::gravity = CVector3f(vel);
}



//
// Si on doit sorter ou pas
//
void dkpSetSorting(bool sort)
{
    CDkp::sorting = sort;
}



//
// Pour effacer le tout, à la fin du programme
//
void dkpShutDown()
{
    for (int i=0;i<(int)CDkp::particles.size();i++)
    {
        CParticle * particle = CDkp::particles.at(i);
        if (particle) delete particle;
    }
    CDkp::particles.clear();

    if (    CDkp::vertexArray) delete []    CDkp::vertexArray;  CDkp::vertexArray = 0;
    if (    CDkp::normalArray) delete []    CDkp::normalArray;  CDkp::normalArray = 0;
    if (    CDkp::texCoordArray) delete []  CDkp::texCoordArray;    CDkp::texCoordArray = 0;
}

//
// Pour updater le tout
//
int dkpUpdate(float delay)
{
    // Le nb de particle
    int particleCount=0;

    // On update notre delait général
    CDkp::delay = delay;

    // On les updates normalement avant tout
    for (int i=0;i<(int)CDkp::particles.size();i++)
    {
        CParticle * particle = CDkp::particles.at(i);
        if (!particle->update())
        {
            // On doit effacer cette particule
            delete particle;
            CDkp::particles.erase(CDkp::particles.begin() + i);
            i--;
            continue;
        }
        else
            particleCount++;
    }

    std::vector<CParticle*>::size_type lastIndex = std::vector<CParticle*>::size_type(-1);
    int curInteration = 0;
    if (CDkp::currentSortInteration >= CDkp::particles.size()-1) CDkp::currentSortInteration = 0;

    // Maintenant, il faut trier ses particles par rapport à la camera
    if (CDkp::particles.size() > 1 && CDkp::sorting) // Il faut qu'il y en ai au moins 2
    {
        for (std::vector<CParticle*>::size_type i=CDkp::currentSortInteration;i<CDkp::particles.size()-1;i++)
        {
            curInteration++;

            // On ne veut pas surcharger
            if (curInteration >= 2000)
            {
                CDkp::currentSortInteration = i;
                break;
            }

            CParticle * p1 = CDkp::particles.at(i);
            CParticle * p2 = CDkp::particles.at(i+1);
            if (p1->camDis < p2->camDis)
            {
                // Alors il faut switcher les deux
                CDkp::particles.erase(CDkp::particles.begin()+i+1);
                CDkp::particles.insert(CDkp::particles.begin()+i, p2);
                if (lastIndex == -1) lastIndex = i;
                if (i>0) i-=2;
                continue;
            }
            else if (lastIndex != -1)
            {
                i = lastIndex - 1;
                lastIndex = std::vector<CParticle*>::size_type(-1);
                continue;
            }

            if (i == CDkp::particles.size()-2)
            {
                CDkp::currentSortInteration = 0;
                break;
            }
        }
    }

    return particleCount;
}


//
// Constructeur / Destructeur
//
CParticle::CParticle(
    float *mposition,
    float *mvel,
    float *mstartColor,
    float *mendColor,
    float mstartSize,
    float mendSize,
    float mduration,
    float mdensity,
    float mairResistanceInfluence,
    float mrotationSpeed,
    unsigned int mtexture,
    unsigned int msrcBlend,
    unsigned int mdstBlend,
    int mtransitionFunc)
{
    texture = mtexture;
    position = CPoint3f(mposition);
    lastPosition = position;
    startColor = CColor4f(mstartColor);
    endColor = CColor4f(mendColor);
    vel = CVector3f(mvel);
    life = 0;
    fadeSpeed = 1.0f / mduration;
    startSize = mstartSize;
    endSize = mendSize;
    density = mdensity;
    angle = (float)(rand() % 36000) / 100.0f;
    rotationSpeed = mrotationSpeed;
    srcBlend = msrcBlend;
    dstBlend = mdstBlend;
    //  transitionFunc = mtransitionFunc;  // Pas utilisé
    (void)mtransitionFunc;// No need to show the warning about unreferenced parameter
    airResistanceInfluence = mairResistanceInfluence;
    billboard = false;
    billboardOpacity = 0;
    billboardFadeDis = 16;
    billboardFadeDelay = 1;
    toDelete = false;

    isMultipleTexture = false;
    textureArray = 0;
    nbFrame = 1;

    // Ici on la calcul tout suite pour pouvoir la placer tout suite à bonne place dans le array
    if(CDkp::sorting) camDis = dot((CDkp::camPos - position), (CDkp::camPos - position));
}

CParticle::~CParticle()
{
}



//
// Pour l'updater
//
int CParticle::update()
{
    if(billboard)
    {
        // On pogne sa distance
        camDis = distanceSquared(CDkp::camPos, position);

        if(toDelete)
        {
            billboardOpacity -= CDkp::delay * billboardFadeDelay;
            if(billboardOpacity <= 0) return 0;
        }
        else
        {
            if(camDis >= billboardFadeDis * billboardFadeDis)
            {
                toDelete = true;
            }
            else
            {
                if(billboardOpacity < 1)
                {
                    billboardOpacity += CDkp::delay * billboardFadeDelay;
                    if(billboardOpacity > 1) billboardOpacity = 1;
                }
            }
        }
    }
    else
    {
        // On diminu sa vie
        life += fadeSpeed * CDkp::delay;

        // On check si elle est encore vivante
        if(life >= 1) return 0;

        // Sa rotation
        angle += rotationSpeed * CDkp::delay;

        // On affecte la gravité
        vel += CDkp::gravity * density * CDkp::delay;

        // On l'affecte à la densité de l'air

        // On anim finalement sa position
        position += vel * CDkp::delay;

        // Sa distance avec notre camera
        if(CDkp::sorting) camDis = distanceSquared(CDkp::camPos, position);
    }

    return 1;
}

//
// Pour le rendre
//
void CParticle::render()
{
    // On la positionne
    glPushMatrix();

    glBlendFunc(srcBlend, dstBlend);

    CColor4f curColor = startColor + (endColor - startColor) * life;

    if(billboard) curColor[3] *= billboardOpacity;

    float sizeTime = startSize + (endSize - startSize) * life;
    glTranslatef(position[0], position[1], position[2]);
    glMultMatrixf(CDkp::modelView);
    glScalef(sizeTime, sizeTime, sizeTime);
    glRotatef(angle, 0, 0, 1);

    if(isMultipleTexture)
    {
        int currentFrame = (int)(nbFrame*life);
        int nextFrame = currentFrame + 1;
        float midFrame = (float)nbFrame * life - (float)currentFrame;
        if(nextFrame >= nbFrame) nextFrame = currentFrame;

        glBindTexture(GL_TEXTURE_2D, textureArray[currentFrame]);
        glColor4fv((curColor*CColor4f(1, 1, 1, 1 - midFrame)).s);
        glPushAttrib(GL_ENABLE_BIT);

        if(dstBlend == DKP_ONE || dstBlend == DKP_ZERO)
        {
            glDisable(GL_FOG);
            glDisable(GL_LIGHTING);
        }

        glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

        glBindTexture(GL_TEXTURE_2D, textureArray[nextFrame]);
        glColor4fv((curColor*CColor4f(1, 1, 1, midFrame)).s);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 6);
    }
    else
    {
        if(CDkp::lastTexture != texture) glBindTexture(GL_TEXTURE_2D, texture);
        CDkp::lastTexture = texture;
        glColor4fv(curColor.s);
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_LIGHTING);
        if(dstBlend == DKP_ONE || dstBlend == DKP_ZERO)
        {
            glDisable(GL_FOG);
            glDisable(GL_LIGHTING);
        }
        glDrawArrays(GL_TRIANGLE_FAN, 0, 6);
        glPopAttrib();
    }
    glPopMatrix();
}
