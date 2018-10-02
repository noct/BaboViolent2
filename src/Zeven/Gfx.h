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
#ifndef ZEVEN_GFX_H
#define ZEVEN_GFX_H

#if defined(DEDICATED_SERVER)
#error Not allowed
#endif

#include <Zeven/Core.h>

struct CMainLoopInterface
{
    virtual void paint() = 0;
    virtual void textWrite(unsigned int caracter) = 0;
};

struct dkGfxContext;
struct dkGfxConfig
{
    int width;
    int height;
    char *title;
    CMainLoopInterface *mMainLoopObject;
    bool fullScreen;
    int refreshRate;
    int mixrate;
    int maxsoftwarechannels;
};

dkGfxContext* dkGfxInit(dkContext* ctx, dkGfxConfig config);
void          dkGfxFini(dkGfxContext* gtx);


/// \brief Module de gestion des polices de caractères
///
/// \file dkf.h
/// Ce module prend en charge la gestion des polices de caractères créées à partir d'ume image TGA.
/// Ceci comprend :
///     - une fonction de chargement de police de caractères
///     - une fonction de destruction de polices de caractères
///     - des fonctions retournant diverses informations sur une chaine de caractère selon la police choisie
///     - une fonction de sélection de la police à utiliser
///     - une fonction de destruction de toutes les polices de caractères présentement chargées
///     - une fonction permettant de dessiner une chaine de caractère sur un QUAD (polygone a 4 coté) en 3D d'une certaine taille et avec une certaine couleur
///
/// Les couleurs peuvent être spécifiées à même la chaine de caractères. Voici la liste des couleurs disponibles et leur caractère correspondant (la couleur sera utilisé pour les caractère suivant) :
///     - \x1 = texte bleu
///     - \x2 = texte vert
///     - \x3 = texte cyan
///     - \x4 = texte rouge
///     - \x5 = texte magenta
///     - \x6 = texte brun
///     - \x7 = texte gris clair
///     - \x8 = texte gris foncé
///     - \x9 = texte jaune
void dkfBindFont(unsigned int ID);



/// \brief crée une police de caractères
///
/// Cette fonction crée une police de caractères à partir d'une image TGA valide. Pour être considérer valide, une image TGA doit respecter certaine règle:
///     - doit être de dimension 512x512 pixels
///     - doit représenter un caractère dans une zone de 32x64 pixels maximum (afin de disposer d'assez d'aire pour 128 caractères)
///     - un masque alpha doit délimiter verticalement chaque caractère avec au moins une ligne de 64 pixels de couleur (0,0,0) (ceci veut aussi dire que le dessin d'un caractère ne doit pas être divisé en deux par une ligne verticale de 64 pixels de noir)
///     - les caractères représentées doivent être ceux entre le code ASCII 32 et 160 exclusivement et dans l'ordre
///     - le premier caractère (33) doit être représenté à partir du coin supérieur gauche de l'image, les caractères suivants étant successivement représentés vers la droite et ensuite de nouveau à partir de gauche lorsqu'on atteint la limite de l'image
///
/// \param filename chemin menant au fichier image TGA depuis l'endroit où se situe le fichier EXE du programme à partir de laquelle une police de caractère sera créé
/// \return identifiant unique référant la nouvelle police de caractère
unsigned int    dkfCreateFont(char *filename);
void            dkfDeleteFont(unsigned int *ID);
float           dkfGetStringHeight(float size, char *text);
float           dkfGetStringWidth(float size, char *text);
void            dkfPrint(float size, float x, float y, float z, char *text);

/// représente la paire de coefficients (source, destination) : (GL_ONE, GL_ONE)
const int DKGL_BLENDING_ADD_SATURATE = 0;
/// représente la paire de coefficients (source, destination) : (GL_SRC_ALPHA, GL_ONE)
const int DKGL_BLENDING_ADD = 3;
/// représente la paire de coefficients (source, destination) : (GL_DST_COLOR, GL_ZERO)
const int DKGL_BLENDING_MULTIPLY = 1;
/// représente la paire de coefficients (source, destination) : (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
const int DKGL_BLENDING_ALPHA = 2;

