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

#include <Zeven/Core.h>
#include <SDL2/SDL.h>
#include <vector>
#include <fstream>

struct dkContext
{
    struct dkc
    {
        int      framePerSecond;
        uint64_t frequency;
        float    perSecond;
        int32_t  frame;
        float    fps;

        uint64_t lastFrameCount;
        float    elapsedf;
        float    currentFrameDelay;
        int      oneSecondFrameCound;
        float    oneSecondElapsedcpt;
    } time;
};

static void dkcInit(dkContext* ctx, dkConfig config)
{
    ctx->time.framePerSecond = config.framePerSecond;
    ctx->time.frequency = SDL_GetPerformanceFrequency();
    ctx->time.perSecond = 1.0f / (float)(ctx->time.framePerSecond);
    ctx->time.frame = 0;
    ctx->time.fps = 0;

    ctx->time.lastFrameCount = 0;
    ctx->time.elapsedf = 0;
    ctx->time.currentFrameDelay = 0;
    ctx->time.oneSecondFrameCound = 0;
    ctx->time.oneSecondElapsedcpt = 0;
}

float dkcGetElapsedf(dkContext* ctx)
{
    return ctx->time.perSecond;
}

float dkcGetFPS(dkContext* ctx)
{
    return ctx->time.fps;
}

int32_t dkcGetFrame(dkContext* ctx)
{
    return ctx->time.frame;
}

void dkcJumpToFrame(dkContext* ctx, int frame)
{
    ctx->time.frame = frame;
    ctx->time.currentFrameDelay = 0;
    ctx->time.lastFrameCount=0;
    ctx->time.elapsedf=0;
    ctx->time.currentFrameDelay=0;
    ctx->time.fps = 0;
    ctx->time.oneSecondFrameCound = 0;
    ctx->time.oneSecondElapsedcpt = 0;
}

int32_t dkcUpdateTimer(dkContext* ctx)
{
    // On prend le nombre de tick du CPU
    uint64_t lGetTickCount = SDL_GetPerformanceCounter();

    // On update le timer
    double elapsedd = (double)((ctx->time.lastFrameCount) ? lGetTickCount - ctx->time.lastFrameCount : 0) / (double)ctx->time.frequency;
    ctx->time.lastFrameCount = lGetTickCount;
    ctx->time.elapsedf = (float)elapsedd;
    ctx->time.currentFrameDelay += ctx->time.elapsedf;
    int32_t nbFrameAdded=0;
    while (ctx->time.currentFrameDelay >= ctx->time.perSecond)
    {
        ctx->time.currentFrameDelay -= ctx->time.perSecond;
        ctx->time.frame++;
        nbFrameAdded++;
    }

    // On update pour le fps
    ctx->time.oneSecondElapsedcpt += ctx->time.elapsedf;
    ctx->time.oneSecondFrameCound++;
    while (ctx->time.oneSecondElapsedcpt >= 1)
    {
        ctx->time.oneSecondElapsedcpt -= 1;
        ctx->time.fps = (float)ctx->time.oneSecondFrameCound;
        ctx->time.oneSecondFrameCound = 0;
    }

    // On retourne le nombre de frame �animer
    return nbFrameAdded;
}

void dkcSleep(dkContext* ctx, int32_t ms)
{
    ZEVEN_UNUSED(ctx);
    SDL_Delay(ms);
}




class CStringInterface;

class CSVType
{
public:
    // Son nom à être affiché
    CString variableName;

    // Est-ce que cette variable peut être bypassé par le fichier config
    bool configBypass;

public:
    // Pour loader d'un fichier config
    virtual void loadConfig(std::ifstream & ficIn) = 0;

    // Pour saver d'un fichier config
    virtual void saveConfig(std::ofstream & ficOut) = 0;

    // Pour setter sa valeur avec les params d'un string
    virtual bool setValue(const CString & paramsT) = 0;

    // Pour obtenir la valeur en string
    virtual CString getValue() = 0;
};

