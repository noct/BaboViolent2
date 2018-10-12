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




#include <Zeven/CString.h>
//#include <windows.h>



//
// Constructeurs
//

////////////////////////////////////////////////////////////////////////////////////////
/// \brief Constructeur utilisêcomme pour un printf
///
/// Ce constructeur fonctionne de la mêe maniêe qu'un printf avec un nombre inconnu
/// d'arguments. Les arguments sont passê tour êtour et intêrê êla string...
///
////////////////////////////////////////////////////////////////////////////////////////
CString::CString(char* fmt, ...)
{
    static char mString[MAX_CARAC];


    if (!fmt)
    {
        s = new char [1];
        s[0] = '\0';
        return;
    }

    // On créer un char de 512 pour contenir le string (pas besoin de plus que 512 je crois)
//  char *mString = new char[MAX_CARAC];

    // Ici on passe tout les param (c comme un printf) pour les mettre dans le string
    va_list     ap;
    va_start(ap, fmt);
#ifdef WIN32
    _vsnprintf(mString, sizeof(mString), fmt, ap);
#else
    vsnprintf(mString, sizeof(mString), fmt, ap);
#endif
    va_end(ap);

    // avoid buffer overrun
    mString[MAX_CARAC - 1] = 0;
    // On lui transfert la valeur finale
    s = new char [strlen(mString)+1];
    strcpy(s, mString);

    //delete [] mString;
}



////////////////////////////////////////////////////////////////////////////////////////
/// \brief Fonction qui permet de modifier la taille de la chaêe de caractêe
///
/// \param newSize : Nouvelle taille.
///
/// \see
/// http://www.cplusplus.com/ref/cstdio/sprintf.html
////////////////////////////////////////////////////////////////////////////////////////
void CString::resize(int newSize)
{
    if (newSize >= len()) return;
    if (newSize > 0)
    {
        char tmp[10];
        sprintf(tmp, "%%.%is", newSize);
        CString newStr(tmp, s);
        set("%s", newStr.s);
    }
    else
    {
        // On le remet êvide
        delete [] s;
        s = new char [1];
        s[0] = '\0';
    }
}

////////////////////////////////////////////////////////////////////////////////////////
/// \brief Fonction qui permet de modifier la taille de la chaêe de caractêe et qui en
/// plus l'inverse
///
/// \param newSize : Nouvelle taille.
///
/// \return Aucun
///
////////////////////////////////////////////////////////////////////////////////////////
void CString::resizeInverse(int newSize){

    if (newSize > 0)
    {
        // On le resize
        int len_ = len();
        if (len_-newSize > 0)
        {
            CString newStr("%s", &(s[len_-newSize]));
            set("%s", newStr.s);
        }
    }
    else
    {
        // On le remet êvide
        delete [] s;
        s = new char [1];
        s[0] = '\0';
    }
}



////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Fonction qui permet d'assigner une valeur êune chaêe existante de la mêe faên
/// qu'un printf
///
/// La fonction supprime d'abbord la chaêe existante et en créer  une nouvelle avec la valeur
/// dêirê
////////////////////////////////////////////////////////////////////////////////////////////
void CString::set(char* fmt, ...){

    if (!fmt)
    {
        delete [] s;
        s = new char [1];
        s[0] = '\0';
        return;
    }

    // On créer un char de 256 pour contenir le string (pas besoin de plus que 256 je crois)
    char *mString = new char[MAX_CARAC];

    // Ici on passe tout les param (c comme un printf) pour les mettre dans le string
    va_list     ap;
    va_start(ap, fmt);
        vsprintf(mString, fmt, ap);
    va_end(ap);

    // On lui transfert la valeur finale
    delete [] s;
    s = new char [strlen(mString)+1];
    strcpy(s, mString);

    delete [] mString;
}



////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Vêifie l'extension d'une chaêe de caractêe
///
/// Fonction qui permet de dêerminer si la chaêe de caractêe possêe l'extension
/// envoyê en paramêre
///
/// \param extension : Extension qu'on veut vêifier.
///
/// \return True : si l'extension est prêente\n
///         False : si l'extension n'est pas prêente
////////////////////////////////////////////////////////////////////////////////////////////
bool CString::checkExtension(char * extension)
{
    // On vêifie que le string est bon
    if (!extension)
    {
        return false;
    }

    // On check le nb de caractêe de notre extension
    int extensionLen = (int)strlen(extension);

    if (extensionLen > 0 && len() >= extensionLen)
    {
        //on compare sans tenir compte de majuscule/minuscule
        return (stricmp(&(s[len()-extensionLen]), extension) == 0);
    }
    else
    {
        return false;
    }
}



