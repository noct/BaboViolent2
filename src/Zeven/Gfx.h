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



/// \brief Interface principale entre le module et le code externe
///
/// Cette classe est l'interface principale entre le module et le code externe. Elle ne contient que 2 fonctions virtuelles pures devant être redéfinies dans une classe-dérivé.
/// Ces 2 fonctions seront automatiquement appelées par la fonction 'callback'. Une instance d'une classe dérivée de celle-ci devra être passée en paramètre à dkwInit() afin de générer une fenêtre Windows valide et active.
///
/// \note : Il est possible de ne pas dériver cette classe abstraite et de simplement passer 0 en paramètre à dkwInit(). Ceci permettra quand même à l'application de s'exécuter sans toutefois générer de rendu ni de mise à jour de la logique d'affaire.
class CMainLoopInterface
{
public:


    /// \brief fonction essentielle qui doit inclure l'aspect Vue et Logique d'affaire de l'application
    ///
    /// Cette fonction doit être redéfinie de façon à gérer toutes les opérations de rendu et toute la logique du jeu. Elle sera appelé automatiquement par la fonction 'callback' à chaque fois qu'un message système WM_PAINT sera reçu. Cet action marquera le début d'un cycle d'exécution du programme.
    ///
    virtual void paint() = 0;



    /// \brief fonction qui donne un moyen de gèrer les caractères entrée au clavier
    ///
    /// Cette fonction doit être redéfinie mais cette redéfinition peut simplement être vide si un autre moyen a été utilisé pour gérer les entrées de caractères (avec le module DKI par exemple)
    ///
    /// \param caracter caractère entré au clavier passé depuis la fonction 'callback' à l'appel
    virtual void textWrite(unsigned int caracter) = 0;
};

/// \param width dimension en pixel de la fenêtre souhaitée
/// \param height dimension en pixel de la fenêtre souhaitée
/// \param title chaine de caractères définissant le nom que portera la fenêtre créée
/// \param mMainLoopObject pointeur vers une classe dérivée de la classe CMainLoopInterface qui contient la définition de la fonction principale du programme (Paint())
/// \param fullScreen booléen qui détermine si la fenêtre doit être en mode plein écran(true) ou en mode fenêtre(false)
/// \return int retourne 1 si la création de la fenêtre s'est bien déroulé, retourne 0 si une erreur s'est produite

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
///
/// De plus, le caractère retour de chariot (\n) peut être utilisé afin de pouvoir écrire sur plus d'une ligne avec un seul appel à dkfPrint().
/// \author David St-Louis (alias Daivuk)
/// \author Louis Poirier (à des fins de documentation seulement)
///

// Les fonction du DKT

/// \brief spécifie la police de caractère à devant être utilisé
///
/// Cette fonction permet de spécifier la police de caractères qui sera active. La police de caractères active est celle qui sera utilisé pour l'affichage d'une chaine de caractères et pour divers calculs de longueurs de chaines.
/// Si on désire changer de police, un nouvel appel spécifiant la nouvelle police de caractère devant être active devra être fait.
///
/// \param ID identifiant unique de la police de caractères
void            dkfBindFont(unsigned int ID);



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



/// \brief destruction d'une police de caractères
///
/// Cette fonction libère la mémoire allouée dans la création d'une police de caractères
///
/// \param ID pointeur vers l'identifiant unique référant la police de caractères devant être effacée
void            dkfDeleteFont(unsigned int *ID);



/// \brief obtient la position d'un caractère dans une chaine de caractère
///
/// Cette fonction obtient la position de la première occurence d'un caractère qui se trouve possiblement dans une chaine de caractère.
/// La police de caractères active est utilisé pour évaluer la largeur de chaque caractère. Les caractères \n sont aussi considérés dans le calcul. Ce qui veut dire que la hauteur du caractère peut ne pas être nulle.
///
/// \param size hauteur en pixel de la chaine de caractères
/// \param text chaine de caractères à considérer
/// \param caracter caractère dont la position sera retourner
/// \return position de la première occurence du caractère dans la chaine de caractères à considérer
CPoint2f        dkfGetCaracterPos(float size, char *text, int caracter);



/// \brief obtient l'index du caractère se trouvant à une certaine position dans une chaine de caractères
///
/// Cette fonction retourne le nombre désignant le n ième caractère d'une chaine de caractères qui se trouve à la position onStringPos.
///
/// \param size hauteur en pixel de la chaine de caractères
/// \param text chaine de caractères à considérer
/// \param onStringPos position cible en pixel dont l'origine se trouve dans le coin supérieur gauche du premier caractère
/// \return le n ième caractère d'une chaine de caractères qui se trouve à la position onStringPos
int             dkfGetOverStringCaracter(float size, char *text, CPoint2f & onStringPos);