// les types de variables
class CSVBool : public CSVType
{
public:
    bool *value;
    CSVBool(const CString &screenName, bool *defaultValue, bool mConfigBypass) {
        variableName=screenName;value=defaultValue;configBypass=mConfigBypass;}
    void loadConfig(std::ifstream & ficIn){
        char tmp[16];
        ficIn >> tmp;
        if (configBypass)
        {
            if (stricmp(tmp, "false") == 0)
            {
                *value = false;
            }
            else if (stricmp(tmp, "true") == 0)
            {
                *value = true;
            }
        }
    }
    void saveConfig(std::ofstream & ficOut)
    {
        if (*value) ficOut << "true";
        else ficOut << "false";
    }
    bool setValue(const CString & paramsT){
        CString params = paramsT;
        CString param1 = params.getFirstToken(' ');
        param1.trim('\"');
        if (param1 == "false") *value = false;
        else if (param1 == "true") *value = true;
        else return false; // Petit m'essage d'erreur
        return true;
    }
    CString getValue()
    {
        if (*value == true) return "true";
        else return "false";
    }
};
class CSVInt : public CSVType
{
public:
    int *value;
    CSVInt(const CString &screenName, int *defaultValue, int minValue, int maxValue, int flags,
        bool mConfigBypass): minV(minValue), maxV(maxValue), defaultV(*defaultValue), limitsFlags(flags)
    {
        variableName=screenName;
        value=defaultValue;
        configBypass=mConfigBypass;
    }
    void loadConfig(std::ifstream & ficIn){
        int tmp;
        ficIn >> tmp;
        if (configBypass)
        {
            if ( ( (limitsFlags & LIMIT_MIN) && (tmp < minV) ) ||
                ( (limitsFlags & LIMIT_MAX) && (tmp > maxV) ) )
                *value = defaultV;
            else
                *value = tmp;
        }
    }
    void saveConfig(std::ofstream & ficOut)
    {
        ficOut << *value;
    }
    bool setValue(const CString & paramsT)
    {
        CString params = paramsT;
        CString param1 = params.getFirstToken(' ');
        param1.trim('\"');
        int tmp = param1.toInt();
        if ( ( (limitsFlags & LIMIT_MIN) && (tmp < minV) ) ||
            ( (limitsFlags & LIMIT_MAX) && (tmp > maxV) ) )
            return false;
        *value = tmp;
        return true;
    }
    CString getValue()
    {
        return CString("%i", *value);
    }
private:
    const int minV, maxV, defaultV, limitsFlags;
private:
    // To prevent generation of assignement operator
    CSVInt & operator= (const CSVInt &);
};
class CSVFloat : public CSVType
{
public:
    float *value;
    CSVFloat(const CString &screenName, float *defaultValue, float minValue, float maxValue, int flags,
        bool mConfigBypass): minV(minValue), maxV(maxValue), defaultV(*defaultValue), limitsFlags(flags)
    {
        variableName=screenName;
        value=defaultValue;
        configBypass=mConfigBypass;
    }
    void loadConfig(std::ifstream & ficIn)
    {
        float tmp;
        ficIn >> tmp;
        if (configBypass)
        {
            if ( ( (limitsFlags & LIMIT_MIN) && (tmp < minV) ) ||
                ( (limitsFlags & LIMIT_MAX) && (tmp > maxV) ) )
                *value = defaultV;
            else
                *value = tmp;
        }
    }
    void saveConfig(std::ofstream & ficOut)
    {
        ficOut << *value;
    }
    bool setValue(const CString & paramsT){
        CString params = paramsT;
        CString param1 = params.getFirstToken(' ');
        param1.trim('\"');
        float tmp = param1.toFloat();
        if ( ( (limitsFlags & LIMIT_MIN) && (tmp < minV) ) ||
            ( (limitsFlags & LIMIT_MAX) && (tmp > maxV) ) )
            return false;
        *value = tmp;
        return true;
    }
    CString getValue()
    {
        return CString("%f", *value);
    }
private:
    const float minV, maxV, defaultV;
    const int limitsFlags;
private:
    // To prevent generation of assignement operator
    CSVFloat & operator= (const CSVFloat &);
};
class CSVVector2i : public CSVType
{
public:
    CVector2i *value;
    CSVVector2i(const CString &screenName, CVector2i *defaultValue, bool mConfigBypass) {
        variableName=screenName;value=defaultValue;configBypass=mConfigBypass;}
    void loadConfig(std::ifstream & ficIn){
        CVector2i tmp;
        ficIn >> tmp[0];
        ficIn >> tmp[1];
        if (configBypass)
        {
            *value = tmp;
        }
    }
    void saveConfig(std::ofstream & ficOut)
    {
        ficOut << (*value)[0] << " ";
        ficOut << (*value)[1];
    }
    bool setValue(const CString & paramsT){
        CString params = paramsT;
        CString param1 = params.getFirstToken(' ');
        CString param2 = params.getFirstToken(' ');
        param1.trim('\"');
        param2.trim('\"');
        *value = CVector2i(param1.toInt(), param2.toInt());
        return true;
    }
    CString getValue()
    {
        return CString("%i %i", (*value)[0], (*value)[1]);
    }
};
class CSVVector2f : public CSVType
{
public:
    CVector2f *value;
    CSVVector2f(const CString &screenName, CVector2f *defaultValue, bool mConfigBypass) {
        variableName=screenName;value=defaultValue;configBypass=mConfigBypass;}
    void loadConfig(std::ifstream & ficIn){
        CVector2f tmp;
        ficIn >> tmp[0];
        ficIn >> tmp[1];
        if (configBypass)
        {
            *value = tmp;
        }
    }
    void saveConfig(std::ofstream & ficOut)
    {
        ficOut << (*value)[0] << " ";
        ficOut << (*value)[1];
    }
    bool setValue(const CString & paramsT){
        CString params = paramsT;
        CString param1 = params.getFirstToken(' ');
        CString param2 = params.getFirstToken(' ');
        param1.trim('\"');
        param2.trim('\"');
        *value = CVector2f(param1.toFloat(), param2.toFloat());
        return true;
    }
    CString getValue()
    {
        return CString("%f %f", (*value)[0], (*value)[1]);
    }
};
class CSVVector3i : public CSVType
{
public:
    CVector3i *value;
    CSVVector3i(const CString &screenName, CVector3i *defaultValue, bool mConfigBypass) {
        variableName=screenName;value=defaultValue;configBypass=mConfigBypass;}
    void loadConfig(std::ifstream & ficIn){
        CVector3i tmp;
        ficIn >> tmp[0];
        ficIn >> tmp[1];
        ficIn >> tmp[2];
        if (configBypass)
        {
            *value = tmp;
        }
    }
    void saveConfig(std::ofstream & ficOut)
    {
        ficOut << (*value)[0] << " ";
        ficOut << (*value)[1] << " ";
        ficOut << (*value)[2];
    }
    bool setValue(const CString & paramsT){
        CString params = paramsT;
        CString param1 = params.getFirstToken(' ');
        CString param2 = params.getFirstToken(' ');
        CString param3 = params.getFirstToken(' ');
        param1.trim('\"');
        param2.trim('\"');
        param3.trim('\"');
        *value = CVector3i(param1.toInt(), param2.toInt(), param3.toInt());
        return true;
    }
    CString getValue()
    {
        return CString("%i %i %i", (*value)[0], (*value)[1], (*value)[2]);
    }
};
class CSVVector3f : public CSVType
{
public:
    CVector3f *value;
    CSVVector3f(const CString &screenName, CVector3f *defaultValue, bool mConfigBypass) {
        variableName=screenName;value=defaultValue;configBypass=mConfigBypass;}
    void loadConfig(std::ifstream & ficIn){
        CVector3f tmp;
        ficIn >> tmp[0];
        ficIn >> tmp[1];
        ficIn >> tmp[2];
        if (configBypass)
        {
            *value = tmp;
        }
    }
    void saveConfig(std::ofstream & ficOut)
    {
        ficOut << (*value)[0] << " ";
        ficOut << (*value)[1] << " ";
        ficOut << (*value)[2];
    }
    bool setValue(const CString & paramsT){
        CString params = paramsT;
        CString param1 = params.getFirstToken(' ');
        CString param2 = params.getFirstToken(' ');
        CString param3 = params.getFirstToken(' ');
        param1.trim('\"');
        param2.trim('\"');
        param3.trim('\"');
        *value = CVector3f(param1.toFloat(), param2.toFloat(), param3.toFloat());
        return true;
    }
    CString getValue()
    {
        return CString("%f %f %f", (*value)[0], (*value)[1], (*value)[2]);
    }
};
class CSVVector4f : public CSVType
{
public:
    CVector4f *value;
    CSVVector4f(const CString &screenName, CVector4f *defaultValue, bool mConfigBypass) {
        variableName=screenName;value=defaultValue;configBypass=mConfigBypass;}
    void loadConfig(std::ifstream & ficIn){
        CVector4f tmp;
        ficIn >> tmp[0];
        ficIn >> tmp[1];
        ficIn >> tmp[2];
        ficIn >> tmp[3];
        if (configBypass)
        {
            *value = tmp;
        }
    }
    void saveConfig(std::ofstream & ficOut)
    {
        ficOut << (*value)[0] << " ";
        ficOut << (*value)[1] << " ";
        ficOut << (*value)[2] << " ";
        ficOut << (*value)[3];
    }
    bool setValue(const CString & paramsT){
        CString params = paramsT;
        CString param1 = params.getFirstToken(' ');
        CString param2 = params.getFirstToken(' ');
        CString param3 = params.getFirstToken(' ');
        CString param4 = params.getFirstToken(' ');
        param1.trim('\"');
        param2.trim('\"');
        param3.trim('\"');
        param4.trim('\"');
        *value = CVector4f(param1.toFloat(), param2.toFloat(), param3.toFloat(), param4.toFloat());
        return true;
    }
    CString getValue()
    {
        return CString("%f %f %f %f", (*value)[0], (*value)[1], (*value)[2], (*value)[3]);
    }
};
class CSVString : public CSVType
{
public:
    CSVString(const CSVString& svString)
    {
        value = svString.value;
        variableName = svString.variableName;
        configBypass = svString.configBypass;
    }
    CString *value;
    CSVString(const CString &screenName, CString *defaultValue, bool mConfigBypass) {
        variableName=screenName;value=defaultValue;configBypass=mConfigBypass;}
    void loadConfig(std::ifstream & ficIn)
    {
        char tmp[256];
        ficIn.getline(tmp, 256);
        CString s(tmp);
        setValue(s);
    }
    void saveConfig(std::ofstream & ficOut)
    {
        ficOut << "\"" << value->s << "\"";
    }
    bool setValue(const CString & params);
    CString getValue()
    {
        return CString("\"%s\"", value->s);
    }
};




