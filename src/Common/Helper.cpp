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

#include "Helper.h"


//
// Pour enlever la couleur d'un message, pour le shadow par exemple
//
CString textColorLess(const CString & text)
{
    CString rval;
    char    result[MAX_CARAC];
    int     ri = 0;

    for (int i=0;i<text.len();++i)
    {
        char c = text[i];
        if ((unsigned char)c >= '\x10' || (unsigned char)c == '\n')
        {
            result[ri] = c;
            ri++;
        }
    }

    result[ri] = '\0';

    return CString("%s", result);
}

//
// Pour utiliser une fonction cubic spline pour pr�enir le lag appearance ;)
//
float cubicSpline(float x0, float x1, float x2, float x3, float t)
{
    return x0 * ((1 - t)*(1 - t)*(1 - t)) + x1 * 3 * t * ((1 - t)*(1 - t)) + x2 * 3 * (t*t) * (1 - t) + x3 * (t*t*t);
}

CVector3f cubicSpline(const CVector3f & x0, const CVector3f & x1, const CVector3f & x2, const CVector3f & x3, float t)
{
    return x0 * ((1 - t)*(1 - t)*(1 - t)) + x1 * 3 * t * ((1 - t)*(1 - t)) + x2 * 3 * (t*t) * (1 - t) + x3 * (t*t*t);
}



//
// On check sphere-to-box
//
bool sphereToBox(CVector3f& p1, CVector3f& p2, CVector3f& omin, CVector3f& omax, CVector3f & radius)
{
    CVector3f min = omin - radius;
    CVector3f max = omax + radius;
    CVector3f d = (p2 - p1) * 0.5f;
    CVector3f e = (max - min) * 0.5f;
    CVector3f c = p1 + d - (min + max) * 0.5f;
    CVector3f ad = CVector3f(fabsf(d[0]),fabsf(d[1]),fabsf(d[2])); // Returns same vector with all components positive

    if (fabsf(c[0]) > e[0] + ad[0])
        return false;
    if (fabsf(c[1]) > e[1] + ad[1])
        return false;
    if (fabsf(c[2]) > e[2] + ad[2])
        return false;

    if (fabsf(d[1] * c[2] - d[2] * c[1]) > e[1] * ad[2] + e[2] * ad[1]/* + EPSILON*/)
        return false;
    if (fabsf(d[2] * c[0] - d[0] * c[2]) > e[2] * ad[0] + e[0] * ad[2]/* + EPSILON*/)
        return false;
    if (fabsf(d[0] * c[1] - d[1] * c[0]) > e[0] * ad[1] + e[1] * ad[0]/* + EPSILON*/)
        return false;

    return true;
}



//
// Distance entre un point et une droite (2 sqrtf() dans fonction)
//
bool segmentToSphere(CVector3f & p1, CVector3f & p2, CVector3f & c, float radius)
{
    // Vecteur directeur
    CVector3f u = p2 - p1;
    float l = u.length();
    u /= l;

    // Vecteur vers les points
    CVector3f p = c - p1;

    // Projection distance
    float d = dot(p, u);

    // On trouve le point le plus proche sur le segment
    CVector3f r;
    if (d < 0)
    {
        r = p1;
    }
    else if (d > l)
    {
        r = p2;
    }
    else
    {
        r = p1 + u * d;
    }

    // On check la distance du point �l'axe
    if (distanceSquared(r, c) <= radius*radius)
    {
        // On ajuste notre p2
        p2 = r;
        return true;
    }
    return false;
}