/// \brief retourne la hauteur total d'une chaine de caractères en considérant les caractères \n
///
/// Cette fonction retourne la hauteur total en pixel d'une chaine de caractères en considérant les caractères \n.
///
/// \param size hauteur en pixel d'un caractère
/// \param text chaine de caractères à considérer
/// \return hauteur total en pixels
float           dkfGetStringHeight(float size, char *text);



/// \brief retourne la largeur de la plus grande sous-chaine d'une chaine de caractères délimitées par les caractères \n
///
/// Cette fonction retourne la largeur de la plus grande sous-chaine d'une chaine de caractères délimitées par les caractères \n
///
/// \param size hauteur en pixel d'un caractère
/// \param text chaine de caractères à considérer
/// \return largeur de la plus grande sous-chaine
float           dkfGetStringWidth(float size, char *text);



/// \brief dessine une chaine de caractères
///
/// Cette fonction dessine une chaine de caractères sur un QUAD (polygone à 4 coté) en 3D en utilisant la police de caractères active, les caractères de couleurs et le caractères retour de chariot (\n).
///
/// \param size grandeur du texte à dessiner en pourcentage par rapport à la hauteur originale (l'aggrandissement est valide pour la largeur et la hauteur du texte)
/// \param x position du texte en 3D
/// \param y position du texte en 3D
/// \param z position du texte en 3D
/// \param text chaine de caractères à dessiner à l'écran
void            dkfPrint(float size, float x, float y, float z, char *text);

/// \name BlendingPreset
/// Drapeaux représentant chacun une configuration de coefficients de mélange de couleurs d'un pixel source et d'un pixel destination fréquemment utilisées. Voir DKP pour plus de détails sur le mélange de couleur (blending)
//@{
/// représente la paire de coefficients (source, destination) : (GL_ONE, GL_ONE)
const int DKGL_BLENDING_ADD_SATURATE = 0;
/// représente la paire de coefficients (source, destination) : (GL_SRC_ALPHA, GL_ONE)
const int DKGL_BLENDING_ADD = 3;
/// représente la paire de coefficients (source, destination) : (GL_DST_COLOR, GL_ZERO)
const int DKGL_BLENDING_MULTIPLY = 1;
/// représente la paire de coefficients (source, destination) : (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
const int DKGL_BLENDING_ALPHA = 2;
//@}


// Les fonction du DKG


/// \brief vérifie la présence d'une extension d'OpenGL supportée par la carte vidéo
///
/// Cette fonction vérifie la présence d'une extension d'OpenGL supportée par la carte vidéo.
///
/// \param extension nom de l'extension
/// \return true si elle est supportée, false sinon
bool            dkglCheckExtension(char * extension);

/// \brief dessine le repère vectoriel de la scène à l'origine
///
/// Cette fonction dessine le repère vectoriel de la scène à l'origine. Le repère vectoriel est constitué de 3 vecteurs tous perpendiculaire l'un par rapport à l'autre.
///
void            dkglDrawCoordSystem();



/// \brief dessine un cube en fil de fer
///
/// Cette fonction dessine un cube en fil de fer.
///
void            dkglDrawWireCube();



/// \brief permet de revenir en mode de rendu en perspective
///
/// Cette fonction de passer du mode de rendu orthographique (surtout utilisé pour dessiner en 2D sur l'écran ou pour conserver le rapport des mesures comme dans les applications CAD) au mode de rendu en perspective 3D.
///
void            dkglPopOrtho();



/// \brief permet de passer en mode de rendu orthographique
///
/// Cette fonction passer du mode de rendu en perspective 3D au mode de rendu orthographique possédant une certaine dimension de rendu.
///
/// \param mWidth dimension en pixel du mode de rendu orthographique
/// \param mHeight dimension en pixel du mode de rendu orthographique
void            dkglPushOrtho(float mWidth, float mHeight);



/// \brief permet de spécifier la fonction de mélange de couleur qui est active
///
/// Cette fonction permet de spécifier la fonction de mélange de couleur(blending) qui est active en passant un des 4 drapeaux BlendingPreset en paramètre
///
/// \param blending drapeau BlendingPreset qui défini une fonction de mélange de couleur(blending)
void            dkglSetBlendingFunc(int blending);


/// enable vsync ( true or false )
void            dkglEnableVsync(bool enabled = true);


