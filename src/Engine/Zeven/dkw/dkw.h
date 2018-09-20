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

/// \brief Module de gestion d'une fen�tre Windows
///
/// \file dkw.h
/// Ce module prend en charge la gestion d'une fen�tre Windows. Ceci comprend :
/// 	- une fonction d'initialisation du module et d'une fen�tre
/// 	- une fonction de terminaison d'utilisation du module
/// 	- des fonctions permettant d'obtenir diverses informations sur la fen�tre et sur le module
/// 	- une fonction de destruction de la fen�tre
/// 	- une fonction de traitement des messages syst�me dans une boucle (le mainloop du programme)
/// 	- une fonction de traitement des messages syst�me pouvant �tre appel� de l'int�rieur du programme
/// 	- une petite classe contenant 2 fonctions virtuel pure devant �tre d�finie qui devront �tre pass� au module et qui seront appel� par celui-ci aux moments opportuns
///
/// Je me doit ici d'expliquer le concept g�n�ral d'ex�cution du programme. Ce module utilise la programmation �v�nementielle pr�sente dans Windows pour g�rer les diff�rents message syst�me.
/// La fonction accomplissant cette t�che est une fonction 'callback' qui est appel� par le syst�me sur r�ception d'un message provenant de ce dernier. Selon le message re�u, une certaine action ou appel de fonction sera effectu�.
/// Mais avant d'appeler la fonction 'callback', on doit v�rifier si des �v�nements syst�mes sont g�n�r�s et si oui, les rediriger vers notre fonction 'callback'. Pour ce faire, on appel dkwMainLoop() qui verif�e constamment si un message syst�me a �t� re�u et qui le redirige vers notre 'callback' s'il y en a un. Cette v�rification s'arr�te lorsque le message syst�me WM_QUIT est re�u (ceci est un signal de fermeture de l'application).
/// Dans le cas o� le programme passe beaucoup de temps dans une fonction, il devient alors possible de rater des messages syst�mes. On peut alors utiliser dkwUpdate() qui effectue une v�rification et renvoie d'un message syst�me par appel.
/// Voici maintenant les diff�rents messages syst�mes g�r�s par le module:
/// 	- WM_CLOSE : message syst�me d�signant l'�tat de fermeture de la fen�tre. Suite � ce message, un message WM_QUIT sera g�n�r� par le module
/// 	- WM_PAINT : message syst�me d�signant qu'aucun autre message plus prioritaire est pr�sent et que l'on peut effectuer un cycle d'ex�cution du programme et le module appel paint() (paint() inclu habituellement un certain nombre d'appel successif � update() et un seul appel � render())
/// 	- WM_SIZE : message syst�me d�signant qu'il vient d'y avoir une modification de la taille de la fen�tre, le module prend en compte les changement automatiquement
/// 	- WM_CHAR : message syst�me d�signant qu'un caract�re vient d'�tre tapp� au clavier, le module utilisera la fonction textWrite() pour traiter le caract�re tapp�
/// 	- WM_KEYDOWN : message syst�me d�signant qu'une touche du clavier est appuy�e, ce message est ignor� �tant donn� la pr�sence de la librairie DKI qui g�re toutes les entr�es de clavier
/// 	- WM_KEYUP : message syst�me d�signant qu'une touche du clavier est relach�e, ce message est ignor� �tant donn� la pr�sence de la librairie DKI qui g�re toutes les entr�es de clavier
/// 	- WM_QUIT : message syst�me d�signant la fermeture du programme. Ce message n'est pas g�r� directement dans la fonction 'callback' mais plutot dans dkwMainLoop(). C'est lorsque ce que message est re�u que dkwMainLoop() retourne.
/// 
/// \author David St-Louis (alias Daivuk)
/// \author Louis Poirier (� des fins de documentation seulement)
///


#ifndef DKW_H
#define DKW_H


#include "platform_types.h"

#include "CVector.h"

#include <SDL2/SDL.h>



/// \brief Interface principale entre le module et le code externe
///
/// Cette classe est l'interface principale entre le module et le code externe. Elle ne contient que 2 fonctions virtuelles pures devant �tre red�finies dans une classe-d�riv�.
/// Ces 2 fonctions seront automatiquement appel�es par la fonction 'callback'. Une instance d'une classe d�riv�e de celle-ci devra �tre pass�e en param�tre � dkwInit() afin de g�n�rer une fen�tre Windows valide et active.
///
/// \note : Il est possible de ne pas d�river cette classe abstraite et de simplement passer 0 en param�tre � dkwInit(). Ceci permettra quand m�me � l'application de s'ex�cuter sans toutefois g�n�rer de rendu ni de mise � jour de la logique d'affaire.
class CMainLoopInterface
{
public:
	
	
	/// \brief fonction essentielle qui doit inclure l'aspect Vue et Logique d'affaire de l'application
	///
	/// Cette fonction doit �tre red�finie de fa�on � g�rer toutes les op�rations de rendu et toute la logique du jeu. Elle sera appel� automatiquement par la fonction 'callback' � chaque fois qu'un message syst�me WM_PAINT sera re�u. Cet action marquera le d�but d'un cycle d'ex�cution du programme.
	///
	virtual void paint() = 0;
	
	
	
