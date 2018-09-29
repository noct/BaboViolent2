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

#ifndef HELPER_H
#define HELPER_H

#include <Zeven/Core.h>

// Pour utiliser une fonction cubic spline pour pr�enir le lag appearance ;)
float cubicSpline(float x0, float x1, float x2, float x3, float t);
CVector3f cubicSpline(const CVector3f & x0,const CVector3f & x1, const CVector3f & x2, const CVector3f & x3, float t);

// On check sphere-to-box
bool sphereToBox(CVector3f& p1, CVector3f& p2, CVector3f& omin, CVector3f& omax, CVector3f & radius);

// Pour enlever la couleur d'un message, pour le shadow par exemple
CString textColorLess(const CString & text);

// Segment to sphere
bool segmentToSphere(CVector3f & P0, CVector3f & P1, CVector3f & H, float r1);


#endif