/// \brief active une lumière OpenGL
///
/// Cette fonction active une des 8 lumières qu'OpenGL offre avec les spécificités suivantes :
///     - position = {x,y,z,1}
///     - couleur ambiente = {r/4,g/4,b/4,1}
///     - couleur diffuse = {r,g,b,1}
///     - couleur spéculaire = {r,g,b,1}
///
/// \param ID Identifiant unique de la lumière (de 0 à 7)
/// \param x position de la lumière
/// \param y position de la lumière
/// \param z position de la lumière
/// \param r couleur de la lumière
/// \param g couleur de la lumière
/// \param b couleur de la lumière
void            dkglSetPointLight(int ID, float x, float y, float z, float r, float g, float b);



/// \brief spécifie et active un mode de rendu en perspective
///
/// Cette fonction spécifie et active un mode de rendu en perspective.
///
/// \param mFieldOfView angle de vue vertical en degrés
/// \param mNear distance la plus proche de la caméra pouvant posséder un rendu
/// \param mFar distance la plus éloignée de la caméra pouvant posséder un rendu
/// \param mWidth largeur de la fenètre (unité arbitraire, seul le ratio mWidth/mHeight importe vraiment)
/// \param mHeight hauteur de la fenètre (unité arbitraire)
void            dkglSetProjection(float mFieldOfView, float mNear, float mFar, float mWidth, float mHeight);

/// \brief transforme la position de la souris 2D en un vecteur 3D
///
/// Cette fonction permet de faire correspondre la position de la souris et une certaine valeur entre [0,1] à un vecteur 3D. Le vecteur est simplement construit en prenant la position de la souris et en y ajoutant zRange (z,y,zRange) et en y soustrayant la position de la caméra.
/// On obtient alors un vecteur que l'on multiplie par la valeur de profondeur la plus éloigné de la caméra pouvant posséder un rendu. C'est ce nouveau vecteur qui est retourné.
///
/// \param pos2D position de la souris à l'écran en pixel
/// \param zRange profondeur désirée (entre 0 et 1)
/// \return le nouveau vecteur représentant correspondant à la position de la souris en 3D à une certaine profondeur.
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

// Les boutons de la mouse
/// \name Constantes d�signants les diff�rents boutons d'une souris
//@{
const int DKI_MOUSE_BUTTON1 = 256;
const int DKI_MOUSE_BUTTON2 = 257;
const int DKI_MOUSE_BUTTON3 = 258;
const int DKI_MOUSE_BUTTON4 = 259;
const int DKI_MOUSE_BUTTON5 = 260;
const int DKI_MOUSE_BUTTON6 = 261;
const int DKI_MOUSE_BUTTON7 = 262;
const int DKI_MOUSE_BUTTON8 = 263;
//@}


// Les boutons du joystick
/// \name Constante d�signant les diff�rents boutons d'un joystick
/// Il y a une limite maximum de 128 buttons d�tectables. Pour atteindre le n i�me bouton, on n'a qu'� faire : DKI_JOY_BUTTON1 + n
//@{
const int DKI_JOY_BUTTON1 = 264; // 128 Buttons. DKI_JOY_BUTTON1 + n
//@}


// Les fonction du DKI

/// \brief retourne la constante d�signant la premi�re touche ayant �t� appuy�e
///
/// Cette fonction permet de connaitre la touche ou le bouton qui a �t� appuy� en premier. La fonction effectue la v�rification pour un instant seulement (lors de l'appel). Un appel pour chaque cycle d'ex�cution est donc n�cessaire afin de savoir si une touche a �t� pes� dans un certain intervalle de temps.
///
/// \return l'index repr�sentant la touche ou le bouton qui a �t� appuy� en premier
int             dkiGetFirstDown();


/// \brief retourne la vitesse � laquelle la roulette de la souris est d�plac�e
///
/// Cette fonction retourne la vitesse � laquelle la roulette de la souris est d�plac�e en nombre de clic
/// -2 signifirait que la wheel a �t� d�cendu 2 fois. (normalement on s'en tien � -1,0 ou 1)
/// Exemple d'utilisation : if (dkiGetMouseWheelVel() < 0) zoomOut();
///
/// \return retourne la vitesse � laquelle la roulette de la souris est d�plac�e
int             dkiGetMouseWheelVel();



/// \brief retourne la position actuelle de la souris
///
/// Cette fonction retourne la position actuelle de la souris. Cette position est en pixel et l'origine est le coin sup�rieur gauche de l'�cran.
///
/// \return retourne la position actuelle de la souris en pixel
CVector2i       dkiGetMouse();



