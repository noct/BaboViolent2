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

class CStringInterface;

struct dkContext;

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

struct SVarBool
{
    bool* value;
};
struct SVarInt
{
    int* value;
    int minV, maxV, defaultV, limitsFlags;
};
struct SVarFloat
{
    float* value;
    float minV, maxV, defaultV;
    int limitsFlags;
};
struct SVarVec2i
{
    CVector2i* value;
};
struct SVarVec2f
{
    CVector2f* value;
};
struct SVarVec3i
{
    CVector3i* value;
};
struct SVarVec3f
{
    CVector3f* value;
};
struct SVarVec4f
{
    CVector4f* value;
};
struct SVarString
{
    CString* value;
};

struct CSVType
{
    enum Type
    {
        VarBool,
        VarInt,
        VarFloat,
        VarVec2i,
        VarVec2f,
        VarVec3i,
        VarVec3f,
        VarVec4f,
        VarString
    } type;
    union Data
    {
        SVarBool   b;
        SVarInt    i;
        SVarFloat  f;
        SVarVec2i  v2i;
        SVarVec2f  v2f;
        SVarVec3i  v3i;
        SVarVec3f  v3f;
        SVarVec4f  v4f;
        SVarString s;
    } data;
    CString variableName;
    bool configBypass;
};

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

    struct dksvar
    {
        std::vector<CSVType> variables;
        CStringInterface *   stringInterface;
    } var;
};


bool varSetValueData(dkContext* ctx, SVarBool & var, const CString & paramsT)
{
    CString params = paramsT;
    CString param1 = params.getFirstToken(' ');
    param1.trim('\"');
    if(param1 == "false") *var.value = false;
    else if(param1 == "true") *var.value = true;
    else return false; // Petit m'essage d'erreur
    return true;

}

bool varSetValueData(dkContext* ctx, SVarInt & var, const CString & paramsT)
{
    CString params = paramsT;
    CString param1 = params.getFirstToken(' ');
    param1.trim('\"');
    int tmp = param1.toInt();
    if(((var.limitsFlags & LIMIT_MIN) && (tmp < var.minV)) ||
        ((var.limitsFlags & LIMIT_MAX) && (tmp > var.maxV)))
        return false;
    *var.value = tmp;
    return true;

}

bool varSetValueData(dkContext* ctx, SVarFloat & var, const CString & paramsT)
{
    CString params = paramsT;
    CString param1 = params.getFirstToken(' ');
    param1.trim('\"');
    float tmp = param1.toFloat();
    if(((var.limitsFlags & LIMIT_MIN) && (tmp < var.minV)) ||
        ((var.limitsFlags & LIMIT_MAX) && (tmp > var.maxV)))
        return false;
    *var.value = tmp;
    return true;

}

bool varSetValueData(dkContext* ctx, SVarVec2i & var, const CString & paramsT)
{
    CString params = paramsT;
    CString param1 = params.getFirstToken(' ');
    CString param2 = params.getFirstToken(' ');
    param1.trim('\"');
    param2.trim('\"');
    *var.value = CVector2i(param1.toInt(), param2.toInt());
    return true;

}

bool varSetValueData(dkContext* ctx, SVarVec2f & var, const CString & paramsT)
{
    CString params = paramsT;
    CString param1 = params.getFirstToken(' ');
    CString param2 = params.getFirstToken(' ');
    param1.trim('\"');
    param2.trim('\"');
    *var.value = CVector2f(param1.toFloat(), param2.toFloat());
    return true;
}

bool varSetValueData(dkContext* ctx, SVarVec3i & var, const CString & paramsT)
{
    CString params = paramsT;
    CString param1 = params.getFirstToken(' ');
    CString param2 = params.getFirstToken(' ');
    CString param3 = params.getFirstToken(' ');
    param1.trim('\"');
    param2.trim('\"');
    param3.trim('\"');
    *var.value = CVector3i(param1.toInt(), param2.toInt(), param3.toInt());
    return true;

}

bool varSetValueData(dkContext* ctx, SVarVec3f & var, const CString & paramsT)
{
    CString params = paramsT;
    CString param1 = params.getFirstToken(' ');
    CString param2 = params.getFirstToken(' ');
    CString param3 = params.getFirstToken(' ');
    param1.trim('\"');
    param2.trim('\"');
    param3.trim('\"');
    *var.value = CVector3f(param1.toFloat(), param2.toFloat(), param3.toFloat());
    return true;

}