class CSystemVariable
{
public:
    // La listes des variables enregistré
    std::vector<CSVType*> variables;

public:
    // Constructeur / Destructeur
    CSystemVariable(); virtual ~CSystemVariable();

    CStringInterface * stringInterface;

public:
    // Nous pouvons enregistrer les nouvelles variables comme bon nous semble avec cette fonction
    void registerSystemVariable(const CString &screenName, bool      *defaultValue, bool mConfigBypass);
    void registerSystemVariable(const CString &screenName, int       *defaultValue, int minValue,
                                             int maxValue, int flags, bool mConfigBypass);
    void registerSystemVariable(const CString &screenName, float     *defaultValue, float minValue,
                                             float maxValue, int flags, bool mConfigBypass);
    void registerSystemVariable(const CString &screenName, CVector2i *defaultValue, bool mConfigBypass);
    void registerSystemVariable(const CString &screenName, CVector2f *defaultValue, bool mConfigBypass);
    void registerSystemVariable(const CString &screenName, CVector3i *defaultValue, bool mConfigBypass);
    void registerSystemVariable(const CString &screenName, CVector3f *defaultValue, bool mConfigBypass);
    void registerSystemVariable(const CString &screenName, CVector4f *defaultValue, bool mConfigBypass);
    void registerSystemVariable(const CString &screenName, CString   *defaultValue, bool mConfigBypass);

