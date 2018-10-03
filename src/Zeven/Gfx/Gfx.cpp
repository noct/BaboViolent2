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
#include <Zeven/Gfx.h>
#include <glad/glad.h>
#include <SDL2/SDL.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"

#include <fstream>
#include <cmath>
#include <string>
#include <sys/stat.h>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct typ_characterProp {
    float u1, v1;
    float u2, v2;
    float w;

    typ_characterProp() {
        u1 = 0; v1 = 0;
        u2 = 0; v2 = 0;
        w = 0;
    }
};

struct CFont
{
    // Le fichier Font .fnt
    CString filename;

    // La texture utilisé (ogl)
    unsigned int textureID;

    // La grosseur de chaque lettre
    int kerning[256];
    float finalCaracKerning[256];

    // La hauteur des lettres
    int height;

    // La liste de display list contenant les caractère individuel
    unsigned int baseFont;

    // Son ID du dkf
    unsigned int fontID;

    // Les propriétées de chaque lettre
    typ_characterProp characterProp[256];

    // Si cette font est tga ou fnt
    bool fntFont;

    // Le nb d'instance
    int nbInstance;
};



static CVector2i mousePos(0, 0);
static int allState[256 + 8 + 128];
static int lastDown = 0;
static float downTimer = 0;
static std::string last_error = "";
static SDL_Window* g_window = nullptr;
static SDL_GLContext g_gl_context = nullptr;
static bool done = true;
static CMainLoopInterface *mainLoopObject = nullptr;
static std::vector<CFont*> fonts;
static unsigned int currentIDCount = 0;
static CFont *currentBind = 0;


//
// Constructeur / Destructeur
//
static void Font_Init(CFont* font)
{
    font->nbInstance = 1;
    font->textureID = 0;
    font->baseFont = 0;
    font->fontID = 0;
    font->fntFont = false;
    for(int i = 0; i < 256; i++)
    {
        font->kerning[i] = 0;
        font->finalCaracKerning[i] = 0;
    }
}

static void Font_destroy(CFont* font)
{
    if(font->baseFont) glDeleteLists(font->baseFont, 128);
    font->baseFont = 0;
}

static void Font_Fini(CFont* font)
{
    Font_destroy(font);
    dktDeleteTexture(&font->textureID);
}

static int Font_loadFontFile(CFont* font, std::ifstream &fntFile)
{
    char variable[256];
    fntFile >> variable;

    while(!fntFile.eof())
    {
        if(variable[0] == '/' && variable[0] == '/')
        {
            fntFile.ignore(512, '\n');
            fntFile >> variable;
            continue;
        }

        if(stricmp(variable, "TEXTURE") == 0)
        {
            fntFile >> variable;
            // On cré notre texture
            font->textureID = dktCreateTextureFromFile(variable, DKT_FILTER_BILINEAR);
        }

        if(stricmp(variable, "HEIGHT") == 0)
        {
            fntFile >> font->height;
        }

        if(stricmp(variable, "KERNING") == 0)
        {
            int current = 0;
            fntFile >> variable;
            while(!fntFile.eof() && !(stricmp(variable, ";") == 0))
            {
                if(stricmp(variable, "OFFSET") == 0)
                {
                    font->kerning[current++] = -1;
                    fntFile >> font->kerning[current++];
                    fntFile >> variable;
                    continue;
                }
                if(stricmp(variable, "NEWLINE") == 0)
                {
                    font->kerning[current++] = -2;
                    fntFile >> variable;
                    continue;
                }

                // Sinon on load le charactère
                fntFile >> font->kerning[current++];
                fntFile >> variable;
            }
        }

        fntFile >> variable;
    }

    font->fntFont = true;
    return 1;
}

static int Font_loadTGAFile(CFont* font, char * tgaFile)
{
    // On cré notre texture
    font->textureID = dktCreateTextureFromFile(tgaFile, DKT_FILTER_BILINEAR);

    // Bon, ben on cré notre font avec cette texture là
    if(font->textureID)
    {
        font->fntFont = false;
        unsigned char * alphaData = new unsigned char[512 * 512];
        glPushAttrib(GL_ENABLE_BIT);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, font->textureID);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_ALPHA, GL_UNSIGNED_BYTE, alphaData);
        glPopAttrib();

        // Chaque caracter fait 64 de haut
        font->characterProp[32].u1 = 0;
        font->characterProp[32].v1 = 0;
        font->characterProp[32].u2 = 0;
        font->characterProp[32].v2 = 0;
        font->characterProp[32].w = .25f;
        font->finalCaracKerning[32] = font->characterProp[32].w;

        int charH = 64;
        int curX = 0;
        int curY = 0;
        for(int c = 33; c < 128 + 32; c++) {
            // Bon, on compte les pixel yééé
            bool nextCarac = false;
            while(!nextCarac) {
                for(int j = 0; j < charH; j++) {
                    if(alphaData[(512 - (j + curY) - 1) * 512 + (curX)] > 0) {
                        int from = curX - 1;
                        int to;
                        if(from < 0) from = 0;

                        // Maintenant on check pour la fin du caractère
                        while(true)
                        {
                            bool allFalse = true;
                            for(int jj = 0; jj < charH; jj++)
                            {
                                if(alphaData[(512 - (jj + curY) - 1) * 512 + (curX)] > 0)
                                {
                                    allFalse = false;
                                    break;
                                }
                            }
                            if(allFalse || curX >= 512 - 1)
                            {
                                to = curX + 1;
                                if(to >= 512) to = 512;
                                break;
                            }
                            curX++;
                        }

                        // Bon, maintenant on peut créer ce character // 25, 34
                        font->characterProp[c].w = (float)(to - from) / (float)charH;
                        font->characterProp[c].u1 = (float)from / (float)512;
                        font->characterProp[c].u2 = (float)to / (float)512;
                        font->characterProp[c].v1 = 1 - (float)curY / (float)512;
                        font->characterProp[c].v2 = 1 - (float)(curY + charH) / (float)512;
                        font->finalCaracKerning[c] = font->characterProp[c].w;

                        nextCarac = true;
                        break;
                    }
                }
                curX++;
                if(curX >= 512) {
                    curX = 0;
                    curY += charH;
                    if(curY >= 512) {
                        c = 128 + 32; // On a fini
                        nextCarac = true;
                        break;
                    }
                }
            }
        }

        delete[] alphaData;
    }

    return font->textureID;
}

static int Font_create(CFont* font, CString path)
{
    font->filename = path;
    return Font_loadTGAFile(font, font->filename.s);

    // On ouvre le fichier de définition
    std::ifstream fntFile(font->filename.s, std::ios::in);

    // On check si ça fonctionné
    if(fntFile.fail())
    {
        return 0;
    }

    // On load le tout!
    if(!Font_loadFontFile(font, fntFile)) { fntFile.close(); return 0; }

    // On start l'application et on run
    fntFile.close();

    return 1;
}

