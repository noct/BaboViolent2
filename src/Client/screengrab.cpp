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
#include "screengrab.h"
#include "ClientScene.h"
#include <ctime>

extern Scene* scene;

bool SaveScreenGrabAuto()
{
    char path[512];
    time_t time;
    ::time(&time);
    sprintf(path, "SS_%d.bmp", time);
    return SaveScreenGrab(path);
}

bool SaveStatsAuto()
{
    char path[512];
    time_t time;
    ::time(&time);
    sprintf(path, "SS_%d.bmp", time);
    SaveScreenGrab(path);
    sprintf(path, "SS_%d.txt", time);

    FILE * pFile;
    pFile = fopen(path, "w");
    if(pFile != NULL)
    {
        auto cscene = static_cast<ClientScene*>(scene);
        Game* pGame = cscene->client->game;

        fputs(pGame->mapName.s, pFile);
        fprintf(pFile, "\nBlue:%d\nRed:%d\n", pGame->blueScore, pGame->redScore);

        for(int i = 0; i < MAX_PLAYER; ++i)
        {
            if(pGame->players[i])
            {
                Player* pPlayer = pGame->players[i];
                fprintf(pFile, "PlayerName:%s\n", textColorLess(pPlayer->name).s);
                fprintf(pFile, "PlayerId:%d\nTeamId:%d\n", pPlayer->playerID, pPlayer->teamID);
                fprintf(pFile, "Kills:%d\n", pPlayer->kills);
                fprintf(pFile, "Deaths:%d\n", pPlayer->deaths);
                fprintf(pFile, "Damage:%f\n", pPlayer->dmg);
                fprintf(pFile, "FlagAttempts:%d\n", pPlayer->flagAttempts);
                fprintf(pFile, "Returns:%d\n", pPlayer->returns);
                fprintf(pFile, "Score:%d\n", pPlayer->score);
            }
        }

        fclose(pFile);
    }

    return true;
}



bool SaveScreenGrab(const char* filename)
{
    return true;
}