    // Pour effacer une variable du stack
    void unregisterSystemVariable(const CString &screenName);

    // pour loader un fichier config contenant la prédéfinition des variables
    void loadConfig(char * filename);
    void loadConfigSVOnly(char * filename);

    // Pour saver un fichier config contenant la prédéfinition des variables
    void saveConfig(char * filename);

    // Pour gèrer les commandes pour modifier les variables
    CMD_RET command(CString & commandName, CString & params);
};



extern CSystemVariable systemVariable;


//
// Pour envoyer une commande console
//
CMD_RET         dksvarCommand(char * str_command)
{
    // On set une commande
    CString strCommand(str_command);

    // On prend le premier token pour savoir c'est quoi notre commande
    CString commandName = strCommand.getFirstToken(' ');

    // On check maintenant si elle contient une commande en liens avec les system variable
    return systemVariable.command(commandName, strCommand); // Elle a ��trait�
}



void dksvarGetFormatedVar(char * varName, CString * formatedString)
{
    for (int i=0;i<(int)systemVariable.variables.size();++i)
    {
        CSVType * var = systemVariable.variables[i];
        if (strnicmp(var->variableName.s, varName, strlen(varName)) == 0)
        {
            // On pogne son nom format�avec la valeur apres
            CString formatedName = CString(varName) + " " + var->getValue();
            systemVariable.stringInterface->updateString(formatedString, formatedName.s);
            return;
        }
    }
}



