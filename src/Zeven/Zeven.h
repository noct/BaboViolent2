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

#ifndef ZEVEN_H
#define ZEVEN_H


#define ZEVEN_UNUSED(p) (p)

#include <cstdint>

#include <Zeven/CMatrix.h>
#include <Zeven/CString.h>
#include <Zeven/CVector.h>

struct dkContext;
struct dkConfig
{
    int framePerSecond;
};

dkContext* dkInit(dkConfig config);
void       dkFini(dkContext* ctx);

const int LIMIT_MIN = 1 << 0;
const int LIMIT_MAX = 1 << 1;

enum CMD_RET {
    CR_OK,
    CR_NOTSUPPORTED,
    CR_NOSUCHVAR,
    CR_INVALIDARGS
};

class CStringInterface
{
public:
    virtual void updateString(CString* string, char * newValue) = 0;
        virtual ~CStringInterface()
        {
        }
};

// Les fonction du DKSVAR


/// \brief exécute une commande
///
/// Cette fonction permet d'exécuter une commande passée sous forme d'une chaine de caractères. Présentement, la seul commande valide est 'set' et sa syntaxe est la suivante:
/// set nomDeLaVariable param1 [param2 param3 ...]
/// L'exécution de cette commande tente de remplir les champs de la variable enregistrée nomDeLaVariable avec les paramètres param1 param2 ... fournis.
///
/// \param command chaine de caractères représentant une commande valide
CMD_RET         dksvarCommand(char * command);



/// \brief exécution d'un fichier de configuration
///
/// Cette fonction permet d'exécuter toutes les commandes contenues dans un fichier de configuration.
///
/// \param filename chemin menant au fichier de configuration à exécuter depuis l'endroit où se situe le fichier EXE du programme.
void            dksvarLoadConfig(char * filename);
void            dksvarLoadConfigSVOnly(char * filename);
void            dksvarSaveConfig(char * filename);



/// \name VariableRegistering
///
/// \brief enregistre une variable d'un certain type
/// Ce groupe de fonctions permet d'enregistrer une variable d'un certain type (bool,int,float,CVector2i,CVector2f,CVector3i,CVector3f,CVector4f,CString) en spécifiant si cette variable permet d'être modifiée par l'exécution d'un fichier de configuration.
///
/// \param screenName nom de la variable qui sera associé à la variable elle-même
/// \param defaultValue valeur par défaut de la variable
/// \param true si cette variable permet d'être modifiée par l'exécution d'un fichier de configuration, false sinon
//@{
void            dksvarRegister(const CString &screenName, bool       *defaultValue, bool mConfigBypass);
void            dksvarRegister(const CString &screenName, int        *defaultValue, int minValue,
                                             int maxValue, int flags, bool mConfigBypass);
void            dksvarRegister(const CString &screenName, float     *defaultValue, float minValue,
                                             float maxValue, int flags, bool mConfigBypass);
void            dksvarRegister(const CString &screenName, CVector2i *defaultValue, bool mConfigBypass);
void            dksvarRegister(const CString &screenName, CVector2f *defaultValue, bool mConfigBypass);
void            dksvarRegister(const CString &screenName, CVector3i *defaultValue, bool mConfigBypass);
void            dksvarRegister(const CString &screenName, CVector3f *defaultValue, bool mConfigBypass);
void            dksvarRegister(const CString &screenName, CVector4f *defaultValue, bool mConfigBypass);
void            dksvarRegister(const CString &screenName, CString    *defaultValue, bool mConfigBypass);
//@}


/// \brief désenregistre une variable enregistrée
///
/// Cette fonction permet de désenregistrer une variable enregistrée. La variable correspondant au nom fournis ne sera plus assujettie à des modifications provenant de l'exécution de commandes.
///
/// \param screenName nom de la variable associé à la variable elle-même
void            dksvarUnregister(const CString &screenName);