bool varSetValueData(dkContext* ctx, SVarVec4f & var, const CString & paramsT)
{
    CString params = paramsT;
    CString param1 = params.getFirstToken(' ');
    CString param2 = params.getFirstToken(' ');
    CString param3 = params.getFirstToken(' ');
    CString param4 = params.getFirstToken(' ');
    param1.trim('\"');
    param2.trim('\"');
    param3.trim('\"');
    param4.trim('\"');
    *var.value = CVector4f(param1.toFloat(), param2.toFloat(), param3.toFloat(), param4.toFloat());
    return true;
}

bool varSetValueData(dkContext* ctx, SVarString & var, const CString & paramsT)
{
    CString params = paramsT;
    params.trim(' ');
    params.trim('\"');

    // On call la string interface, sinon on ne peut PAS modifier le string
    if(ctx->var.stringInterface)
    {
        ctx->var.stringInterface->updateString(var.value, params.s);
        return true;
    }
    return false;
}

void varSaveConfigData(dkContext* ctx, SVarBool & var, std::ofstream & ficOut)
{
    if(*var.value) ficOut << "true";
    else ficOut << "false";
}

void varSaveConfigData(dkContext* ctx, SVarInt & var, std::ofstream & ficOut)
{
    ficOut << *var.value;
}

void varSaveConfigData(dkContext* ctx, SVarFloat & var, std::ofstream & ficOut)
{
    ficOut << *var.value;
}

void varSaveConfigData(dkContext* ctx, SVarVec2i & var, std::ofstream & ficOut)
{
    ficOut << (*var.value)[0] << " ";
    ficOut << (*var.value)[1];
}

void varSaveConfigData(dkContext* ctx, SVarVec2f & var, std::ofstream & ficOut)
{
    ficOut << (*var.value)[0] << " ";
    ficOut << (*var.value)[1];
}

void varSaveConfigData(dkContext* ctx, SVarVec3i & var, std::ofstream & ficOut)
{
    ficOut << (*var.value)[0] << " ";
    ficOut << (*var.value)[1] << " ";
    ficOut << (*var.value)[2];
}

void varSaveConfigData(dkContext* ctx, SVarVec3f & var, std::ofstream & ficOut)
{
    ficOut << (*var.value)[0] << " ";
    ficOut << (*var.value)[1] << " ";
    ficOut << (*var.value)[2];
}

void varSaveConfigData(dkContext* ctx, SVarVec4f & var, std::ofstream & ficOut)
{
    ficOut << (*var.value)[0] << " ";
    ficOut << (*var.value)[1] << " ";
    ficOut << (*var.value)[2] << " ";
    ficOut << (*var.value)[3];
}

void varSaveConfigData(dkContext* ctx, SVarString & var, std::ofstream & ficOut)
{
    ficOut << "\"" << var.value->s << "\"";
}

void varLoadConfigData(dkContext* ctx, SVarBool & var, std::ifstream & ficIn, bool configBypass)
{
    char tmp[16];
    ficIn >> tmp;
    if(configBypass)
    {
        if(stricmp(tmp, "false") == 0)
        {
            *var.value = false;
        }
        else if(stricmp(tmp, "true") == 0)
        {
            *var.value = true;
        }
    }
}

void varLoadConfigData(dkContext* ctx, SVarInt & var, std::ifstream & ficIn, bool configBypass)
{
    int tmp;
    ficIn >> tmp;
    if(configBypass)
    {
        if(((var.limitsFlags & LIMIT_MIN) && (tmp < var.minV)) ||
            ((var.limitsFlags & LIMIT_MAX) && (tmp > var.maxV)))
            *var.value = var.defaultV;
        else
            *var.value = tmp;
    }
}

void varLoadConfigData(dkContext* ctx, SVarFloat & var, std::ifstream & ficIn, bool configBypass)
{
    float tmp;
    ficIn >> tmp;
    if(configBypass)
    {
        if(((var.limitsFlags & LIMIT_MIN) && (tmp < var.minV)) ||
            ((var.limitsFlags & LIMIT_MAX) && (tmp > var.maxV)))
            *var.value = var.defaultV;
        else
            *var.value = tmp;
    }
}

void varLoadConfigData(dkContext* ctx, SVarVec2i & var, std::ifstream & ficIn, bool configBypass)
{
    CVector2i tmp;
    ficIn >> tmp[0];
    ficIn >> tmp[1];
    if(configBypass)
    {
        *var.value = tmp;
    }
}