	/// \brief fonction qui donne un moyen de g�rer les caract�res entr�e au clavier
	///
	/// Cette fonction doit �tre red�finie mais cette red�finition peut simplement �tre vide si un autre moyen a �t� utilis� pour g�rer les entr�es de caract�res (avec le module DKI par exemple)
	///
	/// \param caracter caract�re entr� au clavier pass� depuis la fonction 'callback' � l'appel
	virtual void textWrite(unsigned int caracter) = 0;
};


// Les fonction du DKW



/// \brief initialise le module et g�n�re une fen�tre Windows
///
/// Cette fonction initialise le module afin que son utilisation puisse d�buter. Cette fonction DOIT �tre appel� UNE SEULE FOIS pour toute la dur�e du programme et avant tout autres appels � des fonctions de ce module.
/// 
///
/// \param width dimension en pixel de la fen�tre souhait�e
/// \param height dimension en pixel de la fen�tre souhait�e
/// \param colorDepth nombre de bit utiliser pour chaque composant de couleur d'un pixel (16 ou 32.....donc 32)
/// \param title chaine de caract�res d�finissant le nom que portera la fen�tre cr��e
/// \param mMainLoopObject pointeur vers une classe d�riv�e de la classe CMainLoopInterface qui contient la d�finition de la fonction principale du programme (Paint())
/// \param fullScreen bool�en qui d�termine si la fen�tre doit �tre en mode plein �cran(true) ou en mode fen�tre(false)
/// \return int retourne 1 si la cr�ation de la fen�tre s'est bien d�roul�, retourne 0 si une erreur s'est produite
int				dkwInit(int width, int height, int colorDepth, char *title, CMainLoopInterface *mMainLoopObject, bool fullScreen, int refreshRate = -1);



/// \brief envoie un signal de fermeture du programme et de la fen�tre
///
/// Cette fonction envoie un signal de fermeture du programme et de la fen�tre en g�n�rant un message syst�me WM_QUIT
///
void			dkwForceQuit();



SDL_GLContext dkwGetDC();
SDL_Window* dkwGetHandle();



/// \brief obtient la description de la derni�re erreur encourue par ce module
///
/// Cette fonction retourne la description de la derni�re erreur encourue par ce module
///
/// \return description de la derni�re erreur encourue par ce module
char*			dkwGetLastError();



/// \brief retourne la position de la souris
///
/// Cette fonction retourne la position de la souris en pixel par rapport au coin sup�rieur gauche de la fen�tre.
///
/// \return position de la souris
CVector2i		dkwGetCursorPos();



/// \brief retourne la r�solution actuelle de la fen�tre
///
/// Cette fonction retourne la r�solution actuelle de la fen�tre en pixel.
///
/// \return r�solution actuelle de la fen�tre
CVector2i		dkwGetResolution();



/// \brief boucle principale du programme
///
/// Cette fonction est la boucle principale du programme. Elle ne doit �tre appel� qu'une fois pour toute la dur�e de l'ex�cution du programme. L'ex�cution de cette fonction ne se terminera que lorsque le message syst�me WM_QUIT aura �t� re�u.
///
/// \return 0 si l'ex�cution s'est d�roul�e normalement, retourne 1 sinon
int				dkwMainLoop();



/// \brief terminaison d'utilisation du module
///
/// Cette fonction termine l'utilisation du module et lib�re la m�moire allou�e pour la gestion des messages d'erreurs.
///
void			dkwShutDown();



/// \brief v�rifie si un message syst�me a �t� re�u
///
/// Cette fonction v�rifie si un message syst�me a �t� re�u et le redirige vers la fonction 'callback' si c'est le cas. Cette fonction est utile si l'on veut garder un contact avec l'environnement d'ex�cution pendant un long cycle d'ex�cution de la boucle principale.
///
void			dkwUpdate();


/// \brief Enable/Disable mouse clipping
///
/// Cette fonction active ou desactive le clipping de la souris
///

void			dkwClipMouse( bool abEnabled );


#endif