void            dkglPopOrtho();
void            dkglPushOrtho(float mWidth, float mHeight);
void            dkglSetBlendingFunc(int blending);
void            dkglEnableVsync(bool enabled = true);
void            dkglSetPointLight(int ID, float x, float y, float z, float r, float g, float b);
void            dkglSetProjection(float mFieldOfView, float mNear, float mFar, float mWidth, float mHeight);
CVector3f       dkglUnProject(CVector2i & pos2D, float zRange);
CVector3f       dkglProject(CVector3f & pos3D);

void dkglLookAt(
    double eyex,
    double eyey,
    double eyez,
    double centerx,
    double centery,
    double centerz,
    double upx,
    double upy,
    double upz);

void dkglDrawSphere(double radius, int slices, int stacks, unsigned int topology);


// Les �tats des touches
/// \name �tats des touches et boutons possibles
/// Dans le cas normal, la s�quence d'�tats pour une touche qui est appuy� pendant un certain temps sera :
/// ..., DKI_NOTHING, DKI_NOTHING,(l'�v�nement d'enfoncement de la touche se produit ici) DKI_DOWN, DKI_HOLD, DKI_HOLD, ..., DKI_HOLD, DKI_HOLD, (l'�v�nement de relachement de la touche se produit ici)DKI_UP, DKI_NOTHING, DKI_NOTHING,...
//@{
/// utilis�e pour d�signer qu'aucune touche n'a encore �t� pes� (voir dkiGetFirstDown())
const int DKI_NOKEY = -1;
/// utilis�e pour d�signer qu'une touche n'est pas appuy�e
const int DKI_NOTHING = 0;
/// utilis�e pour d�signer qu'une touche vient d'�tre enfonc�e. Cet �tat est pr�sent seulement une fois pour toute la dur�e de l'enfoncement d'une touche.
const int DKI_DOWN = 1;
/// utilis�e pour d�signer qu'une touche est maintenue enfonc�e. Cet �tat est pr�sent tant que la touche reste enfonc�.
const int DKI_HOLD = 2;
/// utilis�e pour d�signer qu'une touche vient d'�tre relach�e. Cet �tat est pr�sent seulement une fois d�s que la touche a �t� relach�e.
const int DKI_UP = 3;
/// non utilis�
const int DKI_CLIC = 3;
/// non utilis�
const int DKI_DBL_CLIC = 4;
//@}