/// \brief retourne la vitesse � laquelle se d�place la souris
///
/// Cette fonction retourne le d�placement effectu� par la souris en pixels depuis le dernier appel a dkiUpdate().
/// Le d�placement retourn� n'est pas d�pendant de la grandeur de la fen�tre (il y aura un certain d�placement retourn� m�me si on d�place la souris vers la gauche et que la derni�re position �tait (0,0).
/// Parfait pour les jeux de style FPS
///
/// \return retourne la vitesse � laquelle se d�place la souris
CVector2i       dkiGetMouseVel();



/// \brief retourne l'�tat d'une touche ou d'un bouton
///
/// Cette fonction retourne l'�tat d'une touche ou d'un bouton.
///
/// \param inputID identifiant unique de la touche ou du bouton
/// \return �tat de la touche ou du bouton
int             dkiGetState(int inputID);



/// \brief retourne la position de chaque axe d'un joystick ou d'une manette de jeu
///
/// Cette fonction retourne la position de chaque axe d'un joystick ou d'une manette de jeu. La position au repos �tant 0 et les extr�mes �tant -1 et 1.
/// Aucune "dead zone" et courbe de progression est d�fini ici. C'est au client de le faire.
///
/// \return position de chaque axe
CVector3f       dkiGetJoy();
CVector3f       dkiGetJoyR();



/// \brief retourne la vitesse � laquelle se d�place chacun des axes d'un joystick ou d'une manette de jeu
///
/// Cette fonction retourne la vitesse � laquelle se d�place chacun des axes d'un joystick ou d'une manette de jeu par rapport au dernier appel � dkiUpdate().
/// Si la derni�re position d'un axe �tait de -1 et que sa position est � 1 lors de l'appel, la valeur 2 sera retourn� pour cet axe.
///
/// \return vitesse de chaque axe
CVector3f       dkiGetJoyVel();

/// \brief mise � jour des �tats des p�riph�riques d'entr�es
///
/// Cette fonction effectue la mise � jour des �tats des p�riph�riques d'entr�es (clavier, souris, joystick). Elle doit �tre appel�e une fois par cycle d'ex�cution.
/// On doit sp�cifier la dimension de la fen�tre dans laquelle le pointeur de la souris ne pourra pas exc�der.
///
/// \param elapsef non utilis�
/// \param width dimension en pixel de la fen�tre
/// \param height dimension en pixel de la fen�tre
void            dkiUpdate(float elapsef, int width, int height);


// Setter la position du cursor
void            dkiSetMouse(CVector2i & mousePos);


/// \name not used
/// constantes non utilisées
//@{
const int DKP_TRANS_LINEAR = 0;
const int DKP_TRANS_FASTIN = 1;
const int DKP_TRANS_FASTOUT = 2;
const int DKP_TRANS_SMOOTH = 3;
//@}


// BlendingFactorDest
/// \name BlendingFactorDest
/// Drapeaux représentant les 8 configurations possibles pour les coefficients de mélange de couleur (blending) du pixel destination
//@{
#define DKP_ZERO                           0
#define DKP_ONE                            1
#define DKP_SRC_COLOR                      0x0300
#define DKP_ONE_MINUS_SRC_COLOR            0x0301
#define DKP_SRC_ALPHA                      0x0302
#define DKP_ONE_MINUS_SRC_ALPHA            0x0303
#define DKP_DST_ALPHA                      0x0304
#define DKP_ONE_MINUS_DST_ALPHA            0x0305
//@}

// BlendingFactorSrc
/// \name BlendingFactorSrc
/// Drapeaux représentant les 9 configurations possibles pour les coefficients de mélange de couleur (blending) du pixel source
/// \note Il y a 6 drapeaux communs entre BlendingFactorDest et BlendingFactorSrc
//@{
///     DKP_ZERO
///     DKP_ONE
#define DKP_DST_COLOR                      0x0306
#define DKP_ONE_MINUS_DST_COLOR            0x0307
#define DKP_SRC_ALPHA_SATURATE             0x0308
///     DKP_SRC_ALPHA
///     DKP_ONE_MINUS_SRC_ALPHA
///     DKP_DST_ALPHA
///     DKP_ONE_MINUS_DST_ALPHA
//@}


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



/// \brief création d'une particule avec plus de controle et de flexibilité
///
/// Cette fonction accomplie exactement la même chose que dkpCreateParticleEx() mais en utilisant la structure dkp_preset comme paramètre.
///
/// \param preset préconfiguration de la génération de particules
void            dkpCreateParticleExP(dkp_preset & preset);