static void Font_reloadIt(CFont* font)
{
    // On détruit l'encien avant
    Font_destroy(font);

    // On génère 256 display list pour stocker tout les caractères
    font->baseFont = glGenLists(256);

    if(font->fntFont)
    {
        // Variable utilent pour faire le bon pourcentage avec le kerning
        int x = 0;
        int y = 0;

        // On passe nos 128 en loop
        int i = 0, k = 0;
        while(i < 128)
        {
            // On pogne le kerning
            if(font->kerning[k] == -1)
            {
                k++; x += font->kerning[k++];
                continue;
            }
            if(font->kerning[k] == -2)
            {
                x = 0;
                y += font->height;
                k++;
                continue;
            }

            // On les fou en pourcentage sur 512
            float txf = (float)x / 512.0f;
            float tyf = (float)y / 512.0f;
            float twidthf = (float)(font->kerning[k]) / 512.0f;
            float theightf = (float)font->height / 512.0f;
            float widthf = twidthf / theightf;
            x += font->kerning[k];
            k++;
            font->finalCaracKerning[i + 32] = widthf;

            // Ensuite on crée notre display list pour ce caractère
            glNewList(font->baseFont + i + 32 /* on commence au charatère 32, "Space" */, GL_COMPILE);

            // On dessine un quad de 16x16 avec la lettre affiché dedans
            glBegin(GL_QUADS);
            glTexCoord2f(txf, 1 - tyf);
            glVertex2f(0, 0);

            glTexCoord2f(txf, 1 - (tyf + theightf));
            glVertex2f(0, 1);

            glTexCoord2f(txf + twidthf, 1 - (tyf + theightf));
            glVertex2f(widthf, 1);

            glTexCoord2f(txf + twidthf, 1 - tyf);
            glVertex2f(widthf, 0);
            glEnd();

            // On le déplace de 10 pour prévoir l'espace entre deux lettres
            glTranslatef(widthf, 0, 0);
            glEndList();

            // On incrémente pour le charactère suivant
            i++;
        }

        // On cré les charatères spéciaux (le enter)
        glNewList(font->baseFont + 1, GL_COMPILE);
        glColor3f(0, 0, 1);
        glEndList();
        glNewList(font->baseFont + 2, GL_COMPILE);
        glColor3f(0, 1, 0);
        glEndList();
        glNewList(font->baseFont + 3, GL_COMPILE);
        glColor3f(0, 1, 1);
        glEndList();
        glNewList(font->baseFont + 4, GL_COMPILE);
        glColor3f(1, 0, 0);
        glEndList();
        glNewList(font->baseFont + 5, GL_COMPILE);
        glColor3f(1, 0, 1);
        glEndList();
        glNewList(font->baseFont + 6, GL_COMPILE);
        glColor3f(1, .7f, 0);
        glEndList();
        glNewList(font->baseFont + 7, GL_COMPILE);
        glColor3f(.5f, .5f, .5f);
        glEndList();
        glNewList(font->baseFont + 8, GL_COMPILE);
        glColor3f(1, 1, 1);
        glEndList();
        glNewList(font->baseFont + 9, GL_COMPILE);
        glColor3f(1, 1, 0);
        glEndList();
        glNewList(font->baseFont + 10, GL_COMPILE);
        glPopMatrix();
        glTranslatef(0, 1, 0);
        glPushMatrix();
        glEndList();
    }
    else
    {
        // On passe nos 128 en loop
        int i = 0;
        while(i < 128)
        {
            // Ensuite on crée notre display list pour ce caractère
            glNewList(font->baseFont + i + 32 /* on commence au charatère 32, "Space" */, GL_COMPILE);

            // On dessine un quad de 16x16 avec la lettre affiché dedans
            glBegin(GL_QUADS);
            glTexCoord2f(font->characterProp[i + 32].u1, font->characterProp[i + 32].v1);
            glVertex2f(0, 0);

            glTexCoord2f(font->characterProp[i + 32].u1, font->characterProp[i + 32].v2);
            glVertex2f(0, 1);

            glTexCoord2f(font->characterProp[i + 32].u2, font->characterProp[i + 32].v2);
            glVertex2f(font->characterProp[i + 32].w, 1);

            glTexCoord2f(font->characterProp[i + 32].u2, font->characterProp[i + 32].v1);
            glVertex2f(font->characterProp[i + 32].w, 0);
            glEnd();

            // On le déplace de 10 pour prévoir l'espace entre deux lettres
            glTranslatef(font->characterProp[i + 32].w, 0, 0);
            glEndList();

            // On incrémente pour le charactère suivant
            i++;
        }

        // On cré les charatères spéciaux (le enter)
        glNewList(font->baseFont + 1, GL_COMPILE);
        glColor3f(.25f, .25f, 1);
        glEndList();
        glNewList(font->baseFont + 2, GL_COMPILE);
        glColor3f(.25f, 1, .25f);
        glEndList();
        glNewList(font->baseFont + 3, GL_COMPILE);
        glColor3f(.25f, 1, 1);
        glEndList();
        glNewList(font->baseFont + 4, GL_COMPILE);
        glColor3f(1, .25f, .25f);
        glEndList();
        glNewList(font->baseFont + 5, GL_COMPILE);
        glColor3f(1, .25f, 1);
        glEndList();
        glNewList(font->baseFont + 6, GL_COMPILE);
        glColor3f(1, .7f, 0);
        glEndList();
        glNewList(font->baseFont + 7, GL_COMPILE);
        glColor3f(.5f, .5f, .5f);
        glEndList();
        glNewList(font->baseFont + 8, GL_COMPILE);
        glColor3f(1, 1, 1);
        glEndList();
        glNewList(font->baseFont + 9, GL_COMPILE);
        glColor3f(1, 1, 0);
        glEndList();
        glNewList(font->baseFont + 10, GL_COMPILE);
        glPopMatrix();
        glTranslatef(0, 1, 0);
        glPushMatrix();
        glEndList();
    }
}

static void Font_printText(CFont* font, float size, float x, float y, float z, char *text)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(size, size, size);
    glPushMatrix();
    // On dessine ici la chaine avec CallLists
    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIST_BIT | GL_TEXTURE_BIT);
    glBindTexture(GL_TEXTURE_2D, font->textureID);
    glEnable(GL_TEXTURE_2D);
    glListBase(font->baseFont);
    glCallLists(GLsizei(strlen(text)), GL_UNSIGNED_BYTE, text);
    glPopAttrib();
    glPopMatrix();
    glPopMatrix();
}

//
// Pour binder le font pour dire que c lui qu'on utilise
//
void dkfBindFont(unsigned int ID)
{
    for(int i = 0; i < (int)fonts.size(); i++)
    {
        CFont *font = fonts.at(i);
        if(ID == font->fontID)
        {
            currentBind = font;
            return;
        }
    }
}

//
// Pour créer une police de charactère
//
unsigned int dkfCreateFont(char *filename)
{
    // Bon avant là, on check si il l'a pas loadé 2 fois
    for(int i = 0; i < (int)fonts.size(); i++)
    {
        CFont *font = fonts.at(i);
        if(font->filename == filename)
        {
            font->nbInstance++;
            return font->fontID;
        }
    }

    // Sinon on cré la font
    CFont *font = new CFont();
    Font_Init(font);
    font->fontID = ++currentIDCount;
    if(!Font_create(font, filename))
    {
        delete font;
        return 0;
    }

    // On construit le tout
    Font_reloadIt(font);

    fonts.push_back(font);
    return font->fontID;
}

//
// Pour effacer une fonct
//
void dkfDeleteFont(unsigned int *ID)
{
    for(int i = 0; i < (int)fonts.size(); i++)
    {
        CFont *font = fonts.at(i);
        if(font->fontID == *ID)
        {
            font->nbInstance--;
            if(font->nbInstance == 0)
            {
                fonts.erase(fonts.begin() + i);
                if(currentBind == font) currentBind = 0;
                Font_Fini(font);
                delete font;
            }
            *ID = 0;
            return;
        }
    }
}

//
// retourne le height et width en pixel d'un string utilisant cette police
//
float dkfGetStringHeight(float size, char *text)
{
    // Ici on compte les retour de chariot
    size_t len = strlen(text);
    int nbChariot = 1;
    for(size_t i = 0; i < len; i++)
    {
        if(text[i] == '\n') nbChariot++;
    }
    return (float)nbChariot*size; // Pour le height c'est aussi simple que ça
}

float dkfGetStringWidth(float size, char *text)
{
    if(currentBind)
    {
        float bestScore = 0;
        float currentScore = 0;
        size_t len = strlen(text);
        for(size_t i = 0; i < len; i++)
        {
            currentScore += currentBind->finalCaracKerning[(unsigned char)text[i]];

            if(text[i] == '\n' || i == len - 1)
            {
                if(currentScore > bestScore) bestScore = currentScore;
                currentScore = 0;
                continue;
            }
        }

        if(bestScore > 0)
            return bestScore * size;
        else
            return currentScore * size;
    }
    else
        return 0;
}