enum DkiKey : int
{
    None = 0,
    KeyEscape = 41,
    Key1 = 30,
    Key2 = 31,
    Key3 = 32,
    Key4 = 33,
    Key5 = 34,
    Key6 = 35,
    Key7 = 36,
    Key8 = 37,
    Key9 = 38,
    Key0 = 39,
    KeyMinus = 45, /* - on main keyboard */
    KeyEquals = 46,
    KeyBackspace = 42, /* backspace */
    KeyTab = 43,
    KeyQ = 20,
    KeyW = 26,
    KeyE = 8,
    KeyR = 21,
    KeyT = 23,
    KeyY = 28,
    KeyU = 24,
    KeyI = 12,
    KeyO = 18,
    KeyP = 19,
    KeyLeftBracket = 47,
    KeyRightBracket = 48,
    KeyEnter = 40, /* Enter on main keyboard */
    KeyLeftControl = 224,
    KeyA = 4,
    KeyS = 22,
    KeyD = 7,
    KeyF = 9,
    KeyG = 10,
    KeyH = 11,
    KeyJ = 13,
    KeyK = 14,
    KeyL = 15,
    KeySemiColon = 51,
    KeyApostrophe = 52,
    KeyGrave = 53,    /* accent grave */
    KeyLeftShift = 225,
    KeyBackslash = 49,
    KeyZ = 29,
    KeyX = 27,
    KeyC = 6,
    KeyV = 25,
    KeyB = 5,
    KeyN = 17,
    KeyM = 16,
    KeyComma = 54,
    KeyPeriod = 55, /* . on main keyboard */
    KeySlash = 56, /* / on main keyboard */
    KeyRightShift = 229,
    KeyMultiply = 85, /* * on numeric keypad */
    KeyLeftAlt = 226, /* left Alt */
    KeySpaceBar = 44,
    KeyCapsLock = 57,
    KeyF1 = 58,
    KeyF2 = 59,
    KeyF3 = 60,
    KeyF4 = 61,
    KeyF5 = 62,
    KeyF6 = 63,
    KeyF7 = 64,
    KeyF8 = 65,
    KeyF9 = 66,
    KeyF10 = 67,
    KeyNumLock = 83,
    KeyScrollLock = 71, /* Scroll Lock */
    KeyNumPad7 = 95,
    KeyNumPad8 = 96,
    KeyNumPad9 = 97,
    KeyNumPadMinus = 86, /* - on numeric keypad */
    KeyNumPad4 = 92,
    KeyNumPad5 = 93,
    KeyNumPad6 = 94,
    KeyNumPadAdd = 87, /* + on numeric keypad */
    KeyNumPad1 = 89,
    KeyNumPad2 = 90,
    KeyNumPad3 = 91,
    KeyNumPad0 = 98,
    KeyNumPadPeriod = 99, /* . on numeric keypad */
    KeyF11 = 68,
    KeyF12 = 69,
    KeyF13 = 104, /* (NEC PC98) */
    KeyF14 = 105, /* (NEC PC98) */
    KeyF15 = 106, /* (NEC PC98) */
    KeyNumPadEquals = 103, /* = on numeric keypad (NEC PC98) */
    KeyPreviousTrack = 259, /* Previous Track (OINPUT_CIRCUMFLEX on Japanese keyboard) */
    KeyAt = 206, /* (NEC PC98) */
    KeyColon = 203, /* (NEC PC98) */
    KeyStop = 120, /* (NEC PC98) */
    KeyNextTrack = 258, /* Next Track */
    KeyNumPadEnter = 88, /* Enter on numeric keypad */
    KeyRightControl = 228,
    KeyMute = 262, /* Mute */
    KeyCalculator = 226, /* Calculator */
    KeyPlayPause = 261, /* Play / Pause */
    KeyMediaStop = 260, /* Media Stop */
    KeyVolumeDown = 129, /* Volume - */
    KeyVolumeUp = 128, /* Volume + */
    KeyWebHome = 269, /* Web home */
    KeyNumPadComma = 133, /* , on numeric keypad (NEC PC98) */
    KeyNumPadDivide = 84, /* / on numeric keypad */
    Key_SYSRQ = 154,
    KeyRightAlt = 230, /* right Alt */
    KeyAltCar = KeyRightAlt, /* right Alt */
    KeyPause = 72, /* Pause */
    KeyHome = 74, /* Home on arrow keypad */
    KeyUp = 82, /* UpArrow on arrow keypad */
    KeyPageUp = 75, /* PgUp on arrow keypad */
    KeyLeft = 80, /* LeftArrow on arrow keypad */
    KeyRight = 79, /* RightArrow on arrow keypad */
    KeyEnd = 77, /* End on arrow keypad */
    KeyDown = 81, /* DownArrow on arrow keypad */
    KeyPageDown = 78, /* PgDn on arrow keypad */
    KeyInsert = 73, /* Insert on arrow keypad */
    KeyDelete = 76, /* Delete on arrow keypad */
    KeyLeftWindows = 227, /* Left Windows key */
    KeyRightWindows = 231, /* Right Windows key */
    KeyAppMenu = 118, /* AppMenu key */
    KeyPower = 102, /* System Power */
    KeySleep = 282, /* System Sleep */
    KeyWebSearch = 268, /* Web Search */
    KeyWebFavorites = 112, /* Web Favorites */
    KeyWebRefresh = 273, /* Web Refresh */
    KeyWebStop = 272, /* Web Stop */
    KeyWebForward = 271, /* Web Forward */
    KeyWebBack = 270, /* Web Back */
    KeyMyComputer = 267, /* My Computer */
    KeyMail = 265, /* Mail */
    KeyMediaSelect = 263, /* Media Select */

                          /*
                          *  Alternate names for keys originally not used on US keyboards.
                          */
    KeyCircomflex = KeyPreviousTrack, /* Japanese keyboard */

                                      /*
                                      * X-Arcade
                                      */
    XArcadeLeftPaddle = Key3, /* Key3 */
    XArcadeRightPaddle = Key4, /* Key4 */

    XArcade1Player = Key1, /* Key1 */
    XArcade2Player = Key2, /* Key2 */

    XArcadeLJoyLeft = KeyNumPad4, /* KeyNumPad4 */
    XArcadeLJoyRight = KeyNumPad6, /* KeyNumPad6 */
    XArcadeLJoyUp = KeyNumPad8, /* KeyNumPad8 */
    XArcadeLJoyDown = KeyNumPad2, /* KeyNumPad2 */