/// \brief initialisation du module
///
/// Cette fonction effectue l'initialisation du module et doit être appelé AVANT tout autres appels à d'autres fonctions de ce module.
///
void            dkpInit();



/// \brief affiche toutes les particules à l'écran
///
/// Cette fonction effectue le rendu à l'ecran de toutes les particules qui ont été créées jusqu'à présent et qui sont actives.
///
void            dkpRender();




/// \brief libère la mémoire allouée pour la création de particules
///
/// Cette fonction libère toute la mémoire allouée pour la création des particules présentement actives. Toutes les particules créées seront effacées.
///
void            dkpReset();



/// \brief spécifie une densité de l'air
///
/// Cette fonction permet de changer la densité de l'air qui sera utiliser pour la simulation des particules (leurs vitesses sera décélérées proportionnellement à cette valeur)
///
/// \param airDensity nouvelle densité de l'air
void            dkpSetAirDensity(float airDensity);



/// \brief spécifie une attraction gravitationnelle
///
/// Cette fonction permet de changer l'attraction gravitationnelle qui sera utiliser pour la simulation des particules.
///
/// \param vel vecteur accélération gravitationnelle
void            dkpSetGravity(float *vel);



/// \brief active le triage des particules
///
/// Cette fonction permet d'activer ou de désactiver le triage des particules qui seront créées après l'appel. Ce triage fait en sorte que la particule la plus éloignée de la caméra sera rendue en premier, puis la suivante la plus éloignée et ainsi de suite. Ceci permet certain type de mélange de couleur (blending) de donner un effet attendu.
///
/// \param sort true pour activer le triage, false si on veut le désactiver
void            dkpSetSorting(bool sort);

/// \brief effectue la mise à jour des particules pour le rendu
///
/// Cette fonction effectue la mise à jour de la position, la vitesse, la durée de vie, la couleur, l'angle, et l'image de chaque particule pour le rendu.
///
/// \param delay intervalle de temps sur lequel la mise à jour est effectuée.
/// \return le nombre de particule encore actives après l'exécution de la mise à jour
int             dkpUpdate(float delay);





using FSOUND_SAMPLE = void;


// Les fonction du DKS
void FSOUND_3D_Listener_SetAttributes(float* pos, float* vel, float fx, float fy, float fz, float tx, float ty, float tz);
void FSOUND_Update();
void FSOUND_StopSound(int channel);
void FSOUND_SetSFXMasterVolume(int vol);


/// \brief chargement d'un son en mémoire
///
/// Cette fonction effectue le chargement d'un son ou d'une musique en mémoire. Les formats valides sont les même que FMOD. Avant de créer un nouveau son, le module vérifie si ce fichier a déjà été chargé. Si c'est le cas, aucun son ne sera créé et le pointeur FMOD correspondant à ce fichier sera retourné.
///
/// \param filename chemin menant au fichier son ou musique à charger depuis l'endroit où se situe le fichier EXE du programme.
/// \param loop est-ce que ce son ou cette musique doit jouer en boucle?
/// \return pointeur FMOD vers le son ou la musique chargée en mémoire
#ifdef USE_FMODEX
FMOD_SOUND *    dksCreateSoundFromFile(char* filename, bool loop=false);
#else
FSOUND_SAMPLE*  dksCreateSoundFromFile(char* filename, bool loop=false);
#endif



/// \brief destruction d'un son chargé en mémoire
///
/// Cette fonction permet de libérer la mémoire allouée pour un son présentement chargée en mémoire.
///
/// \param pointeur FMOD du son à effacer

#ifdef USE_FMODEX
void dksDeleteSound(FMOD_SOUND * fsound_sample);
#else
void            dksDeleteSound(FSOUND_SAMPLE* fsound_sample);
#endif


// Ajou de fonctions
#ifdef USE_FMODEX
int dksPlaySound(FMOD_SOUND * fsound_sample, int channel, int volume=255);
void dksPlay3DSound(FMOD_SOUND * fsound_sample, int channel, float range, CVector3f & position, int volume=255);
#else
int         dksPlaySound(FSOUND_SAMPLE * fsound_sample, int channel, int volume=255);
void            dksPlay3DSound(FSOUND_SAMPLE * fsound_sample, int channel, float range, CVector3f & position, int volume=255);
#endif

void            dksPlayMusic(char* filename, int channel=-1, int volume=255);
void            dksStopMusic();