void varLoadConfigData(dkContext* ctx, SVarVec2f & var, std::ifstream & ficIn, bool configBypass)
{
    CVector2f tmp;
    ficIn >> tmp[0];
    ficIn >> tmp[1];
    if(configBypass)
    {
        *var.value = tmp;
    }
}

void varLoadConfigData(dkContext* ctx, SVarVec3i & var, std::ifstream & ficIn, bool configBypass)
{
    CVector3i tmp;
    ficIn >> tmp[0];
    ficIn >> tmp[1];
    ficIn >> tmp[2];
    if(configBypass)
    {
        *var.value = tmp;
    }
}

void varLoadConfigData(dkContext* ctx, SVarVec3f & var, std::ifstream & ficIn, bool configBypass)
{
    CVector3f tmp;
    ficIn >> tmp[0];
    ficIn >> tmp[1];
    ficIn >> tmp[2];
    if(configBypass)
    {
        *var.value = tmp;
    }
}

void varLoadConfigData(dkContext* ctx, SVarVec4f & var, std::ifstream & ficIn, bool configBypass)
{
    CVector4f tmp;
    ficIn >> tmp[0];
    ficIn >> tmp[1];
    ficIn >> tmp[2];
    ficIn >> tmp[3];
    if(configBypass)
    {
        *var.value = tmp;
    }
}

void varLoadConfigData(dkContext* ctx, SVarString & var, std::ifstream & ficIn, bool configBypass)
{
    char tmp[256];
    ficIn.getline(tmp, 256);
    CString s(tmp);
    varSetValueData(ctx, var, s);
}

void varLoadConfig(dkContext* ctx, CSVType * var, std::ifstream & ficIn)
{
    switch(var->type)
    {
    case CSVType::VarBool:   varLoadConfigData(ctx, var->data.b, ficIn, var->configBypass); break;
    case CSVType::VarInt:    varLoadConfigData(ctx, var->data.i, ficIn, var->configBypass); break;
    case CSVType::VarFloat:  varLoadConfigData(ctx, var->data.f, ficIn, var->configBypass); break;
    case CSVType::VarVec2i:  varLoadConfigData(ctx, var->data.v2i, ficIn, var->configBypass); break;
    case CSVType::VarVec2f:  varLoadConfigData(ctx, var->data.v2f, ficIn, var->configBypass); break;
    case CSVType::VarVec3i:  varLoadConfigData(ctx, var->data.v3i, ficIn, var->configBypass); break;
    case CSVType::VarVec3f:  varLoadConfigData(ctx, var->data.v3f, ficIn, var->configBypass); break;
    case CSVType::VarVec4f:  varLoadConfigData(ctx, var->data.v4f, ficIn, var->configBypass); break;
    case CSVType::VarString: varLoadConfigData(ctx, var->data.s, ficIn, var->configBypass); break;
    default: break;
    }
}

void varSaveConfig(dkContext* ctx, CSVType * var, std::ofstream & ficOut)
{
    switch(var->type)
    {
    case CSVType::VarBool:   varSaveConfigData(ctx, var->data.b, ficOut); break;
    case CSVType::VarInt:    varSaveConfigData(ctx, var->data.i, ficOut); break;
    case CSVType::VarFloat:  varSaveConfigData(ctx, var->data.f, ficOut); break;
    case CSVType::VarVec2i:  varSaveConfigData(ctx, var->data.v2i, ficOut); break;
    case CSVType::VarVec2f:  varSaveConfigData(ctx, var->data.v2f, ficOut); break;
    case CSVType::VarVec3i:  varSaveConfigData(ctx, var->data.v3i, ficOut); break;
    case CSVType::VarVec3f:  varSaveConfigData(ctx, var->data.v3f, ficOut); break;
    case CSVType::VarVec4f:  varSaveConfigData(ctx, var->data.v4f, ficOut); break;
    case CSVType::VarString: varSaveConfigData(ctx, var->data.s, ficOut); break;
    default: break;
    }
}