    XArcadeRJoyLeft = KeyD, /* KeyD */
    XArcadeRJoyRight = KeyG, /* KeyG */
    XArcadeRJoyUp = KeyR, /* KeyR */
    XArcadeRJoyDown = KeyF, /* KeyF */

    XArcadeLButton1 = KeyLeftControl, /* KeyLeftControl */
    XArcadeLButton2 = KeyLeftAlt, /* KeyLeftAlt */
    XArcadeLButton3 = KeySpaceBar, /* KeySpaceBar */
    XArcadeLButton4 = KeyLeftShift, /* KeyLeftShift */
    XArcadeLButton5 = KeyZ, /* KeyZ */
    XArcadeLButton6 = KeyX, /* KeyX */
    XArcadeLButton7 = KeyC, /* KeyC */
    XArcadeLButton8 = Key5, /* Key5 */

    XArcadeRButton1 = KeyA, /* KeyA */
    XArcadeRButton2 = KeyS, /* KeyS */
    XArcadeRButton3 = KeyQ, /* KeyQ */
    XArcadeRButton4 = KeyW, /* KeyW */
    XArcadeRButton5 = KeyE, /* KeyE */
    XArcadeRButton6 = KeyLeftBracket, /* KeyLeftBracket */
    XArcadeRButton7 = KeyRightBracket, /* KeyRightBracket */
    XArcadeRButton8 = Key6 /* Key6 */
};

const int DKI_MOUSE_BUTTON1 = 256;
const int DKI_MOUSE_BUTTON2 = 257;
const int DKI_MOUSE_BUTTON3 = 258;
const int DKI_MOUSE_BUTTON4 = 259;
const int DKI_MOUSE_BUTTON5 = 260;
const int DKI_MOUSE_BUTTON6 = 261;
const int DKI_MOUSE_BUTTON7 = 262;
const int DKI_MOUSE_BUTTON8 = 263;

const int DKI_JOY_BUTTON1 = 264; // 128 Buttons. DKI_JOY_BUTTON1 + n

int             dkiGetFirstDown();
int             dkiGetMouseWheelVel();
CVector2i       dkiGetMouse();
CVector2i       dkiGetMouseVel();
int             dkiGetState(int inputID);
CVector3f       dkiGetJoy();
CVector3f       dkiGetJoyR();
CVector3f       dkiGetJoyVel();
void            dkiUpdate(float elapsef);

const int DKP_TRANS_LINEAR = 0;
const int DKP_TRANS_FASTIN = 1;
const int DKP_TRANS_FASTOUT = 2;
const int DKP_TRANS_SMOOTH = 3;

#define DKP_ZERO                           0
#define DKP_ONE                            1
#define DKP_SRC_COLOR                      0x0300
#define DKP_ONE_MINUS_SRC_COLOR            0x0301
#define DKP_SRC_ALPHA                      0x0302
#define DKP_ONE_MINUS_SRC_ALPHA            0x0303
#define DKP_DST_ALPHA                      0x0304
#define DKP_ONE_MINUS_DST_ALPHA            0x0305

#define DKP_DST_COLOR                      0x0306
#define DKP_ONE_MINUS_DST_COLOR            0x0307
#define DKP_SRC_ALPHA_SATURATE             0x0308



// Struct pratique pour se créer des presets

/// \brief conteneur de configurations de particules
///
/// Cette structure permet une utilisation plus flexible de la création de particules en isolant les nombreux paramètres de création. Cette structure peut être passée à dkpCreateParticleExP().
/// Voir la définition des paramètres de dkpCreateParticleEx() pour plus de détails sur les membres de cette structure : il s'agit exactement des mêmes champs.
///
struct dkp_preset {
    CVector3f positionFrom;
    CVector3f positionTo;
    CVector3f direction;
    float speedFrom;
    float speedTo;
    float pitchFrom;
    float pitchTo;
    float startSizeFrom;
    float startSizeTo;
    float endSizeFrom;
    float endSizeTo;
    float durationFrom;
    float durationTo;
    CColor4f startColorFrom;
    CColor4f startColorTo;
    CColor4f endColorFrom;
    CColor4f endColorTo;
    float angleFrom;
    float angleTo;
    float angleSpeedFrom;
    float angleSpeedTo;
    float gravityInfluence;
    float airResistanceInfluence;
    unsigned int particleCountFrom;
    unsigned int particleCountTo;
    unsigned int *texture;
    int textureFrameCount;
    unsigned int srcBlend;
    unsigned int dstBlend;
};