void dksvarGetFilteredVar(char * varName, char ** array, int size)
{
    int len = int(strlen(varName));
    std::vector<CString> matchs;
    int i = 0;
    for (i=0;i<(int)systemVariable.variables.size();++i)
    {
        CSVType * var = systemVariable.variables[i];
        if (strnicmp(var->variableName.s, varName, len) == 0)
        {
            // On le place en ordre alphab�ique dans la liste trouv�
            int j=0;
            for (j=0;j<(int)matchs.size();++j)
            {
                if (var->variableName < matchs[j])
                {
                    break;
                }
            }
            matchs.insert(matchs.begin() + j, var->variableName);
        }
    }

    // Finalement, on fou les "size" premier dans notre array
    for (i=0;i<size;++i)
    {
        if (i < (int)matchs.size())
        {
            if (i == 0 && matchs.size() == 1)
            {
                strcpy(array[i], matchs[i].s);
            }
            else
            {
                CString varCopy = matchs[i];
                strcpy(array[i], varCopy.getFirstToken(' ').s);
            }
        }
        else
        {
            array[i][0] = '\0';
        }
    }

    matchs.clear();
}

static void dksvarInit(dkContext* ctx, dkConfig config)
{

}

//
// Pour loader un fichier config
//
void dksvarLoadConfig(char * filename)
{
    systemVariable.loadConfig(filename);
}

void dksvarLoadConfigSVOnly(char * filename)
{
    systemVariable.loadConfigSVOnly(filename);
}



//
// Pour sauvegarder un fichier config
//
void dksvarSaveConfig(char * filename)
{
    systemVariable.saveConfig(filename);
}



//
// Pour enregistrer des variables dans le system variable
//
void dksvarRegister(const CString &screenName, bool       *defaultValue, bool mConfigBypass)
{
    systemVariable.registerSystemVariable(screenName, defaultValue, mConfigBypass);
}

void dksvarRegister(const CString &screenName, int        *defaultValue, int minValue,
                                             int maxValue, int flags, bool mConfigBypass)
{
    systemVariable.registerSystemVariable(screenName, defaultValue, minValue, maxValue, flags, mConfigBypass);
}

void dksvarRegister(const CString &screenName, float  *defaultValue, float minValue,
                                             float maxValue, int flags, bool mConfigBypass)
{
    systemVariable.registerSystemVariable(screenName, defaultValue, minValue, maxValue, flags, mConfigBypass);
}

void dksvarRegister(const CString &screenName, CVector2i *defaultValue, bool mConfigBypass)
{
    systemVariable.registerSystemVariable(screenName, defaultValue, mConfigBypass);
}

void dksvarRegister(const CString &screenName, CVector2f *defaultValue, bool mConfigBypass)
{
    systemVariable.registerSystemVariable(screenName, defaultValue, mConfigBypass);
}

void dksvarRegister(const CString &screenName, CVector3i *defaultValue, bool mConfigBypass)
{
    systemVariable.registerSystemVariable(screenName, defaultValue, mConfigBypass);
}

void dksvarRegister(const CString &screenName, CVector3f *defaultValue, bool mConfigBypass)
{
    systemVariable.registerSystemVariable(screenName, defaultValue, mConfigBypass);
}

void dksvarRegister(const CString &screenName, CVector4f *defaultValue, bool mConfigBypass)
{
    systemVariable.registerSystemVariable(screenName, defaultValue, mConfigBypass);
}