/////////////////////////////////////////////////////////////////////////
///Obtenir le path du fichier (sans le fichier lui-mêe)
/////////////////////////////////////////////////////////////////////////
CString CString::getPath()
{
    // On va chercher le dernier '\' ou '/' trouvê
    for (int i=len()-1;i>=0;i--)
    {
        if (s[i] == '\\' || s[i] == '/')
        {
            char tmp[10];
            sprintf(tmp, "%%.%is", i+1);
            return CString(tmp, s);
        }
    }

    return CString();
}



//////////////////////////////////////////////////////////////
///Obtenir le nom du fichier d'un chemin complet
//////////////////////////////////////////////////////////////
CString CString::getFilename()
{

    // On va chercher le dernier '\' ou '/' trouvê
    for (int i=len()-1;i>=0;i--)
    {
        if (s[i] == '\\' || s[i] == '/')
        {
            char tmp[10];
            sprintf(tmp, "%%.%is", len()-(i+1));
            return CString(tmp, &(s[i+1]));
        }
    }

    return CString();
}



////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Enlever des caractêes au dêut et êla fin de la chaêe
///
/// Cette fonction supprime le caractêe recu en paramêre au dêut et êla fin de la string
///
/// \param caracter : Caractêe êtrimmer
////////////////////////////////////////////////////////////////////////////////////////////
void CString::trim(char caracter)
{

    // En enlêe les caractêes au dêut
    int len_ = len();
    int i=0;
    for (i=0;i<len_;i++)
    {
        if (s[i] != caracter)
        {
            break;
        }
    }
    CString newString("%s", &(s[i]));

    // les dernier caractêe maintenant
    for (i=newString.len()-1;i>=0;i--)
    {
        if (newString.s[i] != caracter)
        {
            break;
        }
    }

    char tmp[10];
    sprintf(tmp, "%%.%is", i+1);
    CString newString2(tmp, newString.s);

    set("%s", newString2.s);
}



