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

CMD_RET dksvarCommand(dkContext* ctx, char * command);
void    dksvarLoadConfig(dkContext* ctx, char * filename);
void    dksvarLoadConfigSVOnly(dkContext* ctx, char * filename);
void    dksvarSaveConfig(dkContext* ctx, char * filename);
void    dksvarRegister(dkContext* ctx, const CString &screenName, bool *defaultValue, bool mConfigBypass);
void    dksvarRegister(dkContext* ctx, const CString &screenName, int *defaultValue, int minValue, int maxValue, int flags, bool mConfigBypass);
void    dksvarRegister(dkContext* ctx, const CString &screenName, float *defaultValue, float minValue, float maxValue, int flags, bool mConfigBypass);
void    dksvarRegister(dkContext* ctx, const CString &screenName, CVector2i *defaultValue, bool mConfigBypass);
void    dksvarRegister(dkContext* ctx, const CString &screenName, CVector2f *defaultValue, bool mConfigBypass);
void    dksvarRegister(dkContext* ctx, const CString &screenName, CVector3i *defaultValue, bool mConfigBypass);
void    dksvarRegister(dkContext* ctx, const CString &screenName, CVector3f *defaultValue, bool mConfigBypass);
void    dksvarRegister(dkContext* ctx, const CString &screenName, CVector4f *defaultValue, bool mConfigBypass);
void    dksvarRegister(dkContext* ctx, const CString &screenName, CString *defaultValue, bool mConfigBypass);
void    dksvarUnregister(dkContext* ctx, const CString &screenName);
void    dksvarInit(dkContext* ctx, CStringInterface * stringInterface);
void    dksvarGetFilteredVar(dkContext* ctx, char * varName, char ** array, int size);
void    dksvarGetFormatedVar(dkContext* ctx, char * varName, CString * formatedString);

float   dkcGetElapsedf(dkContext* ctx);
float   dkcGetFPS(dkContext* ctx);
int32_t dkcGetFrame(dkContext* ctx);
void    dkcJumpToFrame(dkContext* ctx, int frame);
int32_t dkcUpdateTimer(dkContext* ctx);
void    dkcSleep(dkContext* ctx, int32_t ms);

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