void dksvarRegister(const CString &screenName, CString    *defaultValue, bool mConfigBypass)
{
    systemVariable.registerSystemVariable(screenName, defaultValue, mConfigBypass);
}



//
// Pour d�enregistrer
//
void dksvarUnregister(const CString &screenName)
{
    systemVariable.unregisterSystemVariable(screenName);
}


// Notre objet
CSystemVariable systemVariable;

class StringInterface : public CStringInterface
{
public:
    virtual void updateString(CString* string, char * newValue)
    {
        *string = newValue;
    }
    StringInterface()
        {
            //printf("Constructor: 0x%x\n", this);
        }
    virtual ~StringInterface()
        {
        }
};

//
// Constructeur / Destructeurs
//
CSystemVariable::CSystemVariable()
{
    stringInterface = new StringInterface;
}

CSystemVariable::~CSystemVariable()
{
    // On effaces les variables
    for (int i=0;i<(int)variables.size();i++)
    {
        CSVType *svType = variables.at(i);
        if (svType) delete svType;
    }
    variables.clear();
}




bool CSVString::setValue(const CString & paramsT){
    CString params = paramsT;
    params.trim(' ');
    params.trim('\"');

    // On call la string interface, sinon on ne peut PAS modifier le string
    if (systemVariable.stringInterface)
        systemVariable.stringInterface->updateString(value, params.s);
    else return false;

    return true;
}



//
// On load un fichier config
//
void CSystemVariable::loadConfig(char * filename)
{
    std::ifstream ficIn(filename, std::ios::in);

    if (ficIn.fail())
    {
        // On imprime message à la console? (pkoi pas, on est dedans :P)
        return;
    }

    while (!ficIn.eof())
    {
        char variable[256];
        ficIn >> variable;
        if ((variable[0] == '/') && (variable[1] == '/'))
        {
            ficIn.ignore(512, '\n');
            continue;
        }

        // On check à quel variable ça correspond
        for (int i=0;i<(int)variables.size();i++)
        {
            CSVType *svType = variables.at(i);
            CString varNameTmp = svType->variableName;
            if (varNameTmp.getFirstToken(' ') == variable)
            {
                svType->loadConfig(ficIn);
                break;
            }
        }

        // On flush le reste de la ligne
        ficIn.ignore(512, '\n');
    }
    ficIn.close();
}

void CSystemVariable::loadConfigSVOnly(char * filename)
{
    std::ifstream ficIn(filename, std::ios::in);

    if (ficIn.fail())
    {
        // On imprime message à la console? (pkoi pas, on est dedans :P)
        return;
    }

    while (!ficIn.eof())
    {
        char variable[256];
        ficIn >> variable;
        if ((variable[0] == '/') && (variable[1] == '/'))
        {
            ficIn.ignore(512, '\n');
            continue;
        }

        // On check à quel variable ça correspond
        for (int i=0;i<(int)variables.size();i++)
        {
            CSVType *svType = variables.at(i);
            CString varNameTmp = svType->variableName;
            if (varNameTmp.getFirstToken(' ') == variable)
            {
                if (strnicmp(variable, "sv_", 3) == 0)
                {
                    svType->loadConfig(ficIn);
                    break;
                }
                else
                {
                    ficIn.ignore(512, '\n');
                    continue;
                }
            }
        }

        // On flush le reste de la ligne
        ficIn.ignore(512, '\n');
    }
    ficIn.close();
}



//
// Pour saver un fichier config contenant la prédéfinition des variables
//
void CSystemVariable::saveConfig(char * filename)
{
    FILE * fic = fopen(filename, "wb");
    if (!fic) return;
    fclose(fic);

    std::ofstream ficOut(filename, std::ios::in);

    if (ficOut.fail())
    {
        // On imprime message à la console? (pkoi pas, on est dedans :P)
        return;
    }

    ficOut << "// Modifying this file can cause game crashes\n";
    ficOut << "// If you have corrupted something, just delete this file and run the game again\n";

    for (int i=0;i<(int)variables.size();i++)
    {
        ficOut << std::endl;
        CSVType *svType = variables.at(i);

        CString varNameTmp = svType->variableName;
        ficOut <<varNameTmp.getFirstToken(' ').s << " ";
        svType->saveConfig(ficOut);
        ficOut << std::endl;
    }

    ficOut.close();
}



