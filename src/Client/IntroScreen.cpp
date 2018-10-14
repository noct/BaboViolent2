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
#include "IntroScreen.h"
#include "GameVar.h"

//
// Constructeur
//
IntroScreen::IntroScreen()
{
    showDelay = 3;
    tex_rndLogo = dktCreateTextureFromFile("main/textures/RnDLabs.png", DKT_FILTER_LINEAR);
//  tex_glowLogo = dktCreateTextureFromFile("main/textures/RnDLabsGlow.png", DKT_FILTER_LINEAR);
    tex_hgLogo = dktCreateTextureFromFile("main/textures/HeadGames.png", DKT_FILTER_LINEAR);
//  sfx_intro = dksCreateSoundFromFile("main/Sounds/IntroScreen.mp3", false);

//  FSOUND_SetSFXMasterVolume((int)(255.0f*gameVar.s_masterVolume));
/*  if (gameVar.s_masterVolume > 0)
    {
        dksPlayMusic("main/sounds/IntroScreen.mp3", -1);
    }*/
}

//
// Destructeur
//
IntroScreen::~IntroScreen()
{
    dktDeleteTexture(&tex_rndLogo);
    dktDeleteTexture(&tex_hgLogo);
//  dksDeleteSound(sfx_intro);
}

//
// Update
//
void IntroScreen::update(float delay)
{
    if (dkiGetFirstDown() != DKI_NOKEY) showDelay = 0;
    if (showDelay == 6)
    {
    //  dksPlaySound(sfx_intro, -1, 255);
    }
    showDelay -= delay;
}