bool varSetValue(dkContext* ctx, CSVType * var, const CString & paramsT)
{
    switch(var->type)
    {
    case CSVType::VarBool:   return varSetValueData(ctx, var->data.b, paramsT);
    case CSVType::VarInt:    return varSetValueData(ctx, var->data.i, paramsT);
    case CSVType::VarFloat:  return varSetValueData(ctx, var->data.f, paramsT);
    case CSVType::VarVec2i:  return varSetValueData(ctx, var->data.v2i, paramsT);
    case CSVType::VarVec2f:  return varSetValueData(ctx, var->data.v2f, paramsT);
    case CSVType::VarVec3i:  return varSetValueData(ctx, var->data.v3i, paramsT);
    case CSVType::VarVec3f:  return varSetValueData(ctx, var->data.v3f, paramsT);
    case CSVType::VarVec4f:  return varSetValueData(ctx, var->data.v4f, paramsT);
    case CSVType::VarString: return varSetValueData(ctx, var->data.s, paramsT);
    default: break;
    }
}

CString varGetValue(dkContext* ctx, CSVType * var)
{
    switch(var->type)
    {
    case CSVType::VarBool:   return *var->data.b.value ? "true" : "false";
    case CSVType::VarInt:    return CString("%i", *var->data.i.value);
    case CSVType::VarFloat:  return CString("%f", *var->data.f.value);
    case CSVType::VarVec2i:  return CString("%i %i", (*var->data.v2i.value)[0], (*var->data.v2i.value)[1]);
    case CSVType::VarVec2f:  return CString("%f %f", (*var->data.v2f.value)[0], (*var->data.v2f.value)[1]);
    case CSVType::VarVec3i:  return CString("%i %i %i", (*var->data.v3i.value)[0], (*var->data.v3i.value)[1], (*var->data.v3i.value)[2]);
    case CSVType::VarVec3f:  return CString("%f %f %f", (*var->data.v3f.value)[0], (*var->data.v3f.value)[1], (*var->data.v3f.value)[2]);
    case CSVType::VarVec4f:  return CString("%f %f %f %f", (*var->data.v4f.value)[0], (*var->data.v4f.value)[1], (*var->data.v4f.value)[2], (*var->data.v4f.value)[3]);
    case CSVType::VarString: return CString("\"%s\"", var->data.s.value->s);
    default: break;
    }
}

CSVType varCreateBool(const CString &screenName, bool *defaultValue, bool mConfigBypass)
{
    CSVType var = {};
    var.type = CSVType::VarBool;
    var.variableName = screenName;
    var.configBypass = mConfigBypass;
    var.data.b.value = defaultValue;
    return var;
}

CSVType varCreateInt(const CString &screenName, int *defaultValue, int minValue, int maxValue, int flags,  bool mConfigBypass)
{
    CSVType var = {};
    var.type = CSVType::VarBool;
    var.variableName = screenName;
    var.configBypass = mConfigBypass;
    var.data.i.defaultV = *defaultValue;
    var.data.i.limitsFlags = flags;
    var.data.i.maxV = maxValue;
    var.data.i.minV = minValue;
    var.data.i.value = defaultValue;
    return var;
}

CSVType varCreateFloat(const CString &screenName, float *defaultValue, float minValue, float maxValue, int flags, bool mConfigBypass)
{
    CSVType var = {};
    var.type = CSVType::VarBool;
    var.variableName = screenName;
    var.configBypass = mConfigBypass;
    var.data.i.defaultV = *defaultValue;
    var.data.i.limitsFlags = flags;
    var.data.i.maxV = maxValue;
    var.data.i.minV = minValue;
    var.data.f.value = defaultValue;
    return var;
}

CSVType varCreateVec2i(const CString &screenName, CVector2i *defaultValue, bool mConfigBypass)
{
    CSVType var = {};
    var.type = CSVType::VarBool;
    var.variableName = screenName;
    var.configBypass = mConfigBypass;
    var.data.v2i.value = defaultValue;
    return var;
}

CSVType varCreateVec2f(const CString &screenName, CVector2f *defaultValue, bool mConfigBypass)
{
    CSVType var = {};
    var.type = CSVType::VarBool;
    var.variableName = screenName;
    var.configBypass = mConfigBypass;
    var.data.v2f.value = defaultValue;
    return var;
}

CSVType varCreateVec3i(const CString &screenName, CVector3i *defaultValue, bool mConfigBypass)
{
    CSVType var = {};
    var.type = CSVType::VarBool;
    var.variableName = screenName;
    var.configBypass = mConfigBypass;
    var.data.v3i.value = defaultValue;
    return var;
}

CSVType varCreateVec3f(const CString &screenName, CVector3f *defaultValue, bool mConfigBypass)
{
    CSVType var = {};
    var.type = CSVType::VarBool;
    var.variableName = screenName;
    var.configBypass = mConfigBypass;
    var.data.v3f.value = defaultValue;
    return var;
}

