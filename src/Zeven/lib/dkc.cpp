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

/* RndLabs (c) All rights reserved */
#include <cstdint>
#include <Windows.h>


// La class static pour contenir le tout
class CDkc
{
public:
    static int32_t frame; // Le frame qu'on est rendu
    static int64_t lastFrameCount; // Le temps au dernier frame
    static float elapsedf; // Le temps �oul�entre deux frames, pour les animations
    static float currentFrameDelay; // Le temps qu'on a de fait sur ce frame
    static bool usePerformanceTimer; // Si on utilise ou pas le performance timer (plus pr�is)
    static int64_t frequency; // La fr�uence utilis�
    static float perSeconde; // C'est pour computer le nb de frame par seconde
    static int framePerSeconde;
    static float fps; // Le frame rate
    static int oneSecondFrameCound;
    static float oneSecondElapsedcpt;
public:
};

//
// Les trucs static
//
int32_t CDkc::frame=0; // Le frame qu'on est rendu
int64_t CDkc::lastFrameCount=0; // Le temps au dernier frame
float CDkc::elapsedf=0; // Le temps �oul�entre deux frames, pour les animations
float CDkc::currentFrameDelay=0; // Le temps qu'on a de fait sur ce frame
bool CDkc::usePerformanceTimer=false;
int64_t CDkc::frequency=1000;
float CDkc::perSeconde=40; // Par default 25 frames par seconde
int CDkc::framePerSeconde=25;
float CDkc::fps = 0;
int CDkc::oneSecondFrameCound = 0;
float CDkc::oneSecondElapsedcpt = 0;

//
// The elapsed time in seconde
//
float dkcGetElapsedf()
{
	return CDkc::perSeconde;
}

//
// Obtenir le frame per second
//
float dkcGetFPS()
{
	return CDkc::fps;
}

//
// Pour obtenir le nb de frame o on est rendu
//
int32_t dkcGetFrame()
{
	return CDkc::frame;
}

//
// Init the timer (do at your program start)
//
void dkcInit(int framePerSecond)
{
	CDkc::framePerSeconde = framePerSecond;

	#ifdef WIN32
		// On check si on peut utiliser un timer de haute performance
		if (!QueryPerformanceFrequency((LARGE_INTEGER *) &(CDkc::frequency)))
		{
			CDkc::usePerformanceTimer = false;
			CDkc::frequency = 1000;
		}
		else
		{
			CDkc::usePerformanceTimer = true;
		}
	#else
		//		CDkc::frequency = sysconf(_SC_CLK_TCK);
		CDkc::frequency = 1000000LL;
	#endif

	// Pour savoir le bon delait pour le nb de frame par seconde
	CDkc::perSeconde = 1.0f / (float)(CDkc::framePerSeconde);
}

//
// To step a couple of frame or to init it to 0
//
void dkcJumpToFrame(int frame)
{
	CDkc::frame = frame;
	CDkc::currentFrameDelay = 0;
	CDkc::lastFrameCount=0;
	CDkc::elapsedf=0;
	CDkc::currentFrameDelay=0;
	CDkc::fps = 0;
	CDkc::oneSecondFrameCound = 0;
	CDkc::oneSecondElapsedcpt = 0;
}

//
// Will return the current frame count
//
int32_t dkcUpdateTimer()
{
	// On prend le nombre de tick du CPU
	int64_t lGetTickCount;

	#ifdef WIN32
		if (CDkc::usePerformanceTimer)
		{
			QueryPerformanceCounter((LARGE_INTEGER *) &lGetTickCount);
		}
		else
		{
			lGetTickCount = GetTickCount();
		}
	#else
		//get the current number of microseconds since january 1st 1970

		tms tBuf;
		//int ct = times(&tBuf);

		//printf("ct = %i\n",ct);

		lGetTickCount = times(&tBuf);

            struct timeval tv;
            gettimeofday(& tv, 0);
            lGetTickCount = tv.tv_sec * 1000000 + tv.tv_usec;

	#endif

	// On update le timer
	double elapsedd = (double)((CDkc::lastFrameCount) ? lGetTickCount - CDkc::lastFrameCount : 0) / (double)CDkc::frequency;
	CDkc::lastFrameCount = lGetTickCount;
	CDkc::elapsedf = (float)elapsedd;
	CDkc::currentFrameDelay += CDkc::elapsedf;
    int32_t nbFrameAdded=0;
	while (CDkc::currentFrameDelay >= CDkc::perSeconde)
	{
		CDkc::currentFrameDelay -= CDkc::perSeconde;
		CDkc::frame++;
		nbFrameAdded++;
	}

	// On update pour le fps
	CDkc::oneSecondElapsedcpt += CDkc::elapsedf;
	CDkc::oneSecondFrameCound++;
	while (CDkc::oneSecondElapsedcpt >= 1)
	{
		CDkc::oneSecondElapsedcpt -= 1;
		CDkc::fps = (float)CDkc::oneSecondFrameCound;
		CDkc::oneSecondFrameCound = 0;
	}

	// On retourne le nombre de frame �animer
	return nbFrameAdded;
}



void dkcSleep(int32_t ms)
{
	#ifdef WIN32
		int32_t timeMem = GetTickCount();
		int32_t currentTime = timeMem;
		while (currentTime - timeMem < ms)
		{
			currentTime = GetTickCount();
		}
	#else
		struct timespec t;
		t.tv_sec = 0;
		t.tv_nsec = ms * 1000000L;
		nanosleep(& t, 0);
	#endif
}