void dkfPrint(float size, float x, float y, float z, char *text)
{
    if(currentBind)
    {
        Font_printText(currentBind, size, x, y, z, text);
    }
}

//
// Pour obtenir la premi�re touche press� (utile pour setter les touche dans les options)
//
int dkiGetFirstDown()
{
    for(int i = 0; i < 256 + 8 + 128; i++)
    {
        if(allState[i] == DKI_DOWN)
        {
            return i;
        }
    }

    return DKI_NOKEY;
}

int dkiGetMouseWheelVel()
{
    return 0;// CDki::mouseStateDI.lZ;
}

CVector2i dkiGetMouse()
{
    return mousePos;
}

CVector2i dkiGetMouseVel()
{
    return CVector2i(0, 0);
}

int dkiGetState(int inputID)
{
    int r = DKI_NOTHING;

    if(inputID != DKI_NOKEY)
        r = allState[inputID];

    return r;
}

CVector3f dkiGetJoy()
{
    return CVector3f(0, 0, 0);
}

CVector3f dkiGetJoyR()
{
    return CVector3f(0, 0, 0);
}

CVector3f dkiGetJoyVel()
{
    return CVector3f(0, 0, 0);
}


//
// On update le tout
//
void dkiUpdate(float elapsef)
{
    // SDL_GetMouseState() gives mouse position seemingly based on the last window entered/focused(?)
    // The creation of a new windows at runtime and SDL_CaptureMouse both seems to severely mess up with that, so we retrieve that position globally.
    int mx, my;
    Uint32 mouse_buttons = SDL_GetMouseState(&mx, &my);


    (void)elapsef; // Disable warnings
    int i;

    // On capte le clavier
    int numKeys;
    auto kbState = SDL_GetKeyboardState(&numKeys);

    // On update notre clavier
    for(i = 0; i < 256 && i < numKeys; i++)
    {
        if(kbState[i])
        {
            if(allState[i] == DKI_NOTHING)
            {
                allState[i] = DKI_DOWN;
                lastDown = i;
                downTimer = 0;
            }
            else
            {
                allState[i] = DKI_HOLD;
            }
        }
        else
        {
            if(allState[i] == DKI_DOWN || allState[i] == DKI_HOLD)
            {
                allState[i] = DKI_UP;
            }
            else
            {
                allState[i] = DKI_NOTHING;
            }
        }
    }

    // On update les boutons de notre mouse
    bool mouseDown[3] = {
        (mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0,
        (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0,
        (mouse_buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0
    };
    for(i = 0; i < 3; i++)
    {
        if(mouseDown[i])
        {
            if(allState[DKI_MOUSE_BUTTON1 + i] == DKI_NOTHING)
            {
                allState[DKI_MOUSE_BUTTON1 + i] = DKI_DOWN;
            }
            else
            {
                allState[DKI_MOUSE_BUTTON1 + i] = DKI_HOLD;
            }
        }
        else
        {
            if(allState[DKI_MOUSE_BUTTON1 + i] == DKI_DOWN || allState[DKI_MOUSE_BUTTON1 + i] == DKI_HOLD)
            {
                allState[DKI_MOUSE_BUTTON1 + i] = DKI_UP;
            }
            else
            {
                allState[DKI_MOUSE_BUTTON1 + i] = DKI_NOTHING;
            }
        }
    }

    // La position de la sourie
    int wx, wy;
    SDL_Window* focused_window = SDL_GetKeyboardFocus();
    if(g_window == focused_window)
    {
        SDL_GetWindowPosition(g_window, &wx, &wy);
        SDL_GetGlobalMouseState(&mx, &my);
        mx -= wx;
        my -= wy;
    }

    mousePos[0] = mx;
    mousePos[1] = my;

    //if (CDki::mousePos.x > width - 1) CDki::mousePos.x = width - 1;
    //if (CDki::mousePos.x < 0) CDki::mousePos.x = 0;
    //if (CDki::mousePos.y > height - 1) CDki::mousePos.y = height - 1;
    //if (CDki::mousePos.y < 0) CDki::mousePos.y = 0;

    // On capte le joystick
    //if (CDki::diJoypad)
    //{
    //    hr = CDki::diJoypad->Poll();
    //    if (FAILED(hr))
    //    {
    //        hr = CDki::diJoypad->Acquire();
    //        while (hr == DIERR_INPUTLOST)
    //            hr = CDki::diJoypad->Acquire();
    //    }
    //    CDki::diJoypad->GetDeviceState(sizeof(DIJOYSTATE2), &CDki::EtatJoy);
    //}

    // On update les boutons du joystick
    //for (i = 0; i<128; i++)
    //{
    //    if (INPUTDOWN(CDki::EtatJoy.rgbButtons[i]))
    //    {
    //        if (CDki::allState[DKI_JOY_BUTTON1 + i] == DKI_NOTHING)
    //        {
    //            CDki::allState[DKI_JOY_BUTTON1 + i] = DKI_DOWN;
    //        }
    //        else
    //        {
    //            CDki::allState[DKI_JOY_BUTTON1 + i] = DKI_HOLD;
    //        }
    //    }
    //    else
    //    {
    //        if (CDki::allState[DKI_JOY_BUTTON1 + i] == DKI_DOWN || CDki::allState[DKI_JOY_BUTTON1 + i] == DKI_HOLD)
    //        {
    //            CDki::allState[DKI_JOY_BUTTON1 + i] = DKI_UP;
    //        }
    //        else
    //        {
    //            CDki::allState[DKI_JOY_BUTTON1 + i] = DKI_NOTHING;
    //        }
    //    }
    //}
}

// La class pour tenir une texture, son ID et son filename
class CTexture
{
public:
    // Le string pour le filename de la texture
    CString filename;

    // Son ID opengl
    unsigned int oglID;

    // La date de modification du fichier
    int32_t modifDate;

    // Sa résolution
    CVector2i size;

    // Le nombre de fois quelle a été loadé
    int nbInstance;

    // Le nombre de Byte per pixel de la texture
    int bpp;

public:
    // Constructeur / Destructeur
    CTexture()
    {
        oglID = 0;
        nbInstance = 0;
        modifDate = 0;
    }

    virtual ~CTexture()
    {
        glDeleteTextures(1, &oglID);
    }
};

// La class static pour contenir le tout
class CDkt
{
public:
    // Pour tenir la dernière erreur
    static char *lastErrorString;

    // Le vector de nos textures initialisé
    static std::vector<CTexture*> textures;

    // La texture qu'on check pour le auto updating
    static int checkingUpdate;

public:
    // Pour updater l'erreur
    static void updateLastError(char *error);
};


//
// Les trucs static
//
char *CDkt::lastErrorString = 0;
std::vector<CTexture*> CDkt::textures;
int CDkt::checkingUpdate = 0;

//
// Pour simplement reloader un TGA
//
void reloadTGA(CTexture * texture)
{
    // Les variables utilisé pour tenir l'information loadé du Targa
    unsigned char TGAcompare[12];
    unsigned char header[6];
    unsigned int bytesPerPixel;
    unsigned int imageSize;
    unsigned char temp;
    unsigned char *imageData;
    unsigned int bpp;
    unsigned int width;
    unsigned int height;

    // On ouvre le fichier targa
    FILE *file = fopen(texture->filename.s, "rb");

    // Si ça marche pas, oups, on returne 0 comme texture.
    if(file == NULL)
    {
        // on écris l'erreur dans le log
        CDkt::updateLastError(CString("ERROR > Can not read file : \"%s\"", texture->filename.s).s);
        return;
    }

    // On li le header du fichier (12 premiers byte)
    fread(TGAcompare, 1, sizeof(TGAcompare), file);

    // On li la suite du header
    fread(header, 1, sizeof(header), file);

    // On prend le width et le height du header
    width = header[1] * 256 + header[0];
    height = header[3] * 256 + header[2];
    texture->size.set(width, height);

    // On prend le bbp (24bit ou 32bit)
    bpp = header[4];

    // On le divise par 8 pour avoir 3 ou 4 bytes (RGB ou RGBA)
    bytesPerPixel = bpp / 8;
    texture->bpp = bytesPerPixel;

    // On calcul la grandeur de l'image en byte
    imageSize = width * height * bytesPerPixel;

    // On alou alors autant de bytes qu'il faut pour tenir l'image
    imageData = new unsigned char[imageSize];

    // On li maintenant le gros bloc de données
    fread(imageData, 1, imageSize, file);

    // On défini si c'est RGB ou RGBA
    GLint Level = (bytesPerPixel == 3) ? GL_RGB : GL_RGBA;

    // Ici c'est con, mais faut switcher le rouge avec le bleu
    for(unsigned int i = 0; i < imageSize; i += bytesPerPixel) {

        temp = imageData[i];
        imageData[i] = imageData[i + 2];
        imageData[i + 2] = temp;
    }

    // On ferme maintenant le fichier
    fclose(file);

    // On bind cette texture au context
    glBindTexture(GL_TEXTURE_2D, texture->oglID);

    // On construit les mipmaps
    //gluBuild2DMipmaps(GL_TEXTURE_2D, bytesPerPixel, width, height,
    //                Level, GL_UNSIGNED_BYTE, imageData);
    glTexImage2D(GL_TEXTURE_2D, 0, Level, width, height, 0, Level, GL_UNSIGNED_BYTE, imageData);
    glGenerateMipmap(GL_TEXTURE_2D);

    // On delete notre Data qu'on n'a pus besoin
    delete[] imageData;
}

bool loadTextureFromFile(char * filename, int filter, unsigned int* id) {
    // Notre texture ID de ogl
    unsigned int Texture = 0;

    // On check quelle n'existe pas déjà
    for(int l = 0; l < (int)CDkt::textures.size(); l++)
    {
        CTexture *texture = CDkt::textures.at(l);
        if(texture->filename == filename)
        {
            texture->nbInstance++;
            *id = texture->oglID;
            return texture->oglID;
        }
    }

    FILE *file = fopen(filename, "rb");
    if(!file)
    {
        // on écris l'erreur dans le log
        printf("Cannot open texture file: %s\n", filename);
        CDkt::updateLastError(CString("ERROR > Can not read file : \"%s\"", filename).s);
        return 0;
    }

    int w = 0;
    int h = 0;
    int bytesPerPixel = 0;
    stbi_set_flip_vertically_on_load(true);
    stbi_uc* imageData = stbi_load_from_file(file, &w, &h, &bytesPerPixel, STBI_rgb_alpha);
    bytesPerPixel = STBI_rgb_alpha;
    fclose(file);

    if(!imageData)
    {
        // on écris l'erreur dans le log
        printf("Unsupported image format: %s\n", filename);
        CDkt::updateLastError(CString("ERROR > Unsupported image format : \"%s\"", filename).s);
        return 0;
    }

    // On calcul la grandeur de l'image en byte
    unsigned int imageSize = w * h * bytesPerPixel;

    // On défini si c'est RGB ou RGBA
    GLint type = (bytesPerPixel == 3) ? GL_RGB : GL_RGBA;

    // On génère une texture
    glGenTextures(1, &Texture);

    // On bind cette texture au context
    glBindTexture(GL_TEXTURE_2D, Texture);

    // On set le filter
    switch(filter)
    {
    case DKT_FILTER_NEAREST: // Nearest
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;
    }
    case DKT_FILTER_LINEAR: // Linear
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
    }
    case DKT_FILTER_BILINEAR: // Bilinear
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        break;
    }
    case DKT_FILTER_TRILINEAR: // Trilinear
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        break;
    }
    default: // Nearest default
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        break;
    }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, type, w, h, 0, type, GL_UNSIGNED_BYTE, imageData);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(imageData);
    

    // On se cré notre nouvelle texture
    CTexture *texture = new CTexture;
    texture->filename = filename;
    texture->nbInstance = 1;
    texture->size.set(w, h);
    texture->bpp = bytesPerPixel;
    texture->oglID = Texture;

    struct stat attrib;
    stat(filename, &attrib);
    texture->modifDate = int32_t(attrib.st_mtime);

    CDkt::textures.push_back(texture);

    // On retourne l'index de la texture
    *id = Texture;
    return Texture;
}