////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Permet de prendre le token suivant
///
/// Cette fonction retourne les caractêes se trouvant êla droite du caractêe de sêaration
/// reê en paramêre
///
/// \param caracter : Caractêe servant de sêaration
////////////////////////////////////////////////////////////////////////////////////////////
CString CString::getNextToken(int caracter)
{

    // On commence pa r trimmer le tout au cas
    trim((char)caracter);

    CString result;

    // Maintenant on pogne le dernier mot
    // On va chercher le dernier ' ' trouvê
    for (int i = len()-1; i >= -1; i--)
    {
        if (s[i] == caracter || i == -1)
        {
            result = &(s[i+1]);
            resize(i+1);
            break;
        }
    }

    // On trim
    trim((char)caracter);

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Permet de prendre le token êpartir du dêut
///
/// Cette fonction retourne les caractêes se trouvant êla gauche du caractêe de sêaration
/// reê en paramêre. Cela peut êre pratique si nous voulons faire un switch sur la command
/// avant tout et passer le reste du string.
///
/// \param caracter : Caractêe servant de sêaration
////////////////////////////////////////////////////////////////////////////////////////////
CString CString::getFirstToken(int caracterSeparator)
{

    // On commence par trimmer le tout au cas
    trim((char)caracterSeparator);

    CString result;

    // Maintenant on pogne le dernier mot
    // On va chercher le dernier ' ' trouvê
    int len_ = len();
    for (int i=0;i<=len_;i++){
        if (s[i] == caracterSeparator || s[i] == '\0'){
            char tmp[10];
            sprintf(tmp, "%%.%is", i);
            result = CString(tmp, s);
            resizeInverse(len_-i);
            break;
        }
    }

    // On trim
    trim((char)caracterSeparator);

    return result;
}



////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Recherche de caractêes
///
/// Cette fonction utilise une chaêe de caractêe reêe en paramêre et essaie de
/// la trouver dans la chaêe (s). (Seule la premiêe occurence de la chaêe est trouvê)
///
/// \param string : Chaêe de caractêe êtrouver.
///
/// \return True : si la chaêe est trouvê
///         False : si la chaêe n'est pas trouvê
///
/// \note Algorithmes plus performants existent si nêessaire.
////////////////////////////////////////////////////////////////////////////////////////////
bool CString::find(CString string)
{

    //longueur de notre chaêe (s)
    int len_1 = len();
    //longueur de la chaêe qu'on veut trouver
    int len_2 = string.len();

    //on parcours la chaêe pour essayer de trouver la sub-string
    for (int i=0; i < (len_1 - len_2 + 1); i++)
    {
        if (strnicmp(&(s[i]), string.s, len_2) == 0)
        {
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Recherche de caractêes avec pointeur sur la string trouvê
///
/// Cette fonction utilise une chaêe de caractêe reêe en paramêre et essaie de
/// la trouver dans la chaêe (s). La fonction recoit aussi un pointeur en paramêre qui
/// va pointer au dêut de la chaêe trouvê. (Seule la premiêe occurence de la chaêe est trouvê)
///
/// \param  string : Chaêe de caractêe êtrouver.
/// \param  strFound : Pointeur vers le bêut de la chaêe si elle est trouvê
///
/// \return True : si la chaêe est trouvê
///         False : si la chaêe n'est pas trouvê
///
/// \note Algorithmes plus performants existent si nêessaire.
////////////////////////////////////////////////////////////////////////////////////////////
bool CString::find(CString string, char* strFound)
{

    int len_1 = len();
    int len_2 = string.len();

    for (int i=0; i < (len_1 - len_2 + 1) ;i++)
    {
        if (strnicmp(&(s[i]), string.s, len_2) == 0)
        {
            strFound = &(s[i]);
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Recherche de caractêes avec position de la string trouvê
///
/// Cette fonction utilise une chaêe de caractêe reêe en paramêre et essaie de
/// la trouver dans la chaêe (s). La position de la chaêe est aussi retournê
/// (Seule la premiêe occurence de la chaêe est trouvê.)
///
/// \param  string : Chaêe de caractêe êtrouver.
/// \param  index : Position de la chaêe si elle est trouvê.
///
/// \return True : si la chaêe est trouvê
///         False : si la chaêe n'est pas trouvê
///
/// \note Algorithmes plus performants existent si nêessaire.
////////////////////////////////////////////////////////////////////////////////////////////
bool CString::find(CString string, int & index)
{

    int len_1 = len();
    int len_2 = string.len();

    for (int i=0; i < (len_1 - len_2 + 1); i++)
    {
        if (strnicmp(&(s[i]), string.s, len_2) == 0)
        {
            index = i;
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Recherche de caractêes avec pointeur et position de la string trouvê
///
/// Cette fonction utilise une chaêe de caractêe reêe en paramêre et essaie de
/// la trouver dans la chaêe (s). La position de la chaêe est aussi retournê
/// (Seule la premiêe occurence de la chaêe est trouvê.)
///
/// \param  string : Chaêe de caractêe êtrouver.
/// \param  strFound : Pointeur vers le bêut de la chaêe si elle est trouvê
/// \param  index : Position de la chaêe si elle est trouvê.
///
/// \return True : si la chaêe est trouvê
///         False : si la chaêe n'est pas trouvê
///
/// \note Algorithmes plus performants existent si nêessaire.
////////////////////////////////////////////////////////////////////////////////////////////
bool CString::find(CString string, char* strFound, int & index)
{

    int len_1 = len();
    int len_2 = string.len();

    for (int i=0;i < (len_1 - len_2 + 1); i++)
    {
        if (strnicmp(&(s[i]), string.s, len_2) == 0)
        {
            index = i;
            strFound = &(s[i]);
            return true;
        }
    }

    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Recherche de caractêes
///
/// Cette fonction utilise une chaêe de caractêe reêe en paramêre et essaie de
/// la trouver dans la chaêe (s). (Seule la premiêe occurence de la chaêe est trouvê)
///
/// \param string_ : Chaêe de caractêe êtrouver.
///
/// \return True : si la chaêe est trouvê
///         False : si la chaêe n'est pas trouvê
///
/// \note Algorithmes plus performants existent si nêessaire.
////////////////////////////////////////////////////////////////////////////////////////////
bool CString::find(char* string_)
{
    return find(CString("%s", string_)); // On recall l'autre
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Recherche de caractêes avec pointeur sur la string trouvê
///
/// Cette fonction utilise une chaêe de caractêe reêe en paramêre et essaie de
/// la trouver dans la chaêe (s). La fonction recoit aussi un pointeur en paramêre qui
/// va pointer au dêut de la chaêe trouvê. (Seule la premiêe occurence de la chaêe est trouvê)
///
/// \param  string_ : Chaêe de caractêe êtrouver.
/// \param  strFound : Pointeur vers le bêut de la chaêe si elle est trouvê
///
/// \return True : si la chaêe est trouvê
///         False : si la chaêe n'est pas trouvê
///
/// \note Algorithmes plus performants existent si nêessaire.
////////////////////////////////////////////////////////////////////////////////////////////
bool CString::find(char* string_, char* strFound)
{
    return find(CString("%s", string_), strFound); // On recall l'autre
}


////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Recherche de caractêes avec position de la string trouvê
///
/// Cette fonction utilise une chaêe de caractêe reêe en paramêre et essaie de
/// la trouver dans la chaêe (s). La position de la chaêe est aussi retournê
/// (Seule la premiêe occurence de la chaêe est trouvê.)
///
/// \param  string_ : Chaêe de caractêe êtrouver.
/// \param  index : Position de la chaêe si elle est trouvê.
///
/// \return True : si la chaêe est trouvê
///         False : si la chaêe n'est pas trouvê
///
/// \note Algorithmes plus performants existent si nêessaire.
////////////////////////////////////////////////////////////////////////////////////////////
bool CString::find(char* string_, int & index)
{
    return find(CString("%s", string_), index); // On recall l'autre
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Recherche de caractêes avec pointeur et position de la string trouvê
///
/// Cette fonction utilise une chaêe de caractêe reêe en paramêre et essaie de
/// la trouver dans la chaêe (s). La position de la chaêe est aussi retournê
/// (Seule la premiêe occurence de la chaêe est trouvê.)
///
/// \param  string_ : Chaêe de caractêe êtrouver.
/// \param  strFound : Pointeur vers le bêut de la chaêe si elle est trouvê
/// \param  index : Position de la chaêe si elle est trouvê.
///
/// \return True : si la chaêe est trouvê
///         False : si la chaêe n'est pas trouvê
///
/// \note Algorithmes plus performants existent si nêessaire.
////////////////////////////////////////////////////////////////////////////////////////////
bool CString::find(char* string_, char* strFound, int & index)
{
    return find(CString("%s", string_), strFound, index); // On recall l'autre
}



////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Insertion de texte
///
/// Cette fonction insert le texte reê en paramêre êla position dêerminê par le 2e paramêre
///
/// \param  string : Chaêe de caractêe êinsêer.
/// \param  index : Position de l'insertion dans la chaêe de base.
////////////////////////////////////////////////////////////////////////////////////////////
void CString::insert(CString string, int index){

    char tmp[10];
    sprintf(tmp, "%%.%is%%s%%s", index);
    CString newString(tmp, s, string.s, &(s[index]));
    set("%s", newString.s);
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Insertion de texte (avec pointeur)
///
/// Cette fonction insert le texte reê en paramêre êla position dêerminê par le 2e paramêre
///
/// \param  string : Chaêe de caractêe êinsêer.
/// \param  index : Position de l'insertion dans la chaêe de base.
////////////////////////////////////////////////////////////////////////////////////////////
void CString::insert(char *string, int index){
    insert(CString("%s", string), index);
}



////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Suppression de texte
///
/// Cette fonction supprime le texte de la chaêe de base situêaprê la position reê en paramêre
///
/// \param  index : Position de la suppression dans la chaêe de base.
////////////////////////////////////////////////////////////////////////////////////////////
void CString::remove(int index){

    char tmp[10];
    sprintf(tmp, "%%.%is%%s", index);
    CString newString(tmp, s, &(s[index+1]));
    set("%s", newString.s);
}



////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Remplis la chaêe de caractêe par le chemin de l'application
///
/// La fonction trouve le chemin (path) de l'application et l'affecte êla chaêe
////////////////////////////////////////////////////////////////////////////////////////////
void CString::fillWithAppPath()
{

    char *appPath = new char[MAX_CARAC]; //E.P Utile?
    appPath[0] = '\0';                   //E.P Utile?
    set("%s", appPath);                      //E.P Utile?

    CString result = getPath();
    set("%s", result.s);

    delete [] appPath;                  //E.P Utile?
}


////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Utilisation d'un fichier pour setter le texte
///
/// Cette fonction load le texte êpartir du fichier et l'utilise pour setter la chaêe
///
/// \param  fic : fichier
////////////////////////////////////////////////////////////////////////////////////////////
void CString::loadFromFile(FILE *fic)
{
    char tmp[MAX_CARAC] = {0};

    for (int i=0;i<MAX_CARAC;i++)
    {
        fread(&(tmp[i]), 1, sizeof(char), fic);
        if (tmp[i] == 0) break;
    }

    set("%s", tmp);
}



//
// Opêateur sur des char*
//
/*
CString operator+(const char * string, const CString& string2)
{
    return CString("%s%s", string, string2.s);
}

CString operator+(const char * string, int & value)
{
    return CString("%s%i", string, value);
}

CString operator+(const char * string, float & value)
{
    return CString("%s%f", string, value);
}

CString operator+(const char * string1, char* string2)
{
    return CString("%s%s", string1, string2);
}*/