void            dksvarInit(CStringInterface * stringInterface);
void            dksvarGetFilteredVar(char * varName, char ** array, int size);
void            dksvarGetFormatedVar(char * varName, CString * formatedString);





// Les constantes
#define DKO_BUMP_MAP            0x0001 // Not yet
#define DKO_COLOR_ARRAY         0x0002 // Not yet
#define DKO_DETAIL_MAP          0x0004
#define DKO_DYNAMIC_LIGHTING    0x0008
#define DKO_FORCE_WIREFRAME     0x0010
#define DKO_MULTIPASS           0x0020
#define DKO_SELFILL_MAP         0x0040
#define DKO_SPECULAR            0x0080
#define DKO_SPECULAR_MAP        0x0100
#define DKO_TEXTURE_MAP         0x0200
#define DKO_RENDER_BB           0x0400
#define DKO_RENDER_NODE         0x0800
#define DKO_RENDER_FACE         0x1000
#define DKO_CLAMP_TEXTURE       0x2000


// Les fonction du DKO
int             dkoAddAnimationFromFile(unsigned int modelID, char* filename, char* animationName);
void            dkoAddLight(unsigned int modelID, float *position, float *diffuse, float *specular, float range);
void            dkoBuildOctree(unsigned int modelID);
void            dkoComputeStaticLighting(unsigned int modelID);
void            dkoEnable(unsigned int renderFlags);
void            dkoDeleteModel(unsigned int *modelID);
void            dkoDisable(unsigned int renderFlags);
unsigned int    dkoGetDummy(char * dummyName, unsigned int modelID);
void            dkoGetDummyMatrix(char * dummyName, unsigned int modelID, float * mat);
void            dkoGetDummyMatrix(unsigned int dummyID, unsigned int modelID, float * mat);
void            dkoGetDummyMatrix(char * dummyName, unsigned int modelID, float * mat, short frameID);
void            dkoGetDummyMatrix(unsigned int dummyID, unsigned int modelID, float * mat, short frameID);
void            dkoGetDummyPosition(char * dummyName, unsigned int modelID, float * pos);
void            dkoGetDummyPosition(unsigned int dummyID, unsigned int modelID, float * pos);
void            dkoGetDummyPosition(char * dummyName, unsigned int modelID, float * pos, short frameID);
void            dkoGetDummyPosition(unsigned int dummyID, unsigned int modelID, float * pos, short frameID);
char*           dkoGetDummyName(unsigned int dummyID, unsigned int modelID);
char*           dkoGetLastError();
int             dkoGetNbVertex(unsigned int modelID);
void            dkoGetOABB(unsigned int modelID, float *OABB);
float           dkoGetRadius(unsigned int modelID);
short           dkoGetTotalFrame(unsigned int modelID);
float*          dkoGetVertexArray(unsigned int modelID, float frameID, int & nbVertex, float *vertexArray);
void            dkoInit();
void            dkoInitLightList(unsigned int modelID);
unsigned int    dkoLoadFile(char* filename);
unsigned int    dkoLoadFile(unsigned int modelID);
void            dkoOutputDebugInfo(unsigned int modelID, char *filename);
void            dkoPopRenderState();
void            dkoPushRenderState();
bool            dkoRayIntersection(unsigned int modelID, float *p1, float *p2, float *intersect, float *normal, int &n);
void            dkoRender(unsigned int modelID); // Va renderer automatiquement le premier frame
void            dkoRender(unsigned int modelID, unsigned short frameID); // Sp�ifions un frame
void            dkoRender(unsigned int modelID, float frameID); // Avec interpolation (MaLaDe)
void            dkoShutDown();
bool            dkoSphereIntersection(unsigned int modelID, float *p1, float *p2, float rayon, float *intersect, float *normal, int &n);


/// \brief retourne le temps allou�pour un cycle d'ex�ution
///
/// Cette fonction retourne en fait simplement : 1.0f / le param�re framePerSecond pass��dkcInit. Cette valeur est constante et repr�ente la dur� d'un cycle d'ex�ution de la mise �jour.
///
/// \return 1.0f / le nombre d'update d�ir�par seconde
float           dkcGetElapsedf(dkContext* ctx);