unsigned int createTextureTGA(char * filename, int filter) {
    // Notre texture ID de ogl
    unsigned int Texture = 0;

    // On check quelle n'existe pas déjà
    for(int l = 0; l < (int)CDkt::textures.size(); l++)
    {
        CTexture *texture = CDkt::textures.at(l);
        if(texture->filename == filename)
        {
            texture->nbInstance++;
            return texture->oglID;
        }
    }

    // Les variables utilisé pour tenir l'information loadé du Targa
    unsigned char TGAcompare[12];
    unsigned char header[6];
    unsigned int bytesPerPixel;
    unsigned int imageSize;
    unsigned char temp;
    unsigned char *imageData;
    unsigned int bpp;
    unsigned int width;
    unsigned int height;

    // On ouvre le fichier targa
    FILE *file = fopen(filename, "rb");

    // Si ça marche pas, oups, on returne 0 comme texture.
    if(file == NULL)
    {
        // on écris l'erreur dans le log
        printf("Cannot open texture file: %s\n", filename);
        CDkt::updateLastError(CString("ERROR > Can not read file : \"%s\"", filename).s);
        return 0;
    }

    // On li le header du fichier (12 premiers byte)
    fread(TGAcompare, 1, sizeof(TGAcompare), file);

    // On li la suite du header
    fread(header, 1, sizeof(header), file);

    // On prend le width et le height du header
    width = header[1] * 256 + header[0];
    height = header[3] * 256 + header[2];

    // On prend le bbp (24bit ou 32bit)
    bpp = header[4];

    // On le divise par 8 pour avoir 3 ou 4 bytes (RGB ou RGBA)
    bytesPerPixel = bpp / 8;

    // On calcul la grandeur de l'image en byte
    imageSize = width * height * bytesPerPixel;

    // On alou alors autant de bytes qu'il faut pour tenir l'image
    imageData = new unsigned char[imageSize];

    // On li maintenant le gros bloc de données
    fread(imageData, 1, imageSize, file);

    // On défini si c'est RGB ou RGBA
    GLint Level = (bytesPerPixel == 3) ? GL_RGB : GL_RGBA;

    // Ici c'est con, mais faut switcher le rouge avec le bleu
    for(unsigned int i = 0; i < imageSize; i += bytesPerPixel) {

        temp = imageData[i];
        imageData[i] = imageData[i + 2];
        imageData[i + 2] = temp;
    }

    // On ferme maintenant le fichier
    fclose(file);

    // On génère une texture
    glGenTextures(1, &Texture);

    // On bind cette texture au context
    glBindTexture(GL_TEXTURE_2D, Texture);

    // On set le filter
    switch(filter)
    {
    case DKT_FILTER_NEAREST: // Nearest
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;
    }
    case DKT_FILTER_LINEAR: // Linear
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
    }
    case DKT_FILTER_BILINEAR: // Bilinear
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        break;
    }
    case DKT_FILTER_TRILINEAR: // Trilinear
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        break;
    }
    default: // Nearest default
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        break;
    }
    }

    // On construit les mipmaps
    //gluBuild2DMipmaps(GL_TEXTURE_2D, bytesPerPixel, width, height,
    //                Level, GL_UNSIGNED_BYTE, imageData);
    glTexImage2D(GL_TEXTURE_2D, 0, Level, width, height, 0, Level, GL_UNSIGNED_BYTE, imageData);
    glGenerateMipmap(GL_TEXTURE_2D);

    // On delete notre Data qu'on n'a pus besoin
    delete[] imageData;

    // On se cré notre nouvelle texture
    CTexture *texture = new CTexture;
    texture->filename = filename;
    texture->nbInstance = 1;
    texture->size.set(width, height);
    texture->bpp = bytesPerPixel;
    texture->oglID = Texture;

    struct stat attrib;
    stat(filename, &attrib);
    texture->modifDate = int32_t(attrib.st_mtime);

    CDkt::textures.push_back(texture);

    // On retourne l'index de la texture
    return Texture;
}