// Les fonction du DKP

/// \brief non utilisée
///
/// Non utilisée
///
void            dkpCreateBillboard( CVector3f & positionFrom,
                                    CVector3f & positionTo,
                                    float fadeSpeed,
                                    float fadeOutDistance,
                                    float size,
                                    CColor4f & color,
                                    unsigned int textureID,
                                    unsigned int srcBlend,
                                    unsigned int dstBlend);



/// \brief création d'une particule
///
/// Cette fonction permet de créer une particule par appel. Il s'agit ici d'une fonction impliquant un minimum de controle sur le comportement de la particule créé.
///
/// \param position position de départ de la particule par rapport à l'origine de la scène
/// \param vel vecteur vitesse de départ de la particule
/// \param startColor couleur de départ de la particule
/// \param endColor couleur de fin de la particule
/// \param startSize grosseur de départ de la particule
/// \param endSize grosseur de fin de la particule
/// \param duration durée de vie de la particule
/// \param gravityInfluence pourcentage d'influence de la gravité sur la particule
/// \param airResistanceInfluence coefficient de frottement de l'air sur la particule
/// \param rotationSpeed vitesse de rotation de la particule (l'axe de rotation est parallèle à la droite que forme la caméra et la particule et le sens de rotation est déterminé par le signe du nombre)
/// \param texture identifiant unique d'une texture OpenGL chargée en mémoire qui sera la partie visible de la particule
/// \param srcBlend drapeau représentant l'une des 9 configurations possibles du pixel source pour le mélange de couleur(blending)
/// \param dstBlend drapeau représentant l'une des 8 configurations possibles du pixel destination pour le mélange de couleur(blending)
/// \param transitionFunc non utilisé (peut être toujours mis à 0)
void            dkpCreateParticle(  float *position,
                                    float *vel,
                                    float *startColor,
                                    float *endColor,
                                    float startSize,
                                    float endSize,
                                    float duration,
                                    float gravityInfluence,
                                    float airResistanceInfluence,
                                    float rotationSpeed,
                                    unsigned int texture,
                                    unsigned int srcBlend,
                                    unsigned int dstBlend,
                                    int transitionFunc);



