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
#include "COption.h"
#include "GameVar.h"
#include "KeyManager.h"
#include "ClientScene.h"
#include "ClientMap.h"

extern Scene * scene;

COption::COption(CControl * in_parent, CControl * in_alignTo)
{
    m_sfxClic = dksCreateSoundFromFile("main/sounds/Button.wav", false);
    m_sfxOver = dksCreateSoundFromFile("main/sounds/ControlOver.wav", false);

    parent = in_parent;

    //--- Da big frame
    instance = new CControl(parent, CVector2i(0, 0), CVector2i(736, 506), "", this, "FRAME", in_alignTo, CONTROL_SNAP_BOTTOM);
    instance->texture = dktCreateTextureFromFile("main/textures/Menu4Back.png", DKT_FILTER_LINEAR);
    instance->borderColor.set(1, .5f, .25f);

    //--- Labels and controls
    btn_apply = new CControl(instance, CVector2i(20, 10), CVector2i(75, 25), "Apply", this, "BUTTON");
    btn_apply->toolTips = "Apply the above settings. It's the only way to save them.";
    btn_apply->visible = false;

    //--- RENDERING OPTIONS
    CControl * separator = new CControl(instance, CVector2i(10, 20), CVector2i(200, 25), "Rendering options", this, "SEPARATOR");

    //--- Show stats
    CControl * label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Show stats:", this, "LABEL", separator, CONTROL_SNAP_BOTTOM, 15);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Show particles count, bandwith and FPS.";
    chk_showStats = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_showStats->check = gameVar.r_showStats;

    //--- Show latency
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Show latency:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Show latency box.";
    chk_showLatency = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_showLatency->check = gameVar.r_showLatency;

    //--- Precise cursor
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Precise Cursor:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Change how the gun points";
    chk_preciseCursor = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_preciseCursor->check = gameVar.cl_preciseCursor;

    //--- Full screen
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Full screen:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Play the game in fullscreen (Need to reboot the game).";
    chk_fullScreen = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_fullScreen->check = gameVar.r_fullScreen;

    //--- High details menu
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "High detail menus:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Show round corner menus (Required good video card).";
    chk_highDetailMenus = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_highDetailMenus->check = gameVar.r_highDetailMenu;

    //--- Animated menus
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Animated menus:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Animated menu background (Requires a good video card, Restart Required).";
    chk_animatedMenus = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_animatedMenus->check = gameVar.r_animatedMenu;

    //--- Screen resolution
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 150), "Screen resolution:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "(Need to reboot the game).";

    lst_resolution = new CControl(instance, CVector2i(10, 10), CVector2i(300, 150), "", this, "LISTBOX", label1, CONTROL_SNAP_RIGHT);
    CControl* item = new CControl(lst_resolution, CVector2i(10, 10), CVector2i(150, 20), "640 x 360", this, "LABEL");
    item = new CControl(lst_resolution, CVector2i(10, 10), CVector2i(150, 20), "1280 x 720", this, "LABEL", item, CONTROL_SNAP_BOTTOM);
    item = new CControl(lst_resolution, CVector2i(10, 10), CVector2i(150, 20), "1920 x 1080", this, "LABEL", item, CONTROL_SNAP_BOTTOM);
    if(gameVar.r_resolution == CVector2i(640, 360)) lst_resolution->selectChild(0);
    if(gameVar.r_resolution == CVector2i(1280, 720)) lst_resolution->selectChild(1);
    if(gameVar.r_resolution == CVector2i(1920, 1080)) lst_resolution->selectChild(2);

    //--- Display refresh rate
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 170), "Display refresh rate:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "(Need to reboot the game).";
    lst_refreshRate = new CControl(instance, CVector2i(10, 10), CVector2i(300, 170), "", this, "LISTBOX", label1, CONTROL_SNAP_RIGHT);
    item = new CControl(lst_refreshRate, CVector2i(10, 10), CVector2i(150, 20), "default", this, "LABEL");
    item = new CControl(lst_refreshRate, CVector2i(10, 10), CVector2i(150, 20), "60", this, "LABEL", item, CONTROL_SNAP_BOTTOM);
    item = new CControl(lst_refreshRate, CVector2i(10, 10), CVector2i(150, 20), "70", this, "LABEL", item, CONTROL_SNAP_BOTTOM);
    item = new CControl(lst_refreshRate, CVector2i(10, 10), CVector2i(150, 20), "72", this, "LABEL", item, CONTROL_SNAP_BOTTOM);
    item = new CControl(lst_refreshRate, CVector2i(10, 10), CVector2i(150, 20), "75", this, "LABEL", item, CONTROL_SNAP_BOTTOM);
    item = new CControl(lst_refreshRate, CVector2i(10, 10), CVector2i(150, 20), "85", this, "LABEL", item, CONTROL_SNAP_BOTTOM);
    if(gameVar.r_refreshRate == -1) lst_refreshRate->selectChild(0);
    if(gameVar.r_refreshRate == 60) lst_refreshRate->selectChild(1);
    if(gameVar.r_refreshRate == 70) lst_refreshRate->selectChild(2);
    if(gameVar.r_refreshRate == 72) lst_refreshRate->selectChild(3);
    if(gameVar.r_refreshRate == 75) lst_refreshRate->selectChild(4);
    if(gameVar.r_refreshRate == 85) lst_refreshRate->selectChild(5);

    //--- Team Indicator Type
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 90), "Team Indicator Type:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Choose a team indicator type.";
    lst_teamIndicatorType = new CControl(instance, CVector2i(10, 10), CVector2i(300, 90), "", this, "LISTBOX", label1, CONTROL_SNAP_RIGHT);
    item = new CControl(lst_teamIndicatorType, CVector2i(10, 10), CVector2i(250, 20), "Identify team by skin", this, "LABEL");
    item->toolTips = "Team affiliation is indicated by skin color, custom colors for skins are overwritten.";
    item = new CControl(lst_teamIndicatorType, CVector2i(10, 10), CVector2i(250, 20), "Identify all teams by halo", this, "LABEL", item, CONTROL_SNAP_BOTTOM);
    item->toolTips = "Team affiliation is indicated by a glowing halo around the babo, custom colors for skins are not overwritten.";
    item = new CControl(lst_teamIndicatorType, CVector2i(10, 10), CVector2i(250, 20), "Identify your team by halo", this, "LABEL", item, CONTROL_SNAP_BOTTOM);
    item->toolTips = "Team is indicated by a glowing halo around the babo, custom colors for skins are not overwritten.";
    lst_teamIndicatorType->selectChild(gameVar.cl_teamIndicatorType);

    //--- Weather Effects
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Weather effects:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Enable weather effects such as rain and fog.";
    chk_weatherEffects = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_weatherEffects->check = gameVar.r_weatherEffects;

    //--- Wall's shadow
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Wall's shadow:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    chk_wallShadow = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_wallShadow->check = (gameVar.r_shadowQuality == 2) ? true : false;

    //--- Player's shadow
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Player's shadow:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    chk_playerShadow = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_playerShadow->check = gameVar.r_playerShadow;

    //--- Projectile's shadow
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Projectile's shadow:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    chk_projectileShadow = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_projectileShadow->check = gameVar.r_projectileShadow;

    //--- Simple Text
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Basic Text:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Draw in a simplified way, usually increases frame rate";
    chk_simpleText = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_simpleText->check = gameVar.r_simpleText;

    //--- Casing
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Show casing:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Spawn bullet's casing when firing.";
    chk_showCasing = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_showCasing->check = gameVar.r_showCasing;

    //--- Gound mark
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Show ground mark:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Show blood and explosion mark on ground.";
    chk_groundMark = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_groundMark->check = gameVar.r_showGroundMark;



    //--- GUI OPTIONS
    separator = new CControl(instance, CVector2i(10, 20), CVector2i(200, 25), "User Interface options", this, "SEPARATOR", label1, CONTROL_SNAP_BOTTOM, 15);

    //--- Chat Text Size
    label1 = new CControl(instance, CVector2i(10, 10), CVector2i(200, 40), "Chat Text Size:", this, "LABEL", separator, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "The size of player chat text.\n";
    slc_chatTextSize = new CControl(instance, CVector2i(10, 10), CVector2i(300, 25), "28", this, "SLIDER", label1, CONTROL_SNAP_RIGHT);
    slc_chatTextSize->value = gameVar.r_chatTextSize;
    slc_chatTextSize->valueMin = 8;
    slc_chatTextSize->valueMax = 28;

    //--- Event Text Size
    label1 = new CControl(instance, CVector2i(10, 10), CVector2i(200, 40), "Game Event Text Size:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "The size of game events text.\n";
    slc_eventTextSize = new CControl(instance, CVector2i(10, 10), CVector2i(300, 25), "28", this, "SLIDER", label1, CONTROL_SNAP_RIGHT);
    slc_eventTextSize->value = gameVar.r_eventTextSize;
    slc_eventTextSize->valueMin = 8;
    slc_eventTextSize->valueMax = 28;

    //--- Show Event Text
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Show Game Event Text:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Show Game Event Text.";
    chk_showEventText = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_showEventText->check = gameVar.r_showEventText;

    //--- SOUND OPTIONS
    separator = new CControl(instance, CVector2i(10, 20), CVector2i(200, 25), "Sounds and music options", this, "SEPARATOR", label1, CONTROL_SNAP_BOTTOM, 15);

    //--- Show stats
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 70), "Sound quality:", this, "LABEL", separator, CONTROL_SNAP_BOTTOM, 15);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "(Need to reboot the game).";
    lst_mixRate = new CControl(instance, CVector2i(10, 10), CVector2i(300, 70), "", this, "LISTBOX", label1, CONTROL_SNAP_RIGHT);
    item = new CControl(lst_mixRate, CVector2i(10, 10), CVector2i(150, 20), "22050 Hz", this, "LABEL");
    item = new CControl(lst_mixRate, CVector2i(10, 10), CVector2i(150, 20), "44100 Hz", this, "LABEL", item, CONTROL_SNAP_BOTTOM);
    if(gameVar.s_mixRate == 22050) lst_mixRate->selectChild(0);
    if(gameVar.s_mixRate == 44100) lst_mixRate->selectChild(1);

    //--- Max software channel
    label1 = new CControl(instance, CVector2i(10, 10), CVector2i(200, 40), "Max software channels:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "The max number of sounds that can be played in same time.\nNote: With 16 channels bv2 can play all the sounds.";
    txt_maxSoftwareChannels = new CControl(instance, CVector2i(10, 10), CVector2i(300, 40), "16", this, "SLIDER", label1, CONTROL_SNAP_RIGHT);
    txt_maxSoftwareChannels->value = gameVar.s_maxSoftwareChannels;
    txt_maxSoftwareChannels->valueMin = 4;
    txt_maxSoftwareChannels->valueMax = 64;

    //--- Volume
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Master volume:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    slc_volume = new CControl(instance, CVector2i(10, 10), CVector2i(300, 25), "", this, "SLIDER", label1, CONTROL_SNAP_RIGHT);
    slc_volume->value = (int)(gameVar.s_masterVolume * 100);
    slc_volume->valueMin = 0;
    slc_volume->valueMax = 100;

    //--- In game music
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "In game music:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Play in game music.";
    chk_inGameMusic = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_inGameMusic->check = gameVar.s_inGameMusic;


    //--- KEY OPTIONS
    separator = new CControl(instance, CVector2i(10, 20), CVector2i(200, 25), "Key options", this, "SEPARATOR", label1, CONTROL_SNAP_BOTTOM, 15);

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Move up:", this, "LABEL", separator, CONTROL_SNAP_BOTTOM, 15);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Rolling up or forward.";
    key_moveUp = new CControl(instance, CVector2i(10, 10), CVector2i(100, 25),
        keyManager.getKeyName(clientVar.k_moveUp), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_moveUp->selectedIndex = clientVar.k_moveUp;

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Move down:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Rolling down or backward.";
    key_moveDown = new CControl(instance, CVector2i(10, 10), CVector2i(100, 25),
        keyManager.getKeyName(clientVar.k_moveDown), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_moveDown->selectedIndex = clientVar.k_moveDown;

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Move right:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Rolling right.";
    key_moveRight = new CControl(instance, CVector2i(10, 10), CVector2i(100, 25),
        keyManager.getKeyName(clientVar.k_moveRight), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_moveRight->selectedIndex = clientVar.k_moveRight;

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Move left:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Rolling left.";
    key_moveLeft = new CControl(instance, CVector2i(10, 10), CVector2i(100, 25),
        keyManager.getKeyName(clientVar.k_moveLeft), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_moveLeft->selectedIndex = clientVar.k_moveLeft;

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Shoot:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Trigger to shoot, and also used for spawning.";
    key_shoot = new CControl(instance, CVector2i(10, 10), CVector2i(100, 25),
        keyManager.getKeyName(clientVar.k_shoot), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_shoot->selectedIndex = clientVar.k_shoot;

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Use secondary:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Trigger to activate your secondary weapons or power up.";
    key_secondary = new CControl(instance, CVector2i(10, 10), CVector2i(100, 25),
        keyManager.getKeyName(clientVar.k_melee), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_secondary->selectedIndex = clientVar.k_melee;

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Throw grenade:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Throw a grenade.";
    key_throwGrenade = new CControl(instance, CVector2i(10, 10), CVector2i(100, 25),
        keyManager.getKeyName(clientVar.k_throwGrenade), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_throwGrenade->selectedIndex = clientVar.k_throwGrenade;

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Throw molotov:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Throw a molotov Cocktail.";
    key_throwMolotov = new CControl(instance, CVector2i(10, 10), CVector2i(100, 25),
        keyManager.getKeyName(clientVar.k_throwMolotov), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_throwMolotov->selectedIndex = clientVar.k_throwMolotov;

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Use key:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Pick up a gun on ground.\nPlank bomb.\nDesamorse bomb.";
    key_pickUp = new CControl(instance, CVector2i(10, 10), CVector2i(100, 25),
        keyManager.getKeyName(clientVar.k_pickUp), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_pickUp->selectedIndex = clientVar.k_pickUp;

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Chat all:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Chat to all (Friendly and ennemy).";
    key_chatAll = new CControl(instance, CVector2i(10, 10), CVector2i(100, 25),
        keyManager.getKeyName(clientVar.k_chatAll), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_chatAll->selectedIndex = clientVar.k_chatAll;

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Chat team:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Chat to team only.";
    key_chatTeam = new CControl(instance, CVector2i(10, 10), CVector2i(100, 25),
        keyManager.getKeyName(clientVar.k_chatTeam), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_chatTeam->selectedIndex = clientVar.k_chatTeam;

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Show score:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Show the score board.";
    key_showScore = new CControl(instance, CVector2i(10, 10), CVector2i(100, 25),
        keyManager.getKeyName(clientVar.k_showScore), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_showScore->selectedIndex = clientVar.k_showScore;

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Menu access:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Show the menu to select team and weapon.";
    key_menuAccess = new CControl(instance, CVector2i(10, 10), CVector2i(100, 25),
        keyManager.getKeyName(clientVar.k_menuAccess), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_menuAccess->selectedIndex = clientVar.k_menuAccess;

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Screen shot:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Take a screen shot.";
    key_screenShot = new CControl(instance, CVector2i(10, 10), CVector2i(100, 25),
        keyManager.getKeyName(clientVar.k_screenShot), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_screenShot->selectedIndex = clientVar.k_screenShot;

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Stats:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Write statistics.";
    key_stats = new CControl(instance, CVector2i(10, 10), CVector2i(100, 25),
        keyManager.getKeyName(clientVar.k_stats), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_stats->selectedIndex = clientVar.k_stats;

    //--- QUICK MESSAGES OPTIONS
    separator = new CControl(instance, CVector2i(10, 20), CVector2i(200, 25), "Quick messages' options", this, "SEPARATOR", label1, CONTROL_SNAP_BOTTOM, 15);

    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Quick Message  1:", this, "LABEL", separator, CONTROL_SNAP_BOTTOM, 15);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Quick Message 1.";
    txt_qMsg01 = new CControl(instance, CVector2i(10, 10), CVector2i(300, 25), CString("%s", &(gameVar.cl_qMsg01.s[1])), this, "EDIT", label1, CONTROL_SNAP_RIGHT);
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "To all:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Sends the message to everyone (or just the team).";
    chk_qMsg01 = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_qMsg01->check = gameVar.cl_qMsg01.s[0] == 'a';
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Key:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Select the preferd key.";
    key_qMsg01 = new CControl(instance, CVector2i(10, 10), CVector2i(200, 25), keyManager.getKeyName(clientVar.k_qMsg01), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_qMsg01->selectedIndex = clientVar.k_qMsg01;
    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Quick Message  2:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM, 5);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Quick Message 2.";
    txt_qMsg02 = new CControl(instance, CVector2i(10, 10), CVector2i(300, 25), CString("%s", &(gameVar.cl_qMsg02.s[1])), this, "EDIT", label1, CONTROL_SNAP_RIGHT);
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "To all:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Sends the message to everyone (or just the team).";
    chk_qMsg02 = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_qMsg02->check = gameVar.cl_qMsg02.s[0] == 'a';
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Key:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Select the preferd key.";
    key_qMsg02 = new CControl(instance, CVector2i(10, 10), CVector2i(200, 25), keyManager.getKeyName(clientVar.k_qMsg02), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_qMsg02->selectedIndex = clientVar.k_qMsg02;
    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Quick Message  3:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM, 5);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Quick Message 3.";
    txt_qMsg03 = new CControl(instance, CVector2i(10, 10), CVector2i(300, 25), CString("%s", &(gameVar.cl_qMsg03.s[1])), this, "EDIT", label1, CONTROL_SNAP_RIGHT);
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "To all:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Sends the message to everyone (or just the team).";
    chk_qMsg03 = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_qMsg03->check = gameVar.cl_qMsg03.s[0] == 'a';
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Key:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Select the preferd key.";
    key_qMsg03 = new CControl(instance, CVector2i(10, 10), CVector2i(200, 25), keyManager.getKeyName(clientVar.k_qMsg03), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_qMsg03->selectedIndex = clientVar.k_qMsg03;
    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Quick Message  4:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM, 5);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Quick Message 4.";
    txt_qMsg04 = new CControl(instance, CVector2i(10, 10), CVector2i(300, 25), CString("%s", &(gameVar.cl_qMsg04.s[1])), this, "EDIT", label1, CONTROL_SNAP_RIGHT);
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "To all:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Sends the message to everyone (or just the team).";
    chk_qMsg04 = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_qMsg04->check = gameVar.cl_qMsg04.s[0] == 'a';
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Key:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Select the preferd key.";
    key_qMsg04 = new CControl(instance, CVector2i(10, 10), CVector2i(200, 25), keyManager.getKeyName(clientVar.k_qMsg04), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_qMsg04->selectedIndex = clientVar.k_qMsg04;
    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Quick Message  5:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM, 5);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Quick Message 5.";
    txt_qMsg05 = new CControl(instance, CVector2i(10, 10), CVector2i(300, 25), CString("%s", &(gameVar.cl_qMsg05.s[1])), this, "EDIT", label1, CONTROL_SNAP_RIGHT);
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "To all:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Sends the message to everyone (or just the team).";
    chk_qMsg05 = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_qMsg05->check = gameVar.cl_qMsg05.s[0] == 'a';
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Key:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Select the preferd key.";
    key_qMsg05 = new CControl(instance, CVector2i(10, 10), CVector2i(200, 25), keyManager.getKeyName(clientVar.k_qMsg05), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_qMsg05->selectedIndex = clientVar.k_qMsg05;
    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Quick Message  6:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM, 5);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Quick Message 6.";
    txt_qMsg06 = new CControl(instance, CVector2i(10, 10), CVector2i(300, 25), CString("%s", &(gameVar.cl_qMsg06.s[1])), this, "EDIT", label1, CONTROL_SNAP_RIGHT);
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "To all:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Sends the message to everyone (or just the team).";
    chk_qMsg06 = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_qMsg06->check = gameVar.cl_qMsg06.s[0] == 'a';
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Key:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Select the preferd key.";
    key_qMsg06 = new CControl(instance, CVector2i(10, 10), CVector2i(200, 25), keyManager.getKeyName(clientVar.k_qMsg06), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_qMsg06->selectedIndex = clientVar.k_qMsg06;
    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Quick Message  7:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM, 5);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Quick Message 7.";
    txt_qMsg07 = new CControl(instance, CVector2i(10, 10), CVector2i(300, 25), CString("%s", &(gameVar.cl_qMsg07.s[1])), this, "EDIT", label1, CONTROL_SNAP_RIGHT);
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "To all:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Sends the message to everyone (or just the team).";
    chk_qMsg07 = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_qMsg07->check = gameVar.cl_qMsg07.s[0] == 'a';
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Key:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Select the preferd key.";
    key_qMsg07 = new CControl(instance, CVector2i(10, 10), CVector2i(200, 25), keyManager.getKeyName(clientVar.k_qMsg07), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_qMsg07->selectedIndex = clientVar.k_qMsg07;
    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Quick Message  8:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM, 5);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Quick Message 8.";
    txt_qMsg08 = new CControl(instance, CVector2i(10, 10), CVector2i(300, 25), CString("%s", &(gameVar.cl_qMsg08.s[1])), this, "EDIT", label1, CONTROL_SNAP_RIGHT);
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "To all:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Sends the message to everyone (or just the team).";
    chk_qMsg08 = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_qMsg08->check = gameVar.cl_qMsg08.s[0] == 'a';
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Key:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Select the preferd key.";
    key_qMsg08 = new CControl(instance, CVector2i(10, 10), CVector2i(200, 25), keyManager.getKeyName(clientVar.k_qMsg08), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_qMsg08->selectedIndex = clientVar.k_qMsg08;
    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Quick Message  9:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM, 5);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Quick Message 9.";
    txt_qMsg09 = new CControl(instance, CVector2i(10, 10), CVector2i(300, 25), CString("%s", &(gameVar.cl_qMsg09.s[1])), this, "EDIT", label1, CONTROL_SNAP_RIGHT);
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "To all:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Sends the message to everyone (or just the team).";
    chk_qMsg09 = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_qMsg09->check = gameVar.cl_qMsg09.s[0] == 'a';
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Key:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Select the preferd key.";
    key_qMsg09 = new CControl(instance, CVector2i(10, 10), CVector2i(200, 25), keyManager.getKeyName(clientVar.k_qMsg09), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_qMsg09->selectedIndex = clientVar.k_qMsg09;
    //--- Key
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Quick Message 10:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM, 5);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Quick Message 10.";
    txt_qMsg10 = new CControl(instance, CVector2i(10, 10), CVector2i(300, 25), CString("%s", &(gameVar.cl_qMsg10.s[1])), this, "EDIT", label1, CONTROL_SNAP_RIGHT);
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "To all:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Sends the message to everyone (or just the team).";
    chk_qMsg10 = new CControl(instance, CVector2i(10, 10), CVector2i(25, 25), "", this, "CHECK", label1, CONTROL_SNAP_RIGHT);
    chk_qMsg10->check = gameVar.cl_qMsg10.s[0] == 'a';
    label1 = new CControl(instance, CVector2i(20, 10), CVector2i(200, 25), "Key:", this, "LABEL", label1, CONTROL_SNAP_BOTTOM);
    label1->textAlign = CONTROL_TEXTALIGN_MIDDLERIGHT;
    label1->toolTips = "Select the preferd key.";
    key_qMsg10 = new CControl(instance, CVector2i(10, 10), CVector2i(200, 25), keyManager.getKeyName(clientVar.k_qMsg10), this, "KEY", label1, CONTROL_SNAP_RIGHT);
    key_qMsg10->selectedIndex = clientVar.k_qMsg10;

    //--- FINAL SEPARATOR
    separator = new CControl(instance, CVector2i(10, 20), CVector2i(200, 25), "", this, "SEPARATOR", label1, CONTROL_SNAP_BOTTOM, 15);


    instance->backColor.set(0, .3f, .7f);
    instance->imgColor = instance->backColor;

    animY = 0;
    velY = 0;
    originalY = instance->localPos[1];
}


COption::~COption()
{
    dksDeleteSound(m_sfxClic);
    dksDeleteSound(m_sfxOver);
}


void COption::MouseEnter(CControl * control)
{
    if(control->style == "BUTTON" || control->style == "LISTBOX" || control->style == "CHECK" || control->style == "KEY")
    {
        dksPlaySound(m_sfxOver, -1, 200);
    }
}
void COption::MouseLeave(CControl * control)
{
}
void COption::MouseDown(CControl * control)
{
}
void COption::MouseUp(CControl * control)
{
}
void COption::MouseMove(CControl * control)
{
}
void COption::Click(CControl * control)
{
    auto cscene = static_cast<ClientScene*>(scene);
    if(control->style == "EDIT" || control->style == "BUTTON" || control->style == "LISTBOX" || control->style == "CHECK" || control->style == "KEY")
    {
        dksPlaySound(m_sfxClic, -1, 200);
    }

    /*  if (control == btn_apply)
        {*/
        //--- Save all da settings!

        // Rendering options
    gameVar.r_showStats = chk_showStats->check;
    gameVar.r_showLatency = chk_showLatency->check;
    gameVar.r_fullScreen = chk_fullScreen->check;
    gameVar.cl_preciseCursor = chk_preciseCursor->check;

    switch(lst_resolution->selectedIndex)
    {
    case 0: gameVar.r_resolution.set(640, 360); break;
    case 1: gameVar.r_resolution.set(1280, 720); break;
    case 2: gameVar.r_resolution.set(1920, 1080); break;
    }

    switch(lst_refreshRate->selectedIndex)
    {
    case 0: gameVar.r_refreshRate = -1; break;
    case 1: gameVar.r_refreshRate = 60; break;
    case 2: gameVar.r_refreshRate = 70; break;
    case 3: gameVar.r_refreshRate = 72; break;
    case 4: gameVar.r_refreshRate = 75; break;
    case 5: gameVar.r_refreshRate = 85; break;
    default: gameVar.r_refreshRate = -1; break;
    }

    gameVar.cl_teamIndicatorType = lst_teamIndicatorType->selectedIndex;

    gameVar.r_shadowQuality = (chk_wallShadow->check) ? 2 : 0;
    gameVar.r_playerShadow = chk_playerShadow->check;
    gameVar.r_projectileShadow = chk_projectileShadow->check;
    gameVar.r_showCasing = chk_showCasing->check;
    gameVar.r_showGroundMark = chk_groundMark->check;
    gameVar.r_highDetailMenu = chk_highDetailMenus->check;
    gameVar.r_animatedMenu = chk_animatedMenus->check;
    gameVar.r_simpleText = chk_simpleText->check;

    // Must reload weather if changed
    if(gameVar.r_weatherEffects != chk_weatherEffects->check) {
        gameVar.r_weatherEffects = chk_weatherEffects->check;
        if(cscene && cscene->client && cscene->client->game && cscene->client->game->map)
        {
            auto cmap = static_cast<ClientMap*>(cscene->client->game->map);
            cmap->reloadWeather();
        }
    }


    // GUI Options
    gameVar.r_chatTextSize = slc_chatTextSize->value;
    gameVar.r_eventTextSize = slc_eventTextSize->value;
    gameVar.r_showEventText = chk_showEventText->check;


    // Sound options
    switch(lst_mixRate->selectedIndex)
    {
    case 0: gameVar.s_mixRate = 22050; break;
    case 1: gameVar.s_mixRate = 44100; break;
    default: gameVar.s_mixRate = 22050; break;
    }
    gameVar.s_maxSoftwareChannels = txt_maxSoftwareChannels->value;
    gameVar.s_masterVolume = (float)(slc_volume->value) / 100.0f;
    gameVar.s_inGameMusic = chk_inGameMusic->check;

    // Key options
    clientVar.k_moveUp = key_moveUp->selectedIndex;
    clientVar.k_moveDown = key_moveDown->selectedIndex;
    clientVar.k_moveRight = key_moveRight->selectedIndex;
    clientVar.k_moveLeft = key_moveLeft->selectedIndex;
    clientVar.k_shoot = key_shoot->selectedIndex;
    clientVar.k_throwGrenade = key_throwGrenade->selectedIndex;
    clientVar.k_throwMolotov = key_throwMolotov->selectedIndex;
    clientVar.k_pickUp = key_pickUp->selectedIndex;
    clientVar.k_chatAll = key_chatAll->selectedIndex;
    clientVar.k_chatTeam = key_chatTeam->selectedIndex;
    clientVar.k_showScore = key_showScore->selectedIndex;
    clientVar.k_menuAccess = key_menuAccess->selectedIndex;

    clientVar.k_screenShot = key_screenShot->selectedIndex;
    clientVar.k_stats = key_stats->selectedIndex;

    gameVar.cl_qMsg01.set("%c%s", (chk_qMsg01->check ? 'a' : 't'), txt_qMsg01->text.s);
    gameVar.cl_qMsg02.set("%c%s", (chk_qMsg02->check ? 'a' : 't'), txt_qMsg02->text.s);
    gameVar.cl_qMsg03.set("%c%s", (chk_qMsg03->check ? 'a' : 't'), txt_qMsg03->text.s);
    gameVar.cl_qMsg04.set("%c%s", (chk_qMsg04->check ? 'a' : 't'), txt_qMsg04->text.s);
    gameVar.cl_qMsg05.set("%c%s", (chk_qMsg05->check ? 'a' : 't'), txt_qMsg05->text.s);
    gameVar.cl_qMsg06.set("%c%s", (chk_qMsg06->check ? 'a' : 't'), txt_qMsg06->text.s);
    gameVar.cl_qMsg07.set("%c%s", (chk_qMsg07->check ? 'a' : 't'), txt_qMsg07->text.s);
    gameVar.cl_qMsg08.set("%c%s", (chk_qMsg08->check ? 'a' : 't'), txt_qMsg08->text.s);
    gameVar.cl_qMsg09.set("%c%s", (chk_qMsg09->check ? 'a' : 't'), txt_qMsg09->text.s);
    gameVar.cl_qMsg10.set("%c%s", (chk_qMsg10->check ? 'a' : 't'), txt_qMsg10->text.s);

    clientVar.k_qMsg01 = key_qMsg01->selectedIndex;
    clientVar.k_qMsg02 = key_qMsg02->selectedIndex;
    clientVar.k_qMsg03 = key_qMsg03->selectedIndex;
    clientVar.k_qMsg04 = key_qMsg04->selectedIndex;
    clientVar.k_qMsg05 = key_qMsg05->selectedIndex;
    clientVar.k_qMsg06 = key_qMsg06->selectedIndex;
    clientVar.k_qMsg07 = key_qMsg07->selectedIndex;
    clientVar.k_qMsg08 = key_qMsg08->selectedIndex;
    clientVar.k_qMsg09 = key_qMsg09->selectedIndex;
    clientVar.k_qMsg10 = key_qMsg10->selectedIndex;
    //  }
}
void COption::Validate(CControl * control)
{
    auto cscene = static_cast<ClientScene*>(scene);
    if(control->style == "EDIT" || control->style == "KEY")
    {
        dksPlaySound(m_sfxClic, -1, 200);
    }

    //--- Save all da settings!

    //gameVar.cl_playerName = txt_playerName->text;

    // Rendering options
    gameVar.r_showStats = chk_showStats->check;
    gameVar.r_showLatency = chk_showLatency->check;
    gameVar.r_fullScreen = chk_fullScreen->check;
    gameVar.cl_preciseCursor = chk_preciseCursor->check;

    switch(lst_resolution->selectedIndex)
    {
    default:
    case 0: gameVar.r_resolution.set(640, 360); break;
    case 1: gameVar.r_resolution.set(1280, 720); break;
    case 2: gameVar.r_resolution.set(1920, 1080); break;
    }

    gameVar.r_shadowQuality = (chk_wallShadow->check) ? 2 : 0;
    gameVar.r_playerShadow = chk_playerShadow->check;
    gameVar.r_projectileShadow = chk_projectileShadow->check;
    gameVar.r_showCasing = chk_showCasing->check;
    gameVar.r_showGroundMark = chk_groundMark->check;
    gameVar.r_highDetailMenu = chk_highDetailMenus->check;
    gameVar.r_animatedMenu = chk_animatedMenus->check;

    // Must reload weather if changed
    if(gameVar.r_weatherEffects != chk_weatherEffects->check) {
        gameVar.r_weatherEffects = chk_weatherEffects->check;
        if(cscene && cscene->client && cscene->client->game && cscene->client->game->map)
        {
            auto cmap = static_cast<ClientMap*>(cscene->client->game->map);
            cmap->reloadWeather();
        }
    }

    switch(lst_refreshRate->selectedIndex)
    {
    case 0: gameVar.r_refreshRate = -1; break;
    case 1: gameVar.r_refreshRate = 60; break;
    case 2: gameVar.r_refreshRate = 70; break;
    case 3: gameVar.r_refreshRate = 72; break;
    case 4: gameVar.r_refreshRate = 75; break;
    case 5: gameVar.r_refreshRate = 85; break;
    default: gameVar.r_refreshRate = -1; break;
    }
    // GUI Options
    gameVar.r_chatTextSize = slc_chatTextSize->value;
    gameVar.r_eventTextSize = slc_eventTextSize->value;
    gameVar.r_showEventText = chk_showEventText->check;

    // Sound options
    switch(lst_mixRate->selectedIndex)
    {
    case 0: gameVar.s_mixRate = 22050; break;
    case 1: gameVar.s_mixRate = 44100; break;
    default: gameVar.s_mixRate = 22050; break;
    }
    gameVar.s_maxSoftwareChannels = txt_maxSoftwareChannels->value;
    gameVar.s_masterVolume = (float)(slc_volume->value) / 100.0f;
    gameVar.s_inGameMusic = chk_inGameMusic->check;

    // Key options
    clientVar.k_moveUp = key_moveUp->selectedIndex;
    clientVar.k_moveDown = key_moveDown->selectedIndex;
    clientVar.k_moveRight = key_moveRight->selectedIndex;
    clientVar.k_moveLeft = key_moveLeft->selectedIndex;
    clientVar.k_shoot = key_shoot->selectedIndex;
    clientVar.k_melee = key_secondary->selectedIndex;
    clientVar.k_throwGrenade = key_throwGrenade->selectedIndex;
    clientVar.k_throwMolotov = key_throwMolotov->selectedIndex;
    clientVar.k_pickUp = key_pickUp->selectedIndex;
    clientVar.k_chatAll = key_chatAll->selectedIndex;
    clientVar.k_chatTeam = key_chatTeam->selectedIndex;
    clientVar.k_showScore = key_showScore->selectedIndex;
    clientVar.k_menuAccess = key_menuAccess->selectedIndex;

    clientVar.k_screenShot = key_screenShot->selectedIndex;
    clientVar.k_stats = key_stats->selectedIndex;

    gameVar.cl_qMsg01.set("%c%s", (chk_qMsg01->check ? 'a' : 't'), txt_qMsg01->text.s);
    gameVar.cl_qMsg02.set("%c%s", (chk_qMsg02->check ? 'a' : 't'), txt_qMsg02->text.s);
    gameVar.cl_qMsg03.set("%c%s", (chk_qMsg03->check ? 'a' : 't'), txt_qMsg03->text.s);
    gameVar.cl_qMsg04.set("%c%s", (chk_qMsg04->check ? 'a' : 't'), txt_qMsg04->text.s);
    gameVar.cl_qMsg05.set("%c%s", (chk_qMsg05->check ? 'a' : 't'), txt_qMsg05->text.s);
    gameVar.cl_qMsg06.set("%c%s", (chk_qMsg06->check ? 'a' : 't'), txt_qMsg06->text.s);
    gameVar.cl_qMsg07.set("%c%s", (chk_qMsg07->check ? 'a' : 't'), txt_qMsg07->text.s);
    gameVar.cl_qMsg08.set("%c%s", (chk_qMsg08->check ? 'a' : 't'), txt_qMsg08->text.s);
    gameVar.cl_qMsg09.set("%c%s", (chk_qMsg09->check ? 'a' : 't'), txt_qMsg09->text.s);
    gameVar.cl_qMsg10.set("%c%s", (chk_qMsg10->check ? 'a' : 't'), txt_qMsg10->text.s);

    clientVar.k_qMsg01 = key_qMsg01->selectedIndex;
    clientVar.k_qMsg02 = key_qMsg02->selectedIndex;
    clientVar.k_qMsg03 = key_qMsg03->selectedIndex;
    clientVar.k_qMsg04 = key_qMsg04->selectedIndex;
    clientVar.k_qMsg05 = key_qMsg05->selectedIndex;
    clientVar.k_qMsg06 = key_qMsg06->selectedIndex;
    clientVar.k_qMsg07 = key_qMsg07->selectedIndex;
    clientVar.k_qMsg08 = key_qMsg08->selectedIndex;
    clientVar.k_qMsg09 = key_qMsg09->selectedIndex;
    clientVar.k_qMsg10 = key_qMsg10->selectedIndex;
}