/// \brief retourne le nombre d'image dessin�s �l'�ran pas seconde
///
/// Cette fonction retourne le nombre d'image dessin�s �l'�ran pas seconde. cette valeur peut grandement varier (lors d'un lag par exemple)
///
/// \return le nombre d'image dessin�s �l'�ran pas seconde
float           dkcGetFPS(dkContext* ctx);



/// \brief retourne le nombre de cycles d'ex�utions de mise �jour (update) �oul� depuis un certain rep�e
///
/// Cette fonction retourne le nombre de cycles d'ex�utions de mise �jour (update) �oul� depuis le d�ut de l'ex�ution du programme entier dans le cas o aucun appel �dkcJumpToFrame() n'a ��fait.
/// Dans le cas o dkcJumpToFrame() a ��appel� l'�uation suivante pr�aut avec le dernier appel fait �dkcJumpToFrame():
/// nombre de cycles d'ex�utions de mise �jour (update) �oul� depuis le dernier appel de dkcJumpToFrame() = valeur de retour de dkcGetFrame() - param�re pass�au dernier appel de dkcJumpToFrame()
///
/// \return retourne le nombre de cycles d'ex�utions de mise �jour (update) �oul� depuis un certain rep�e
int32_t         dkcGetFrame(dkContext* ctx);

/// \brief modifie le num�o du cycle d'ex�ution de mise �jour courant
///
/// Cette fonction modifie le num�o du cycle d'ex�ution de mise �jour courant.
///
/// \param frame nouveau num�o du cycle d'ex�ution de mise �jour courant
void            dkcJumpToFrame(dkContext* ctx, int frame);



/// \brief met �jour les rep�es de temps et retourne le nombre de cycle d'ex�ution de mise �jour (update) ��re effectu� avant d'effectuer le prochain rendu
///
/// Cette fonction met �jour les rep�es de temps n�essaire au bon fonctionnement du module et retourne le nombre de cycle d'ex�ution de mise �jour (update) ��re effectu� avant d'effectuer le prochain rendu pour rester conforme au nombre de cycle d'ex�ution de mise �jour �abli par le param�re framePerSecond pass��la fonction dkcInit()
///
/// \return le nombre de cycle d'ex�ution de mise �jour (update) ��re effectu�
int32_t         dkcUpdateTimer(dkContext* ctx);


void            dkcSleep(dkContext* ctx, int32_t ms);


#ifndef DEDICATED_SERVER
#include <Zeven/lib/dkf.h>
#include <Zeven/lib/dkgl.h>
#include <Zeven/lib/dki.h>
#include <Zeven/lib/dkp.h>
#include <Zeven/lib/dks.h>
#include <Zeven/lib/dkt.h>
#include <Zeven/lib/dkw.h>
#endif

#define ZEVEN_SAFE_DELETE(a) if (a) {delete a; a = 0;}
#define ZEVEN_SAFE_DELETE_ARRAY(a) if (a) {delete [] a; a = 0;}

#define ZEVEN_SAFE_ALLOCATE(a, type, size) ZEVEN_SAFE_DELETE_ARRAY(a) a = new type [size];

#define ZEVEN_SUCCEEDED(a) (a > 0) ? true : false
#define ZEVEN_FAILED(a) (a <= 0) ? true : false

#define ZEVEN_SUCCESS 1
#define ZEVEN_FAIL 0

#define ZEVEN_DELETE_VECTOR(a, cpt) for (int cpt=0;cpt<(int)a.size();delete a[cpt++]); a.clear();
#define ZEVEN_VECTOR_CALL(a, cpt, func) for (int cpt=0;cpt<(int)a.size();++cpt) a[cpt]->func;

///---- Prototype
CVector2i dkwGetCursorPos_main();

#endif