/// \brief création d'une particule avec plus de controle
///
/// Cette fonction permet de créer une ou un groupe de particules avec ou sans animations par appel. Il s'agit ici d'une fonction impliquant plus de controle sur le comportement de la particule créé que la fonction dkpCreateParticle().
/// Chaque paire de paramètre dont les noms se terminent par 'From' et 'To' définissent une portée de valeurs à l'intérieur de laquelle une certaine valeur sera choisie aléatoirement.
///
/// \param positionFrom position de départ de la particule (extémité d'une boite alignée avec chaque axe du repère de la scène, la position généré aléatoirement se trouvera dans cette boite)
/// \param positionTo position de départ de la particule ( seconde extémité d'une boite alignée avec chaque axe du repère de la scène, la position généré aléatoirement se trouvera dans cette boite)
/// \param direction vecteur direction de départ de la particule (sera multiplié par 'speed' pour donner le vecteur vitesse de départ de la particule)
/// \param speedFrom vitesse de départ de la particule
/// \param speedTo vitesse de départ de la particule
/// \param pitchFrom angle de faisceau de départ (entre 0 et 360)
/// \param pitchTo angle de faisceau de départ(entre 0 et 360)
/// \param startSizeFrom grandeur de départ
/// \param startSizeTo grandeur de départ
/// \param endSizeFrom grandeur de fin (grandeur qu'aura la particule à la fin de sa durée de vie, l'interpolation est linéaire)
/// \param endSizeTo grandeur de fin (grandeur qu'aura la particule à la fin de sa durée de vie, l'interpolation est linéaire)
/// \param durationFrom durée de vie
/// \param durationTo durée de vie
/// \param startColorFrom couleur de départ
/// \param startColorTo couleur de départ
/// \param endColorFrom couleur de fin (couleur qu'aura la particule à la fin de sa durée de vie, l'interpolation est linéaire)
/// \param endColorTo couleur de fin (couleur qu'aura la particule à la fin de sa durée de vie, l'interpolation est linéaire)
/// \param angleFrom angle de départ
/// \param angleTo angle de départ
/// \param angleSpeedFrom vitesse de rotation
/// \param angleSpeedTo vitesse de rotation
/// \param gravityInfluence pourcentage d'influence de la gravité sur la particule
/// \param airResistanceInfluence coefficient de frottement de l'air sur la particule
/// \param particleCountFrom nombre de particule devant être créées
/// \param particleCountTo nombre de particule devant être créées
/// \param texture tableau d'identifiants uniques de textures OpenGL chargées en mémoire. L'ordre du tableau déterminera l'animation de la particule
/// \param textureFrameCount nombre de textures contenues dans le paramètre 'texture'. Ce nombre détermine aussi le nombre d'images constituants l'animation de la ou des particules
/// \param srcBlend drapeau représentant l'une des 9 configurations possibles du pixel source pour le mélange de couleur(blending)
/// \param dstBlend drapeau représentant l'une des 8 configurations possibles du pixel destination pour le mélange de couleur(blending)
void            dkpCreateParticleEx(const CVector3f & positionFrom,
                                    const CVector3f & positionTo,
                                    const CVector3f & direction,
                                    float speedFrom,
                                    float speedTo,
                                    float pitchFrom,
                                    float pitchTo,
                                    float startSizeFrom,
                                    float startSizeTo,
                                    float endSizeFrom,
                                    float endSizeTo,
                                    float durationFrom,
                                    float durationTo,
                                    const CColor4f & startColorFrom,
                                    const CColor4f & startColorTo,
                                    const CColor4f & endColorFrom,
                                    const CColor4f & endColorTo,
                                    float angleFrom,
                                    float angleTo,
                                    float angleSpeedFrom,
                                    float angleSpeedTo,
                                    float gravityInfluence,
                                    float airResistanceInfluence,
                                    unsigned int particleCountFrom,
                                    unsigned int particleCountTo,
                                    unsigned int *texture,
                                    int textureFrameCount,
                                    unsigned int srcBlend,
                                    unsigned int dstBlend);
void            dkpCreateParticleExP(dkp_preset & preset);
void            dkpInit();
void            dkpRender();
void            dkpReset();
void            dkpSetAirDensity(float airDensity);
void            dkpSetGravity(float *vel);
int             dkpUpdate(float delay);

const int DKT_FILTER_NEAREST = 0;
const int DKT_FILTER_LINEAR = 1;
const int DKT_FILTER_BILINEAR = 2;
const int DKT_FILTER_TRILINEAR = 3;
const int DKT_LUMINANCE = 1;
const int DKT_RGB = 3;
const int DKT_RGBA = 4;

unsigned int dktCreateEmptyTexture(int w, int h, int bpp, int filter);
void         dktCreateTextureFromBuffer(unsigned int *textureID, unsigned char *buffer, int w, int h, int bpp, int filter);
unsigned int dktCreateTextureFromFile(char *filename, int filter);
void         dktDeleteTexture(unsigned int *textureID);
void         dktGetTextureData(unsigned int textureID, unsigned char * data);

void      dkwForceQuit();
void      dkwSwap();
char*     dkwGetLastError();
CVector2i dkwGetCursorPos();
CVector2i dkwGetResolution();
int       dkwMainLoop();
void      dkwClipMouse( bool abEnabled );

// Les constantes
#define DKO_BUMP_MAP            0x0001 // Not yet
#define DKO_COLOR_ARRAY         0x0002 // Not yet
#define DKO_DETAIL_MAP          0x0004
#define DKO_DYNAMIC_LIGHTING    0x0008
#define DKO_FORCE_WIREFRAME     0x0010
#define DKO_MULTIPASS           0x0020
#define DKO_SELFILL_MAP         0x0040
#define DKO_SPECULAR            0x0080
#define DKO_SPECULAR_MAP        0x0100
#define DKO_TEXTURE_MAP         0x0200
#define DKO_RENDER_BB           0x0400
#define DKO_RENDER_NODE         0x0800
#define DKO_RENDER_FACE         0x1000
#define DKO_CLAMP_TEXTURE       0x2000