//
// Pour créer une texture vide
//
unsigned int     dktCreateEmptyTexture(int w, int h, int bpp, int filter)
{
    unsigned int textureID = 0;

    // On se cré notre nouvelle texture
    CTexture *texture = new CTexture;
    texture->filename = "Custom";
    texture->nbInstance = 1;
    CDkt::textures.push_back(texture);
    texture->size.set(w, h);

    // On cré notre array
    int totalSize = w * h*bpp;
    unsigned char *buffer = new unsigned char[w*h*bpp];
    for(int i = 0; i < totalSize; buffer[i++] = 255);

    // On cré une texture ogl et on la bind
    glGenTextures(1, &textureID);
    texture->oglID = textureID;
    glBindTexture(GL_TEXTURE_2D, textureID);

    // On check le level
    GLuint level = 0;
    if(bpp == 1) level = GL_LUMINANCE;
    if(bpp == 3) level = GL_RGB;
    if(bpp == 4) level = GL_RGBA;
    texture->bpp = bpp;

    // On set le filter
    switch(filter)
    {
    case DKT_FILTER_NEAREST: // Nearest
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;
    }
    case DKT_FILTER_LINEAR: // Linear
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
    }
    case DKT_FILTER_BILINEAR: // Bilinear
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        break;
    }
    case DKT_FILTER_TRILINEAR: // Trilinear
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        break;
    }
    default: // Nearest default
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        break;
    }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, level, w, h, 0, level, GL_UNSIGNED_BYTE, buffer);
    glGenerateMipmap(GL_TEXTURE_2D);

    delete[] buffer;

    return textureID;
}



//
// Pour créer une texture à partir d'un buffer
//
void dktCreateTextureFromBuffer(unsigned int *textureID, unsigned char *buffer, int w, int h, int bpp, int filter)
{
    // On delete l'ancienne (elle DOIT exister)
    dktDeleteTexture(textureID);

    // On se cré notre nouvelle texture
    CTexture *texture = new CTexture;
    texture->filename = "Custom";
    texture->nbInstance = 1;
    CDkt::textures.push_back(texture);

    // On cré une texture ogl et on la bind
    glGenTextures(1, textureID);
    texture->oglID = *textureID;
    glBindTexture(GL_TEXTURE_2D, *textureID);

    // On set le filter
    switch(filter)
    {
    case DKT_FILTER_NEAREST: // Nearest
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;
    }
    case DKT_FILTER_LINEAR: // Linear
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
    }
    case DKT_FILTER_BILINEAR: // Bilinear
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        break;
    }
    case DKT_FILTER_TRILINEAR: // Trilinear
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        break;
    }
    default: // Nearest default
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        break;
    }
    }

    // On check le level
    GLuint level = 0;
    if(bpp == 1) level = GL_LUMINANCE;
    if(bpp == 3) level = GL_RGB;
    if(bpp == 4) level = GL_RGBA;

    texture->size.set(w, h);
    texture->bpp = bpp;

    // On construit la texture et ses mipmap
    //gluBuild2DMipmaps(GL_TEXTURE_2D, bpp, w, h,
    //                level, GL_UNSIGNED_BYTE, buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, level, w, h, 0, level, GL_UNSIGNED_BYTE, buffer);
    glGenerateMipmap(GL_TEXTURE_2D);
}



//
// Pour créer une texture à partir d'une image
//
unsigned int dktCreateTextureFromFile(char *mFilename, int filter)
{
    unsigned int id = 0;
    if(loadTextureFromFile(mFilename, filter, &id))
    {
        return id;
    }

    if(strnicmp(&(mFilename[strlen(mFilename) - 3]), "TGA", 3) == 0)
    {
        return createTextureTGA(mFilename, filter);
    }
    else
    {
        // Doit obligatoirement être un TGA
        CDkt::updateLastError("DKT : The image is not supported");
        return 0;
    }
}

//
// On update la dernière erreur
//
void CDkt::updateLastError(char *error)
{
    if(lastErrorString)
    {
        delete[] lastErrorString;
        lastErrorString = 0;
    }
    if(error)
    {
        lastErrorString = new char[strlen(error) + 1];
        strcpy(lastErrorString, error);
    }
}



//
// Pour effacer une texture du stack
//
void dktDeleteTexture(unsigned int *textureID)
{
    for(int i = 0; i < (int)CDkt::textures.size(); i++)
    {
        CTexture *texture = CDkt::textures.at(i);
        if(texture->oglID == *textureID)
        {
            texture->nbInstance--;
            if(texture->nbInstance <= 0)
            {
                CDkt::textures.erase(CDkt::textures.begin() + i);
                delete texture;
            }
            break;
        }
    }

    *textureID = 0;
}

//
// Pour obtenir le data d'une texture ogl
//
unsigned char * dktGetTextureData(unsigned int textureID, int* w, int* h, int* bpp)
{
    for(int i = 0; i < (int)CDkt::textures.size(); i++)
    {
        CTexture *texture = CDkt::textures.at(i);
        if(texture->oglID == textureID)
        {
            unsigned char * data = new unsigned char[texture->size.x()*texture->size.y()*texture->bpp];
            glBindTexture(GL_TEXTURE_2D, textureID);
            glGetTexImage(GL_TEXTURE_2D, 0, (texture->bpp == 3) ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data);
            *w = texture->size.x();
            *h = texture->size.y();
            *bpp = texture->bpp;
            return data;
        }
    }

    return nullptr;
}

//
// Pour forcer l'application à fermer
//
void dkwForceQuit()
{
    done = true;
}

//
// Obtenir la dernière erreur
//
char* dkwGetLastError()
{
    return (char*)last_error.c_str();
}

//
// Pour retourner la position de la sourie sur l'écran
//
CVector2i dkwGetCursorPos()
{
    return dkiGetMouse();
}

//
// On retourne la résolution de la fenêtre
//
CVector2i dkwGetResolution()
{
    SDL_Window* pWindow = SDL_GL_GetCurrentWindow();
    int w, h;
    SDL_GetWindowSize(pWindow, &w, &h);

    return { w, h };
}

// On clip la mouse au window rect
void dkwClipMouse(bool abEnabled)
{
}

//
// On effectu le loop principal de l'application
//
int dkwMainLoop()
{
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    SDL_Event event;

    int(*sdlEventCall)(SDL_Event * event) = SDL_WaitEvent;

    while(SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if(event.type == SDL_QUIT)
            done = true;
        if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(g_window))
            done = true;
        if(event.type == SDL_TEXTINPUT)
        {
            mainLoopObject->textWrite(event.text.text[0]);
        }
        //if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        //{
        //    width = event.window.data1;
        //    height = event.window.data2;
        //}
    }


    bool show_demo_window = false;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(g_window);
    ImGui::NewFrame();
    if(show_demo_window)
    {
        ImGui::ShowDemoWindow(&show_demo_window);
    }
    ImGui::Render();

    mainLoopObject->paint();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Swap buffers if valid context is found
    if(g_gl_context)
    {
        SDL_ShowCursor(false);
        SDL_GL_SwapWindow(g_window);
    }

    return done ? 0 : 1;
}

struct dkGfxContext
{
    int temp;
};

bool dksInit(int mixrate, int maxsoftwarechannels);

void dkoShutDown();
void dkpShutDown();
void dksShutDown();

void dkwSwap()
{
    SDL_GL_SwapWindow(g_window);
}

