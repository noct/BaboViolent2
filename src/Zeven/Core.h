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

#ifndef ZEVEN_CORE_H
#define ZEVEN_CORE_H

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



/// \brief retourne le temps allouêpour un cycle d'exêution
///
/// Cette fonction retourne en fait simplement : 1.0f / le paramêre framePerSecond passêêdkcInit. Cette valeur est constante et reprêente la durê d'un cycle d'exêution de la mise êjour.
///
/// \return 1.0f / le nombre d'update dêirêpar seconde
float           dkcGetElapsedf(dkContext* ctx);



/// \brief retourne le nombre d'image dessinês êl'êran pas seconde
///
/// Cette fonction retourne le nombre d'image dessinês êl'êran pas seconde. cette valeur peut grandement varier (lors d'un lag par exemple)
///
/// \return le nombre d'image dessinês êl'êran pas seconde
float           dkcGetFPS(dkContext* ctx);



/// \brief retourne le nombre de cycles d'exêutions de mise êjour (update) êoulê depuis un certain repêe
///
/// Cette fonction retourne le nombre de cycles d'exêutions de mise êjour (update) êoulê depuis le dêut de l'exêution du programme entier dans le cas o aucun appel êdkcJumpToFrame() n'a êêfait.
/// Dans le cas o dkcJumpToFrame() a êêappelê l'êuation suivante prêaut avec le dernier appel fait êdkcJumpToFrame():
/// nombre de cycles d'exêutions de mise êjour (update) êoulê depuis le dernier appel de dkcJumpToFrame() = valeur de retour de dkcGetFrame() - paramêre passêau dernier appel de dkcJumpToFrame()
///
/// \return retourne le nombre de cycles d'exêutions de mise êjour (update) êoulê depuis un certain repêe
int32_t         dkcGetFrame(dkContext* ctx);

/// \brief modifie le numêo du cycle d'exêution de mise êjour courant
///
/// Cette fonction modifie le numêo du cycle d'exêution de mise êjour courant.
///
/// \param frame nouveau numêo du cycle d'exêution de mise êjour courant
void            dkcJumpToFrame(dkContext* ctx, int frame);



/// \brief met êjour les repêes de temps et retourne le nombre de cycle d'exêution de mise êjour (update) êêre effectuê avant d'effectuer le prochain rendu
///
/// Cette fonction met êjour les repêes de temps nêessaire au bon fonctionnement du module et retourne le nombre de cycle d'exêution de mise êjour (update) êêre effectuê avant d'effectuer le prochain rendu pour rester conforme au nombre de cycle d'exêution de mise êjour êabli par le paramêre framePerSecond passêêla fonction dkcInit()
///
/// \return le nombre de cycle d'exêution de mise êjour (update) êêre effectuê
int32_t         dkcUpdateTimer(dkContext* ctx);


void            dkcSleep(dkContext* ctx, int32_t ms);

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