// Les fonction du DKO
int             dkoAddAnimationFromFile(unsigned int modelID, char* filename, char* animationName);
void            dkoAddLight(unsigned int modelID, float *position, float *diffuse, float *specular, float range);
void            dkoBuildOctree(unsigned int modelID);
void            dkoComputeStaticLighting(unsigned int modelID);
void            dkoEnable(unsigned int renderFlags);
void            dkoDeleteModel(unsigned int *modelID);
void            dkoDisable(unsigned int renderFlags);
unsigned int    dkoGetDummy(char * dummyName, unsigned int modelID);
void            dkoGetDummyMatrix(char * dummyName, unsigned int modelID, float * mat);
void            dkoGetDummyMatrix(unsigned int dummyID, unsigned int modelID, float * mat);
void            dkoGetDummyMatrix(char * dummyName, unsigned int modelID, float * mat, short frameID);
void            dkoGetDummyMatrix(unsigned int dummyID, unsigned int modelID, float * mat, short frameID);
void            dkoGetDummyPosition(char * dummyName, unsigned int modelID, float * pos);
void            dkoGetDummyPosition(unsigned int dummyID, unsigned int modelID, float * pos);
void            dkoGetDummyPosition(char * dummyName, unsigned int modelID, float * pos, short frameID);
void            dkoGetDummyPosition(unsigned int dummyID, unsigned int modelID, float * pos, short frameID);
char*           dkoGetDummyName(unsigned int dummyID, unsigned int modelID);
char*           dkoGetLastError();
int             dkoGetNbVertex(unsigned int modelID);
void            dkoGetOABB(unsigned int modelID, float *OABB);
float           dkoGetRadius(unsigned int modelID);
short           dkoGetTotalFrame(unsigned int modelID);
float*          dkoGetVertexArray(unsigned int modelID, float frameID, int & nbVertex, float *vertexArray);
void            dkoInit();
void            dkoInitLightList(unsigned int modelID);
unsigned int    dkoLoadFile(char* filename);
unsigned int    dkoLoadFile(unsigned int modelID);
void            dkoOutputDebugInfo(unsigned int modelID, char *filename);
void            dkoPopRenderState();
void            dkoPushRenderState();
bool            dkoRayIntersection(unsigned int modelID, float *p1, float *p2, float *intersect, float *normal, int &n);
void            dkoRender(unsigned int modelID); // Va renderer automatiquement le premier frame
void            dkoRender(unsigned int modelID, unsigned short frameID); // Sp�ifions un frame
void            dkoRender(unsigned int modelID, float frameID); // Avec interpolation (MaLaDe)
bool            dkoSphereIntersection(unsigned int modelID, float *p1, float *p2, float rayon, float *intersect, float *normal, int &n);



using FSOUND_SAMPLE = void;
void FSOUND_3D_Listener_SetAttributes(float* pos, float* vel, float fx, float fy, float fz, float tx, float ty, float tz);
void FSOUND_Update();
void FSOUND_StopSound(int channel);
void FSOUND_SetSFXMasterVolume(int vol);

#ifdef USE_FMODEX
FMOD_SOUND *    dksCreateSoundFromFile(char* filename, bool loop=false);
void            dksDeleteSound(FMOD_SOUND * fsound_sample);
int             dksPlaySound(FMOD_SOUND * fsound_sample, int channel, int volume=255);
void            dksPlay3DSound(FMOD_SOUND * fsound_sample, int channel, float range, CVector3f & position, int volume=255);
void            dksSet3DListenerAttributes(const CVector3f * pos, const CVector3f * vel, const CVector3f * forward, const CVector3f * up);
void            dksUpdate();
void            dksSetSfxMasterVolume(float volume);
void            dksStopSound(FMOD_SOUND * s);
FMOD_SYSTEM *   dksGetSystem();
FMOD_CHANNEL *  dksGetChannel(FMOD_SOUND * s);
#else
FSOUND_SAMPLE*  dksCreateSoundFromFile(char* filename, bool loop=false);
void            dksDeleteSound(FSOUND_SAMPLE* fsound_sample);
int             dksPlaySound(FSOUND_SAMPLE * fsound_sample, int channel, int volume=255);
void            dksPlay3DSound(FSOUND_SAMPLE * fsound_sample, int channel, float range, CVector3f & position, int volume=255);
#endif
void            dksPlayMusic(char* filename, int channel=-1, int volume=255);
void            dksStopMusic();

#endif