dkGfxContext* dkGfxInit(dkContext* ctx, dkGfxConfig config)
{
    dkGfxContext* gtx = new dkGfxContext;

    mainLoopObject = config.mMainLoopObject;

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        last_error = std::string("Error: ") + SDL_GetError() + "\n";
        return 0;
    }

    // Decide GL+GLSL versions
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if(config.fullScreen) flags |= SDL_WINDOW_FULLSCREEN;
    g_window = SDL_CreateWindow(config.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, config.width, config.height, flags);
    g_gl_context = SDL_GL_CreateContext(g_window);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    SDL_GL_MakeCurrent(g_window, g_gl_context);

    if(!gladLoadGLLoader(SDL_GL_GetProcAddress))
    {
        last_error = "Failed to initialize BrebisGL\n";
        return nullptr;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    ImGui_ImplSDL2_InitForOpenGL(g_window, g_gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui::StyleColorsDark();

    done = false;

    memset(allState, 0, sizeof(allState));

    dkoInit();
    dkpInit();
    return gtx;
}

void dkGfxFini(dkGfxContext* gtx)
{
    dksShutDown();
    dkpShutDown();
    dkoShutDown();

    for(int i = 0; i < (int)fonts.size(); i++)
    {
        CFont *font = fonts.at(i);
        if(font) delete font;
    }
    fonts.clear();
    currentBind = 0;
    currentIDCount = 0;

    for(int i = 0; i < (int)CDkt::textures.size(); i++)
    {
        CTexture *texture = CDkt::textures.at(i);
        if(texture) delete texture;
    }
    CDkt::textures.clear();
    CDkt::updateLastError(0);

    delete gtx;
}

#define TORAD 0.01745329251994329576923690768489f


static void createPerspectiveFieldOfView(float fov, float aspectRatio, float nearPlane, float farPlane, float out[4][4])
{
    float CosFov = std::cos(0.5f * fov * TORAD);
    float SinFov = std::sin(0.5f * fov * TORAD);

    float Height = CosFov / SinFov;
    float Width = Height / aspectRatio;
    float fRange = farPlane / (nearPlane - farPlane);

    out[0][0] = Width;
    out[0][1] = 0.0f;
    out[0][2] = 0.0f;
    out[0][3] = 0.0f;

    out[1][0] = 0.0f;
    out[1][1] = Height;
    out[1][2] = 0.0f;
    out[1][3] = 0.0f;

    out[2][0] = 0.0f;
    out[2][1] = 0.0f;
    out[2][2] = fRange;
    out[2][3] = -1.0f;

    out[3][0] = 0.0f;
    out[3][1] = 0.0f;
    out[3][2] = fRange * nearPlane;
    out[3][3] = 0.0f;
}

void dkglEnableVsync(bool vsync)
{
    SDL_GL_SetSwapInterval(vsync ? 1 : 0);
}

//
// Pour revenir en vue 3D
//
void dkglPopOrtho()
{
    // On pop nos matrice
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // On pop nos attribs
    glPopAttrib();
}



//
// Pour setter la vue en 2D
//
void dkglPushOrtho(float mWidth, float mHeight)
{
    // On push les attribs pour certaines modifications
    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
    //  glCullFace(GL_BACK);

        // En mode 2D on ne veux pas de z-buffer
    glDisable(GL_DEPTH_TEST);

    // On push nos matrice et on set la matrice ortho
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, mWidth, mHeight, 0, -9999, 9999);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}



//
// Pour setter la fonction de blending
//
void dkglSetBlendingFunc(int blending)
{
    switch(blending)
    {
    case DKGL_BLENDING_ADD_SATURATE:
        glBlendFunc(GL_ONE, GL_ONE);
        break;
    case DKGL_BLENDING_ADD:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
    case DKGL_BLENDING_MULTIPLY:
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
        break;
    case DKGL_BLENDING_ALPHA:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }
}



//
// Pour rapidement créer une lumière dans votre scene (très basic)
//
void dkglSetPointLight(int ID, float x, float y, float z, float r, float g, float b)
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0 + ID);
    float pos[] = { x,y,z,1 };
    float amb[] = { r / 6,g / 6,b / 6,1 };
    float diff[] = { r,g,b,1 };
    float spec[] = { r,g,b,1 };

    glLightfv(GL_LIGHT0 + ID, GL_POSITION, pos);
    glLightfv(GL_LIGHT0 + ID, GL_AMBIENT, amb);
    glLightfv(GL_LIGHT0 + ID, GL_DIFFUSE, diff);
    glLightfv(GL_LIGHT0 + ID, GL_SPECULAR, spec);
}



//
// On set la vue de la perspective là
//
void dkglSetProjection(float mFieldOfView, float mNear, float mFar, float mWidth, float mHeight)
{
    // On met la matrice de projection pour ce créer une vue perspective
    glMatrixMode(GL_PROJECTION);

    // On remet cette matrice à identity
    glLoadIdentity();

    // On ajuste la matrice de projection
    float proj[4][4];
    createPerspectiveFieldOfView(mFieldOfView, mWidth / mHeight, mNear, mFar, proj);
    glMultMatrixf(&proj[0][0]);

    // On remet cette de model view (qui est celle de la position et l'orientation)
    glMatrixMode(GL_MODELVIEW);

    // La model view à identity
    glLoadIdentity();
}

int glhInvertMatrixf2(float *m, float *out);

int glhProjectf(float objx, float objy, float objz, float *modelview, float *projection, int *viewport, float *windowCoordinate)
{
    // Transformation vectors
    float fTempo[8];
    // Modelview transform
    fTempo[0] = modelview[0] * objx + modelview[4] * objy + modelview[8] * objz + modelview[12]; // w is always 1
    fTempo[1] = modelview[1] * objx + modelview[5] * objy + modelview[9] * objz + modelview[13];
    fTempo[2] = modelview[2] * objx + modelview[6] * objy + modelview[10] * objz + modelview[14];
    fTempo[3] = modelview[3] * objx + modelview[7] * objy + modelview[11] * objz + modelview[15];
    // Projection transform, the final row of projection matrix is always [0 0 -1 0]
    // so we optimize for that.
    fTempo[4] = projection[0] * fTempo[0] + projection[4] * fTempo[1] + projection[8] * fTempo[2] + projection[12] * fTempo[3];
    fTempo[5] = projection[1] * fTempo[0] + projection[5] * fTempo[1] + projection[9] * fTempo[2] + projection[13] * fTempo[3];
    fTempo[6] = projection[2] * fTempo[0] + projection[6] * fTempo[1] + projection[10] * fTempo[2] + projection[14] * fTempo[3];
    fTempo[7] = -fTempo[2];
    // The result normalizes between -1 and 1
    if(fTempo[7] == 0.0f) // The w value
        return 0;
    fTempo[7] = 1.0f / fTempo[7];
    // Perspective division
    fTempo[4] *= fTempo[7];
    fTempo[5] *= fTempo[7];
    fTempo[6] *= fTempo[7];
    // Window coordinates
    // Map x, y to range 0-1
    windowCoordinate[0] = (fTempo[4] * 0.5f + 0.5f)*viewport[2] + viewport[0];
    windowCoordinate[1] = (fTempo[5] * 0.5f + 0.5f)*viewport[3] + viewport[1];
    // This is only correct when glDepthRange(0.0, 1.0)
    windowCoordinate[2] = (1.0f + fTempo[6])*0.5f;  // Between 0 and 1
    return 1;
}

void MultiplyMatrices4by4OpenGL_FLOAT(float *result, float *matrix1, float *matrix2);
void MultiplyMatrixByVector4by4OpenGL_FLOAT(float *resultvector, const float *matrix, const float *pvector);