//
// Pour effacer une variable du stack
//
void CSystemVariable::unregisterSystemVariable(const CString &screenName)
{
    for (int i=0;i<(int)variables.size();i++)
    {
        CSVType *svType = variables.at(i);
        CString varCopy = svType->variableName;
        if (varCopy.getFirstToken(' ') == screenName)
        {
            variables.erase(variables.begin() + i);
            delete svType;
            return;
        }
    }
}



//
// Pour gèrer les commandes pour modifier les variables
//
CMD_RET CSystemVariable::command(CString & commandName, CString & params)
{
    if (commandName == "set")
    {
        // On prend le nom de la variable
        CString varName = params.getFirstToken(' ');
        varName.trim('\"'); // Au cas où, y en a des épais tse

        // On loop les variables pour savoir si elle existe et on lui transfer ses params
        for (int i=0;i<(int)variables.size();i++)
        {
            CSVType *svType = variables.at(i);
            CString varCopy = svType->variableName;
            if (varCopy.getFirstToken(' ') == varName)
            {
                if (svType->setValue(params)) return CR_OK;
                else
                {
                    // Oups, un ti message d'erreur. Mauvais nombre de param probablement
                    return CR_INVALIDARGS;
                }
            }
        }

        // Oups un ti message d'erreur pour dire que la variable n'existe pas
        return CR_NOSUCHVAR;
    }

    // Aucune commande n'a été trouvé ici
    return CR_NOTSUPPORTED;
}



//
// Pour enregistrer les variable
//
void CSystemVariable::registerSystemVariable(const CString &screenName, bool *defaultValue, bool mConfigBypass)
{
    CSVType *newType = new CSVBool(screenName, defaultValue, mConfigBypass);
    variables.push_back(newType);
}

void CSystemVariable::registerSystemVariable(const CString &screenName, int *defaultValue, int minValue,
                                             int maxValue, int flags, bool mConfigBypass)
{
    CSVType *newType = new CSVInt(screenName, defaultValue, minValue, maxValue, flags, mConfigBypass);
    variables.push_back(newType);
}

void CSystemVariable::registerSystemVariable(const CString &screenName, float *defaultValue, float minValue,
                                             float maxValue, int flags, bool mConfigBypass)
{
    CSVType *newType = new CSVFloat(screenName, defaultValue, minValue, maxValue, flags, mConfigBypass);
    variables.push_back(newType);
}

void CSystemVariable::registerSystemVariable(const CString &screenName, CVector2i *defaultValue, bool mConfigBypass)
{
    CSVType *newType = new CSVVector2i(screenName, defaultValue, mConfigBypass);
    variables.push_back(newType);
}

void CSystemVariable::registerSystemVariable(const CString &screenName, CVector2f *defaultValue, bool mConfigBypass)
{
    CSVType *newType = new CSVVector2f(screenName, defaultValue, mConfigBypass);
    variables.push_back(newType);
}

void CSystemVariable::registerSystemVariable(const CString &screenName, CVector3i *defaultValue, bool mConfigBypass)
{
    CSVType *newType = new CSVVector3i(screenName, defaultValue, mConfigBypass);
    variables.push_back(newType);
}

void CSystemVariable::registerSystemVariable(const CString &screenName, CVector3f *defaultValue, bool mConfigBypass)
{
    CSVType *newType = new CSVVector3f(screenName, defaultValue, mConfigBypass);
    variables.push_back(newType);
}

void CSystemVariable::registerSystemVariable(const CString &screenName, CVector4f *defaultValue, bool mConfigBypass)
{
    CSVType *newType = new CSVVector4f(screenName, defaultValue, mConfigBypass);
    variables.push_back(newType);
}

void CSystemVariable::registerSystemVariable(const CString &screenName, CString *defaultValue, bool mConfigBypass)
{
    CSVType *newType = new CSVString(screenName, defaultValue, mConfigBypass);
    variables.push_back(newType);
}

dkContext* dkInit(dkConfig config)
{
    dkContext* ctx = new dkContext;

    dkcInit(ctx, config);
    dksvarInit(ctx, config);

    return ctx;
}

void dkFini(dkContext* ctx)
{
    delete ctx;
}