CSVType varCreateVec4f(const CString &screenName, CVector4f *defaultValue, bool mConfigBypass)
{
    CSVType var = {};
    var.type = CSVType::VarBool;
    var.variableName = screenName;
    var.configBypass = mConfigBypass;
    var.data.v4f.value = defaultValue;
    return var;
}

CSVType varCreateString(const CSVType& svString)
{
    CSVType var = {};
    var.type = CSVType::VarString;
    var.variableName = svString.variableName;
    var.configBypass = svString.configBypass;
    var.data.s.value = svString.data.s.value;
    return var;
}

CSVType varCreateString(const CString &screenName, CString *defaultValue, bool mConfigBypass)
{
    CSVType var = {};
    var.type = CSVType::VarString;
    var.variableName = screenName;
    var.configBypass = mConfigBypass;
    var.data.s.value = defaultValue;
    return var;
}


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

    // On retourne le nombre de frame êanimer
    return nbFrameAdded;
}

void dkcSleep(dkContext* ctx, int32_t ms)
{
    ZEVEN_UNUSED(ctx);
    SDL_Delay(ms);
}

CMD_RET dksvarCommand(dkContext * ctx, char * str_command)
{
    // On set une commande
    CString params(str_command);

    // On prend le premier token pour savoir c'est quoi notre commande
    CString commandName = params.getFirstToken(' ');

    if(commandName == "set")
    {
        // On prend le nom de la variable
        CString varName = params.getFirstToken(' ');
        varName.trim('\"'); // Au cas où, y en a des épais tse

        // On loop les variables pour savoir si elle existe et on lui transfer ses params
        for(int i = 0; i < (int)ctx->var.variables.size(); i++)
        {
            CSVType& svType = ctx->var.variables.at(i);
            CString varCopy = svType.variableName;
            if(varCopy.getFirstToken(' ') == varName)
            {
                if(varSetValue(ctx, &svType, params)) return CR_OK;
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

void dksvarGetFormatedVar(dkContext* ctx, char * varName, CString * formatedString)
{
    for(int i = 0; i < (int)ctx->var.variables.size(); ++i)
    {
        CSVType & var = ctx->var.variables[i];
        if(strnicmp(var.variableName.s, varName, strlen(varName)) == 0)
        {
            // On pogne son nom formatêavec la valeur apres
            CString formatedName = CString(varName) + " " + varGetValue(ctx, &var);
            ctx->var.stringInterface->updateString(formatedString, formatedName.s);
            return;
        }
    }
}

void dksvarGetFilteredVar(dkContext* ctx, char * varName, char ** array, int size)
{
    int len = int(strlen(varName));
    std::vector<CString> matchs;
    int i = 0;
    for (i=0;i<(int)ctx->var.variables.size();++i)
    {
        CSVType & var = ctx->var.variables[i];
        if (strnicmp(var.variableName.s, varName, len) == 0)
        {
            // On le place en ordre alphabêique dans la liste trouvê
            int j=0;
            for (j=0;j<(int)matchs.size();++j)
            {
                if (var.variableName < matchs[j])
                {
                    break;
                }
            }
            matchs.insert(matchs.begin() + j, var.variableName);
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
    ctx->var.stringInterface = new StringInterface;
}

static void dksvarFini(dkContext* ctx)
{
    ctx->var.variables.clear();
}

void dksvarLoadConfig(dkContext* ctx, char * filename)
{
    std::ifstream ficIn(filename, std::ios::in);

    if(ficIn.fail())
    {
        // On imprime message à la console? (pkoi pas, on est dedans :P)
        return;
    }

    while(!ficIn.eof())
    {
        char variable[256];
        ficIn >> variable;
        if((variable[0] == '/') && (variable[1] == '/'))
        {
            ficIn.ignore(512, '\n');
            continue;
        }

        // On check à quel variable ça correspond
        for(int i = 0; i < (int)ctx->var.variables.size(); i++)
        {
            CSVType & svType = ctx->var.variables.at(i);
            CString varNameTmp = svType.variableName;
            if(varNameTmp.getFirstToken(' ') == variable)
            {
                varLoadConfig(ctx, &svType, ficIn);
                break;
            }
        }

        // On flush le reste de la ligne
        ficIn.ignore(512, '\n');
    }
    ficIn.close();
}

void dksvarLoadConfigSVOnly(dkContext* ctx, char * filename)
{
    std::ifstream ficIn(filename, std::ios::in);

    if(ficIn.fail())
    {
        // On imprime message à la console? (pkoi pas, on est dedans :P)
        return;
    }

    while(!ficIn.eof())
    {
        char variable[256];
        ficIn >> variable;
        if((variable[0] == '/') && (variable[1] == '/'))
        {
            ficIn.ignore(512, '\n');
            continue;
        }

        // On check à quel variable ça correspond
        for(int i = 0; i < (int)ctx->var.variables.size(); i++)
        {
            CSVType & svType = ctx->var.variables.at(i);
            CString varNameTmp = svType.variableName;
            if(varNameTmp.getFirstToken(' ') == variable)
            {
                if(strnicmp(variable, "sv_", 3) == 0)
                {
                    varLoadConfig(ctx, &svType, ficIn);
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

void dksvarSaveConfig(dkContext* ctx, char * filename)
{
    FILE * fic = fopen(filename, "wb");
    if(!fic) return;
    fclose(fic);

    std::ofstream ficOut(filename, std::ios::in);

    if(ficOut.fail())
    {
        // On imprime message à la console? (pkoi pas, on est dedans :P)
        return;
    }

    ficOut << "// Modifying this file can cause game crashes\n";
    ficOut << "// If you have corrupted something, just delete this file and run the game again\n";

    for(int i = 0; i < (int)ctx->var.variables.size(); i++)
    {
        ficOut << std::endl;
        CSVType & svType = ctx->var.variables.at(i);

        CString varNameTmp = svType.variableName;
        ficOut << varNameTmp.getFirstToken(' ').s << " ";
        varSaveConfig(ctx, &svType, ficOut);
        ficOut << std::endl;
    }

    ficOut.close();
}

void dksvarRegister(dkContext* ctx, const CString &screenName, bool *defaultValue, bool mConfigBypass)
{
    ctx->var.variables.push_back(varCreateBool(screenName, defaultValue, mConfigBypass));
}

void dksvarRegister(dkContext* ctx, const CString &screenName, int *defaultValue, int minValue, int maxValue, int flags, bool mConfigBypass)
{
    ctx->var.variables.push_back(varCreateInt(screenName, defaultValue, minValue, maxValue, flags, mConfigBypass));
}

void dksvarRegister(dkContext* ctx, const CString &screenName, float  *defaultValue, float minValue, float maxValue, int flags, bool mConfigBypass)
{
    ctx->var.variables.push_back(varCreateFloat(screenName, defaultValue, minValue, maxValue, flags, mConfigBypass));
}

void dksvarRegister(dkContext* ctx, const CString &screenName, CVector2i *defaultValue, bool mConfigBypass)
{
    ctx->var.variables.push_back(varCreateVec2i(screenName, defaultValue, mConfigBypass));
}

void dksvarRegister(dkContext* ctx, const CString &screenName, CVector2f *defaultValue, bool mConfigBypass)
{
    ctx->var.variables.push_back(varCreateVec2f(screenName, defaultValue, mConfigBypass));
}

void dksvarRegister(dkContext* ctx, const CString &screenName, CVector3i *defaultValue, bool mConfigBypass)
{
    ctx->var.variables.push_back(varCreateVec3i(screenName, defaultValue, mConfigBypass));
}

void dksvarRegister(dkContext* ctx, const CString &screenName, CVector3f *defaultValue, bool mConfigBypass)
{
    ctx->var.variables.push_back(varCreateVec3f(screenName, defaultValue, mConfigBypass));
}

void dksvarRegister(dkContext* ctx, const CString &screenName, CVector4f *defaultValue, bool mConfigBypass)
{
    ctx->var.variables.push_back(varCreateVec4f(screenName, defaultValue, mConfigBypass));
}

void dksvarRegister(dkContext* ctx, const CString &screenName, CString    *defaultValue, bool mConfigBypass)
{
    ctx->var.variables.push_back(varCreateString(screenName, defaultValue, mConfigBypass));
}

void dksvarUnregister(dkContext* ctx, const CString &screenName)
{
    for(int i = 0; i < (int)ctx->var.variables.size(); i++)
    {
        CSVType & svType = ctx->var.variables.at(i);
        CString varCopy = svType.variableName;
        if(varCopy.getFirstToken(' ') == screenName)
        {
            ctx->var.variables.erase(ctx->var.variables.begin() + i);
            return;
        }
    }
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
    dksvarFini(ctx);
    delete ctx;
}