int glhUnProjectf(float winx, float winy, float winz, float *modelview, float *projection, int *viewport, float *objectCoordinate)
{
    // Transformation matrices
    float m[16], A[16];
    float in[4], out[4];
    // Calculation for inverting a matrix, compute projection x modelview
    // and store in A[16]
    MultiplyMatrices4by4OpenGL_FLOAT(A, projection, modelview);
    // Now compute the inverse of matrix A
    if(glhInvertMatrixf2(A, m) == 0)
        return 0;
    // Transformation of normalized coordinates between -1 and 1
    in[0] = (winx - (float)viewport[0]) / (float)viewport[2] * 2.0f - 1.0f;
    in[1] = (winy - (float)viewport[1]) / (float)viewport[3] * 2.0f - 1.0f;
    in[2] = 2.0f*winz - 1.0f;
    in[3] = 1.0f;
    // Objects coordinates
    MultiplyMatrixByVector4by4OpenGL_FLOAT(out, m, in);
    if(out[3] == 0.0f)
        return 0;
    out[3] = 1.0f / out[3];
    objectCoordinate[0] = out[0] * out[3];
    objectCoordinate[1] = out[1] * out[3];
    objectCoordinate[2] = out[2] * out[3];
    return 1;
}

void MultiplyMatrices4by4OpenGL_FLOAT(float *result, float *matrix1, float *matrix2)
{
    result[0] = matrix1[0] * matrix2[0] +
        matrix1[4] * matrix2[1] +
        matrix1[8] * matrix2[2] +
        matrix1[12] * matrix2[3];
    result[4] = matrix1[0] * matrix2[4] +
        matrix1[4] * matrix2[5] +
        matrix1[8] * matrix2[6] +
        matrix1[12] * matrix2[7];
    result[8] = matrix1[0] * matrix2[8] +
        matrix1[4] * matrix2[9] +
        matrix1[8] * matrix2[10] +
        matrix1[12] * matrix2[11];
    result[12] = matrix1[0] * matrix2[12] +
        matrix1[4] * matrix2[13] +
        matrix1[8] * matrix2[14] +
        matrix1[12] * matrix2[15];
    result[1] = matrix1[1] * matrix2[0] +
        matrix1[5] * matrix2[1] +
        matrix1[9] * matrix2[2] +
        matrix1[13] * matrix2[3];
    result[5] = matrix1[1] * matrix2[4] +
        matrix1[5] * matrix2[5] +
        matrix1[9] * matrix2[6] +
        matrix1[13] * matrix2[7];
    result[9] = matrix1[1] * matrix2[8] +
        matrix1[5] * matrix2[9] +
        matrix1[9] * matrix2[10] +
        matrix1[13] * matrix2[11];
    result[13] = matrix1[1] * matrix2[12] +
        matrix1[5] * matrix2[13] +
        matrix1[9] * matrix2[14] +
        matrix1[13] * matrix2[15];
    result[2] = matrix1[2] * matrix2[0] +
        matrix1[6] * matrix2[1] +
        matrix1[10] * matrix2[2] +
        matrix1[14] * matrix2[3];
    result[6] = matrix1[2] * matrix2[4] +
        matrix1[6] * matrix2[5] +
        matrix1[10] * matrix2[6] +
        matrix1[14] * matrix2[7];
    result[10] = matrix1[2] * matrix2[8] +
        matrix1[6] * matrix2[9] +
        matrix1[10] * matrix2[10] +
        matrix1[14] * matrix2[11];
    result[14] = matrix1[2] * matrix2[12] +
        matrix1[6] * matrix2[13] +
        matrix1[10] * matrix2[14] +
        matrix1[14] * matrix2[15];
    result[3] = matrix1[3] * matrix2[0] +
        matrix1[7] * matrix2[1] +
        matrix1[11] * matrix2[2] +
        matrix1[15] * matrix2[3];
    result[7] = matrix1[3] * matrix2[4] +
        matrix1[7] * matrix2[5] +
        matrix1[11] * matrix2[6] +
        matrix1[15] * matrix2[7];
    result[11] = matrix1[3] * matrix2[8] +
        matrix1[7] * matrix2[9] +
        matrix1[11] * matrix2[10] +
        matrix1[15] * matrix2[11];
    result[15] = matrix1[3] * matrix2[12] +
        matrix1[7] * matrix2[13] +
        matrix1[11] * matrix2[14] +
        matrix1[15] * matrix2[15];
}

void MultiplyMatrixByVector4by4OpenGL_FLOAT(float *resultvector, const float *matrix, const float *pvector)
{
    resultvector[0] = matrix[0] * pvector[0] + matrix[4] * pvector[1] + matrix[8] * pvector[2] + matrix[12] * pvector[3];
    resultvector[1] = matrix[1] * pvector[0] + matrix[5] * pvector[1] + matrix[9] * pvector[2] + matrix[13] * pvector[3];
    resultvector[2] = matrix[2] * pvector[0] + matrix[6] * pvector[1] + matrix[10] * pvector[2] + matrix[14] * pvector[3];
    resultvector[3] = matrix[3] * pvector[0] + matrix[7] * pvector[1] + matrix[11] * pvector[2] + matrix[15] * pvector[3];
}

#define SWAP_ROWS_DOUBLE(a, b) { double *_tmp = a; (a) = (b); (b) = _tmp; }
#define SWAP_ROWS_FLOAT(a, b) { float *_tmp = a; (a) = (b); (b) = _tmp; }
#define MAT(m, r, c) (m)[(c) * 4 + (r)]