#ifdef USE_FMODEX
void dksSet3DListenerAttributes(const CVector3f * pos, const CVector3f * vel, const CVector3f * forward, const CVector3f * up);
void dksUpdate();
void dksSetSfxMasterVolume(float volume);
void dksStopSound(FMOD_SOUND * s);
FMOD_SYSTEM * dksGetSystem();
FMOD_CHANNEL * dksGetChannel(FMOD_SOUND * s);
#endif




/// \name Filtering
/// Les drapeaux représentants les différents filtres communs de texturage qui détermineront la facon dont une texture affichée en 3D sera transposée en pixel d'une certaine couleur.
/// Avant d'aller plus loin, je me doit d'abord expliquer ce qu'est un Mipmap. Un Mipmap est un assortiment d'images toutes générées à partir d'une image source. L'assortiment est simplement l'ensemble des différentes résolutions de l'image de base trouvées en divisant toujours la résolution de l'image source par 2.
/// Par exemple, le Mipmap d'une image source de 64X128 sera l'ensemble : 64X128, 32X64, 16X32, 8X16, 4X8, 2X4, 1X2 et 1X1 (l'image source sera 'stretcher' pour s'ajuster à chaque résolution). En utilisant cet ensemble d'image plutot que toujours l'image source, on se retrouve à avoir un gain important de performance plus l'image à être afficher se trouve loin en 3D. Le seul cout étant la mémoire nécessaire pour conserver le Mipmap entier moins l'image source.
///
/// Il existe 2 cas dans lequel un certain calcul est nécessaire pour déterminer la couleur qu'un certain pixel aura avant d'être afficher:
///     -# le pixel couvre une région égale ou plus petite qu'un texel.
///     -# le pixel couvre une plus grande région qu'un texel (un texel est un élément de texture)
///
/// OpenGL a défini 6 facons de calculer(ou approximer) la couleur que le pixel aura avant d'être afficher:
///     - GL_NEAREST : utilise le texel de l'image source le plus près du centre du pixel à être texturé (valide pour les 2 cas)
///     - GL_LINEAR : utilise la moyenne de la couleur des 4 texels les plus près du centre du pixel à être texturé (valide pour les 2 cas)
///     - GL_NEAREST_MIPMAP_NEAREST : utilise l'image du Mipmap ayant la taille qui se rapproche le plus de la taille du pixel et applique le critère GL_NEAREST avec cette image (valide seulement dans le cas 2)
///     - GL_LINEAR_MIPMAP_NEAREST : utilise l'image du Mipmap ayant la taille qui se rapproche le plus de la taille du pixel et applique le critère GL_LINEAR avec cette image (valide seulement dans le cas 2)
///     - GL_NEAREST_MIPMAP_LINEAR : utilise les 2 images du Mipmap ayant les tailles qui se rapprochent le plus de la taille du pixel et applique le critère GL_NEAREST avec l'image résultante de la moyenne des couleur des 2 images du Mipmap choisies (valide seulement dans le cas 2)
///     - GL_LINEAR_MIPMAP_LINEAR : utilise les 2 images du Mipmap ayant les tailles qui se rapprochent le plus de la taille du pixel et applique le critère GL_LINEAR avec l'image résultante de la moyenne des couleur des 2 images du Mipmap choisies (valide seulement dans le cas 2)
///
/// On peut noter que les Mipmap ne sont utiles que dans le cas 2.
/// De ces principes, il résulte 4 configurations les plus fréquemments utilisés:
///     - DKT_FILTER_NEAREST
///         - cas 1 : GL_NEAREST
///         - cas 2 : GL_NEAREST
///     - DKT_FILTER_LINEAR
///         - cas 1 : GL_LINEAR
///         - cas 2 : GL_LINEAR
///     - DKT_FILTER_BILINEAR
///         - cas 1 : GL_LINEAR
///         - cas 2 : GL_LINEAR_MIPMAP_NEAREST
///     - DKT_FILTER_TRILINEAR
///         - cas 1 : GL_LINEAR
///         - cas 2 : GL_LINEAR_MIPMAP_LINEAR
///     - par défaut (si aucun spécifier)
///         - cas 1 : GL_NEAREST
///         - cas 2 : GL_NEAREST_MIPMAP_NEAREST
///
/// Ce procédé est déterminé lors de l'application d'une texture sur une face d'un modèle ou lors de la création d'une texture à partir d'une autre. On peut donc changer de filtre tant qu'on veut avant le rendu sur l'écran ou sur un tampon mémoire.
/// Ces drapeaux peuvent être utilisé pour chaque fonctions du module qui contient un paramètre nommé filter. La combinaison de plusieurs drapeaux n'est pas permise.
//@{
const int DKT_FILTER_NEAREST = 0;
const int DKT_FILTER_LINEAR = 1;
const int DKT_FILTER_BILINEAR = 2;
const int DKT_FILTER_TRILINEAR = 3;
//@}

