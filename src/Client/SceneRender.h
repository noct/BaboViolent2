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
#ifndef CLIENT_SCENE_RENDER_H
#define CLIENT_SCENE_RENDER_H

#include <Zeven/Core.h>
#include <vector>

struct Client;
struct ClientConsole;
struct ClientGame;
struct ClientPlayer;
struct ClientProjectile;
struct ClientScene;
struct ClientWeapon;
struct Douille;
struct Drip;
struct FloorMark;
struct IntroScreen;
struct NukeFlash;
struct NuzzleFlash;
struct Player;
struct Trail;

void Client_Render(Client* client, float & alphaScope);
void ClientConsole_Render(ClientConsole* console);
void ClientGame_Render(ClientGame* game);
void ClientGame_RenderBlueTeam(ClientGame* game, std::vector<Player*> & teamOrder, int & vPos);
void ClientGame_RenderFFA(ClientGame* game, std::vector<Player*> & teamOrder, int & vPos);
void ClientGame_RenderMiniMap(ClientGame* game);
void ClientGame_RenderRedTeam(ClientGame* game, std::vector<Player*> & teamOrder, int & vPos);
void ClientGame_RenderSpectator(ClientGame* game, std::vector<Player*> & teamOrder, int & vPos);
void ClientGame_RenderStats(ClientGame* game);
void ClientPlayer_Render(ClientPlayer* player);
void ClientPlayer_RenderName(ClientPlayer* player);
void ClientProjectile_Render(ClientProjectile* proj);
void ClientProjectile_RenderShadow(ClientProjectile* proj);
void ClientScene_Render(ClientScene* scene);
void ClientWeapon_Render(ClientWeapon* weapon);
void Douille_Render(Douille* douille);
void Drip_Render(Drip* drip);
void FloorMark_Render(FloorMark* mark);
void IntroScreen_Render(IntroScreen* intro);
void NukeFlash_Render(NukeFlash* nuke);
void NuzzleFlash_Render(NuzzleFlash* nf);
void Trail_Render(Trail* trail);
void Trail_RenderBullet(Trail* trail);

// Pour êrire du texte centrê
void printCenterText(float x, float y, float size, bool enableShadow, const CString & text);
void printLeftText(float x, float y, float size, bool enableShadow, const CString & text);
void printRightText(float x, float y, float size, bool enableShadow, const CString & text);

// Pour afficher un quad truc simple de meme
void renderTexturedQuad(int x, int y, int w, int h, unsigned int texture);
void renderTexturedQuadSmooth(int x, int y, int w, int h, unsigned int texture);
//void renderMenuQuad(int x, int y, int w, int h);
void renderMenuQuad(int x, int y, int w, int h);

void renderLoadingScreen(unsigned int font);

#endif