// This code comes directly from GLU except that it is for float
int glhInvertMatrixf2(float *m, float *out)
{
    float wtmp[4][8];
    float m0, m1, m2, m3, s;
    float *r0, *r1, *r2, *r3;
    r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];
    r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
        r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
        r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
        r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
        r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
        r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
        r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
        r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
        r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
        r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
        r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
        r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;
    /* choose pivot - or die */
    if(fabsf(r3[0]) > fabsf(r2[0]))
        SWAP_ROWS_FLOAT(r3, r2);
    if(fabsf(r2[0]) > fabsf(r1[0]))
        SWAP_ROWS_FLOAT(r2, r1);
    if(fabsf(r1[0]) > fabsf(r0[0]))
        SWAP_ROWS_FLOAT(r1, r0);
    if(0.0 == r0[0])
        return 0;
    /* eliminate first variable */
    m1 = r1[0] / r0[0];
    m2 = r2[0] / r0[0];
    m3 = r3[0] / r0[0];
    s = r0[1];
    r1[1] -= m1 * s;
    r2[1] -= m2 * s;
    r3[1] -= m3 * s;
    s = r0[2];
    r1[2] -= m1 * s;
    r2[2] -= m2 * s;
    r3[2] -= m3 * s;
    s = r0[3];
    r1[3] -= m1 * s;
    r2[3] -= m2 * s;
    r3[3] -= m3 * s;
    s = r0[4];
    if(s != 0.0) {
        r1[4] -= m1 * s;
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r0[5];
    if(s != 0.0) {
        r1[5] -= m1 * s;
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r0[6];
    if(s != 0.0) {
        r1[6] -= m1 * s;
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r0[7];
    if(s != 0.0) {
        r1[7] -= m1 * s;
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }
    /* choose pivot - or die */
    if(fabsf(r3[1]) > fabsf(r2[1]))
        SWAP_ROWS_FLOAT(r3, r2);
    if(fabsf(r2[1]) > fabsf(r1[1]))
        SWAP_ROWS_FLOAT(r2, r1);
    if(0.0 == r1[1])
        return 0;
    /* eliminate second variable */
    m2 = r2[1] / r1[1];
    m3 = r3[1] / r1[1];
    r2[2] -= m2 * r1[2];
    r3[2] -= m3 * r1[2];
    r2[3] -= m2 * r1[3];
    r3[3] -= m3 * r1[3];
    s = r1[4];
    if(0.0 != s) {
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r1[5];
    if(0.0 != s) {
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r1[6];
    if(0.0 != s) {
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r1[7];
    if(0.0 != s) {
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }
    /* choose pivot - or die */
    if(fabsf(r3[2]) > fabsf(r2[2]))
        SWAP_ROWS_FLOAT(r3, r2);
    if(0.0f == r2[2])
        return 0;
    /* eliminate third variable */
    m3 = r3[2] / r2[2];
    r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
        r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];
    /* last check */
    if(0.0f == r3[3])
        return 0;
    s = 1.0f / r3[3];       /* now back substitute row 3 */
    r3[4] *= s;
    r3[5] *= s;
    r3[6] *= s;
    r3[7] *= s;
    m2 = r2[3];         /* now back substitute row 2 */
    s = 1.0f / r2[2];
    r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
        r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
    m1 = r1[3];
    r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
        r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
    m0 = r0[3];
    r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
        r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;
    m1 = r1[2];         /* now back substitute row 1 */
    s = 1.0f / r1[1];
    r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
        r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
    m0 = r0[2];
    r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
        r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;
    m0 = r0[1];         /* now back substitute row 0 */
    s = 1.0f / r0[0];
    r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
        r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);
    MAT(out, 0, 0) = r0[4];
    MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
    MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
    MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
    MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
    MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
    MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
    MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
    MAT(out, 3, 3) = r3[7];
    return 1;
}

//
// Pour projeter la mouse ou un point 2D quelconque
//
CVector3f dkglUnProject(CVector2i & pos2D, float zRange)
{
    float v[3];
    CVector3f pos((float)pos2D[0], (float)pos2D[1], zRange);
    GLfloat modelMatrix[16];
    GLfloat projMatrix[16];
    GLint    viewport[4];

    glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);
    glGetFloatv(GL_PROJECTION_MATRIX, projMatrix);
    glGetIntegerv(GL_VIEWPORT, viewport);

    glhUnProjectf(
        pos[0],
        pos[1],
        pos[2],
        modelMatrix,
        projMatrix,
        viewport,
        v);

    return CVector3f(v[0], v[1], v[2]);
}


CVector3f dkglProject(CVector3f & pos3D)
{
    float v[3];
    GLfloat modelMatrix[16];
    GLfloat projMatrix[16];
    GLint    viewport[4];

    glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);
    glGetFloatv(GL_PROJECTION_MATRIX, projMatrix);
    glGetIntegerv(GL_VIEWPORT, viewport);

    glhProjectf(
        pos3D[0],
        pos3D[1],
        pos3D[2],
        modelMatrix,
        projMatrix,
        viewport,
        v);
    return CVector3f(v[0], v[1], v[2]);
}

static void lookAt(
    float eyex, float eyey, float eyez,
    float orgx, float orgy, float orgz,
    float  upx, float  upy, float  upz)
{
    //Calculate eye direction vector
    float vpnx = orgx - eyex;
    float vpny = orgy - eyey;
    float vpnz = orgz - eyez;

    //Normalize it
    float len = sqrt(vpnx * vpnx + vpny * vpny + vpnz * vpnz);
    vpnx /= len;
    vpny /= len;
    vpnz /= len;

    //Calculate right vector
    float rvx = vpny * upz - vpnz * upy;
    float rvy = vpnz * upx - vpnx * upz;
    float rvz = vpnx * upy - vpny * upx;

    //Calculate new up vector
    float nux = rvy * vpnz - rvz * vpny;
    float nuy = rvz * vpnx - rvx * vpnz;
    float nuz = rvx * vpny - rvy * vpnx;

    //Put it all in a pretty Matrix
    float mat[16] = {
        rvx, nux, -vpnx, 0,
        rvy, nuy, -vpny, 0,
        rvz, nuz, -vpnz, 0,
        0, 0, 0, 1
    };

    //Apply the matrix and translate to eyepoint
    glMultMatrixf(mat);
    glTranslatef(-eyex, -eyey, -eyez);
}

void dkglLookAt(
    GLdouble eyex,
    GLdouble eyey,
    GLdouble eyez,
    GLdouble centerx,
    GLdouble centery,
    GLdouble centerz,
    GLdouble upx,
    GLdouble upy,
    GLdouble upz)
{
    lookAt((float)eyex, (float)eyey, (float)eyez, (float)centerx, (float)centery, (float)centerz, (float)upx, (float)upy, (float)upz);
}

#define PI2 6.283185307179586476925286766559f

// Very expensive call
void dkglDrawSphere(GLdouble radius, GLint slices, GLint stacks, GLenum topology)
{
    glBegin(topology);
    for(int j = 0; j < stacks; ++j)
    {
        float angleX0 = -(float)j / (float)stacks * PI + PI / 2;
        float angleX1 = -(float)(j + 1) / (float)stacks * PI + PI / 2;
        for(int i = 0; i < slices; ++i)
        {
            float angleZ0 = (float)i / (float)slices * PI2;
            float angleZ1 = (float)(i + 1) / (float)slices * PI2;

            glTexCoord2f((float)(i) / (float)slices, (float)(j) / (float)stacks);
            glNormal3f(
                std::cosf(angleZ0) * std::cosf(angleX0),
                std::sinf(angleZ0) * std::cosf(angleX0),
                std::sinf(angleX0)
            );
            glVertex3f(
                std::cosf(angleZ0) * std::cosf(angleX0) * (float)radius,
                std::sinf(angleZ0) * std::cosf(angleX0) * (float)radius,
                std::sinf(angleX0) * (float)radius);

            glTexCoord2f((float)(i) / (float)slices, (float)(j + 1) / (float)stacks);
            glNormal3f(
                std::cosf(angleZ0) * std::cosf(angleX1),
                std::sinf(angleZ0) * std::cosf(angleX1),
                std::sinf(angleX1)
            );
            glVertex3f(
                std::cosf(angleZ0) * std::cosf(angleX1) * (float)radius,
                std::sinf(angleZ0) * std::cosf(angleX1) * (float)radius,
                std::sinf(angleX1) * (float)radius);

            glTexCoord2f((float)(i + 1) / (float)slices, (float)(j + 1) / (float)stacks);
            glNormal3f(
                std::cosf(angleZ1) * std::cosf(angleX1),
                std::sinf(angleZ1) * std::cosf(angleX1),
                std::sinf(angleX1)
            );
            glVertex3f(
                std::cosf(angleZ1) * std::cosf(angleX1) * (float)radius,
                std::sinf(angleZ1) * std::cosf(angleX1) * (float)radius,
                std::sinf(angleX1) * (float)radius);

            glTexCoord2f((float)(i) / (float)slices, (float)(j) / (float)stacks);
            glNormal3f(
                std::cosf(angleZ0) * std::cosf(angleX0),
                std::sinf(angleZ0) * std::cosf(angleX0),
                std::sinf(angleX0)
            );
            glVertex3f(
                std::cosf(angleZ0) * std::cosf(angleX0) * (float)radius,
                std::sinf(angleZ0) * std::cosf(angleX0) * (float)radius,
                std::sinf(angleX0) * (float)radius);

            glTexCoord2f((float)(i + 1) / (float)slices, (float)(j + 1) / (float)stacks);
            glNormal3f(
                std::cosf(angleZ1) * std::cosf(angleX1),
                std::sinf(angleZ1) * std::cosf(angleX1),
                std::sinf(angleX1)
            );
            glVertex3f(
                std::cosf(angleZ1) * std::cosf(angleX1) * (float)radius,
                std::sinf(angleZ1) * std::cosf(angleX1) * (float)radius,
                std::sinf(angleX1) * (float)radius);

            glTexCoord2f((float)(i + 1) / (float)slices, (float)(j) / (float)stacks);
            glNormal3f(
                std::cosf(angleZ1) * std::cosf(angleX0),
                std::sinf(angleZ1) * std::cosf(angleX0),
                std::sinf(angleX0)
            );
            glVertex3f(
                std::cosf(angleZ1) * std::cosf(angleX0) * (float)radius,
                std::sinf(angleZ1) * std::cosf(angleX0) * (float)radius,
                std::sinf(angleX0) * (float)radius);
        }
    }
    glEnd();
}