/// \name BytePerPixel
/// Ces drapeaux font référence à certains formats internes de pixel qu'OpenGL utilise. Ils doivent être utilisé pour chaque fonctions du module qui contient un paramètre nommé bpp ou internalFormat.
//@{
/// 1 byte par pixel qui peut représenter 256 tons de gris pour chaque pixel
const int DKT_LUMINANCE = 1;
/// 3 byte par pixel qui peuvent représenter 256 tons de rouge, de vert et de bleu pour chaque pixel
const int DKT_RGB = 3;
/// 4 byte par pixel qui peuvent représenter 256 tons de rouge, de vert, de bleu et de transparence pour chaque pixel
const int DKT_RGBA = 4;
//@}


// Les fonction du DKT


/// \brief effectue une effet de bluring sur une texture déjà chargée en mémoire
///
/// Cette fonction permet de modifier une texture chargée en mémoire en prenant la moyenne des 8 pixels adjacents pour chaque pixel de l'image. Le nombre d'itération est déterminé par nbPass.
///
/// \param textureID identifiant unique de la texture
/// \param nbPass nombre d'itération de l'effet de bluring
void             dktBlurTexture(unsigned int textureID, int nbPass);



/// \brief permet de changer le filtre de texturage utilisée pour toute les textures présentement chargées en mémoire
///
/// Cette fonction permet de changer le filtre de texturage utilisée pour toute les textures présentement chargées en mémoire.
/// Le filtre final qui sera utilisé lors du rendu sera celui spécifié par le dernier appel fait à cette fonction (sauf si d'autre fonctions modifie aussi le filtre).
///
/// \param filter filtre de texturage à être utiliser pour toute les textures
void             dktChangeFilter(int filter);



/// \brief crée une texture blanche
///
/// Cette texture permet de créé une texture vierge ((255,255,255,255) ou (255,255,255) ou (255) pour chaque pixel). La texture créé aura les dimension wXh, aura bpp byte par pixel et utilisera le filtre de texturage filter.
///
/// \param w largeur en pixel de la texture à créer
/// \param h hauteur en pixel de la texture à créer
/// \param bpp drapeau du format de pixel à être utilisé
/// \param filter drapeau de filtre de texturage à être utiliser
/// \return identifiant unique de la texture créée
unsigned int     dktCreateEmptyTexture(int w, int h, int bpp, int filter);



/// \brief remplace une texture existante par une autre
///
/// Cette fonction permet de remplacer une texture existante par le contenu d'un certain tableau qui contient une liste de pixel défini proprement (de dimension wXh et de bpp byte par pixel). Un filtre de texturage peut aussi être spécifié.
///
/// \param textureID identifiant unique de la texture existante dont le contenu sera remplacé
/// \param buffer pointeur vers la liste des pixels qui défini une images de dimension wXh et de bpp byte par pixel
/// \param w largeur en pixel de la texture à créer
/// \param h  hauteur en pixel de la texture à créer
/// \param bpp drapeau du format de pixel à être utilisé
/// \param filter drapeau de filtre de texturage à être utiliser
void             dktCreateTextureFromBuffer(unsigned int *textureID, unsigned char *buffer, int w, int h, int bpp, int filter);



/// \brief crée une texture à partir d'un fichier targa (TGA)
///
/// Cette fonction permet de créer une texture à partir d'un fichier targa (TGA) en utilisant un filtre de texturage.
///
/// \param filename chemin menant au fichier TGA à charger depuis l'endroit où se situe le fichier EXE du programme.
/// \param filter drapeau de filtre de texturage à être utiliser
/// \return identifiant unique de la texture créée
unsigned int     dktCreateTextureFromFile(char *filename, int filter);



/// \brief libère la mémoire allouée pour une texture
///
/// Cette fonction libère la mémoire allouée pour une texture chargé précédemment.
///
/// \param textureID identifiant unique de la texture à être effacé
void             dktDeleteTexture(unsigned int *textureID);



/// \brief obtient le nombre de byte par pixel d'une texture
///
/// Cette fonction retourne le nombre de byte par pixel d'une texture déjà chargée en mémoire.
///
/// \param textureID identifiant unique de la texture
/// \return le nombre de byte par pixel de la texture
int              dktGetTextureBytePerPixel(unsigned int textureID);



/// \brief obtient la description de la dernière erreur encourue par ce module
///
/// Cette fonction retourne la description de la dernière erreur encourue par ce module
///
/// \return description de la dernière erreur encourue par ce module
char*            dktGetLastError();



/// \brief obtient le tableau des pixels qui définissent une texture déjà chargée
///
/// Cette fonction permet d'obtenir le tableau des pixels qui définissent une texture déjà chargée.
///
/// \param textureID identifiant unique de la texture cible
/// \param data pointeur qui contiendra l'adresse du tableau des pixels de la texture cible
void             dktGetTextureData(unsigned int textureID, unsigned char * data);



/// \brief obtient les dimensions d'une texture déjà chargée
///
/// Cette fonction permet d'obtenir les dimensions d'une texture déjà chargée.
///
/// \param textureID identifiant unique de la texture cible
/// \return dimension de la texture cible
CVector2i        dktGetTextureSize(unsigned int textureID);



/// n'est plus utilisé
void             dktInit();



/// \brief remplace une texture existante par le contenu d'une portion du framebuffer (la dernière image affichée à l'écran)
///
/// Cette fonction permet de capturer une partie de l'écran en la mettant dans une texture existante. Le rectangle de la capture de l'écran est défini en pixel par la position (x,y) et la dimension wXh (la position désignant le coin inférieur gauche du rectangle).
///
/// \param textureID identifiant unique de la texture
/// \param x coordonnée de la position du coin inférieur gauche du rectangle de capture
/// \param y coordonnée de la position du coin inférieur gauche du rectangle de capture
/// \param w dimension de la nouvelle texture (doit être une puissance de 2)
/// \param h dimension de la nouvelle texture (doit être une puissance de 2)
/// \param internalFormat drapeau du format de pixel à être utilisé pour la nouvelle texture
void             dktRenderToTexture(unsigned int textureID, int x, int y, int w, int h, unsigned int internalFormat);

/// \brief met à jour toutes les textures présentement chargées en mémoire
///
/// Cette fonction vérifie si des changements ont été apportés aux textures présentement chargée en mémoire et fait la mise à jour de leur contenu si nécessaire.
/// La vérification est faite pour une seule texture à la fois par appel. Chaque appel successif fera la vérification pour la texture suivante. Lorsqu'on atteint la dernière texture, on recommence à vérifier la première texture.
/// Ceci est particulièrement utile lorsque le module est utilisé par plus d'une application : on peut modifier la texture avec un éditeur et voir immédiatement le résultat dans le jeu par exemple.
///
void             dktUpdate();







// Les fonction du DKW


/// \brief envoie un signal de fermeture du programme et de la fenêtre
///
/// Cette fonction envoie un signal de fermeture du programme et de la fenêtre en générant un message système WM_QUIT
///
void            dkwForceQuit();


void            dkwSwap();




/// \brief obtient la description de la dernière erreur encourue par ce module
///
/// Cette fonction retourne la description de la dernière erreur encourue par ce module
///
/// \return description de la dernière erreur encourue par ce module
char*           dkwGetLastError();



/// \brief retourne la position de la souris
///
/// Cette fonction retourne la position de la souris en pixel par rapport au coin supérieur gauche de la fenêtre.
///
/// \return position de la souris
CVector2i       dkwGetCursorPos();



/// \brief retourne la résolution actuelle de la fenêtre
///
/// Cette fonction retourne la résolution actuelle de la fenêtre en pixel.
///
/// \return résolution actuelle de la fenêtre
CVector2i       dkwGetResolution();



/// \brief boucle principale du programme
///
/// Cette fonction est la boucle principale du programme. Elle ne doit être appelé qu'une fois pour toute la durée de l'exécution du programme. L'exécution de cette fonction ne se terminera que lorsque le message système WM_QUIT aura été reçu.
///
/// \return 0 si l'exécution s'est déroulée normalement, retourne 1 sinon
int             dkwMainLoop();

/// \brief vérifie si un message système a été reçu
///
/// Cette fonction vérifie si un message système a été reçu et le redirige vers la fonction 'callback' si c'est le cas. Cette fonction est utile si l'on veut garder un contact avec l'environnement d'exécution pendant un long cycle d'exécution de la boucle principale.
///
void            dkwUpdate();


/// \brief Enable/Disable mouse clipping
///
/// Cette fonction active ou desactive le clipping de la souris
///

void            dkwClipMouse( bool abEnabled );



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

#endif

