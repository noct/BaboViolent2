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

/* RndLabs inc. (c) All rights reserved */



#ifndef CSTRING_H
#define CSTRING_H



/// \name Constantes
/// Nombre maximum de caractêes qu'un string peut contenir
//E.P Uhhhmmm... pas sr que j'aime ê...
#define MAX_CARAC 512


#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#ifndef WIN32
    #include "LinuxHeader.h"
#endif


////////////////////////////////////////////////////////////////////////////////////////
/// \brief Classe utile pour gêer des chaêes de caractêes
///
/// Elle permet de créer r des objets contenant des fonctions de recherche de caractêes,
/// d'insertion, de remplacement, bref toutes les opêations qui peuvent êre utile lors
/// de la manipulation de chaêes de caractêes.
/// Classe dêivê de CAttribute.
////////////////////////////////////////////////////////////////////////////////////////
class CString
{
public:
    /// Pointeur qui contient notre chaêe de caractêe
    char* s;

    // Constructeurs

    ///Constructeur par dêaut. La string est initialisê avec un seul caractêe, le caractêe nul.
    CString(){s = new char[1]; s[0] = '\0';}
    CString(char* fmt, ...);

    /// Constructeur copie
    CString(const CString & objToCopy){s = new char[strlen(objToCopy.s)+1]; strcpy(s, objToCopy.s);}

    /// Destructeur
    virtual ~CString(){delete [] s;}

    // Vêifie l'extension d'une chaêe de caractêe
    bool checkExtension(char * extension);

    // Remplis la chaêe de caractêe par le chemin de l'application
    void fillWithAppPath();

    // On check si on a le string voulu inclu dedans
    bool find(CString string);
    bool find(char* string);
    bool find(CString string, char* strFound);
    bool find(char* string, char* strFound);
    bool find(CString string, int & index);
    bool find(char* string, int & index);
    bool find(CString string, char* strFound, int & index);
    bool find(char* string, char* strFound, int & index);

    ///Obtenir le nom du fichier d'un chemin complet
    CString getFilename();

    // Prendre les tokens
    CString getFirstToken(int caracterSeparator);
    CString getNextToken(int caracterSeparator);

    ///Obtenir le path du fichier (sans le fichier lui-mêe)
    CString getPath();

    ///Obtenir le pointeur this (HEU??? sert êquoi???)
    CString * getThis(){return this;}

    ///Obtenir le type de l'objet
    CString getType(){return "CString";}

    // Insêer un string êune certaine position
    void insert(CString string, int index);
    void insert(char *, int index);

    ///Retourne vrai si la string est vide
    bool isNull(){return (s[0] == '\0') ? true : false;}

    //  int len(){int len_=0; while(s[len_++]); return len_-1;}  // On peut trê bien le faire nous mêe
    /// Grandeur de la string
    int len() const {return (int)strlen(s);} // Probablement que strlen() est plus optimisêen _asm

    // Pour retirer un caractêe
    void remove(int index);

    /// Pour remplacer un caractêe par un autre dans tout le string
    void replace(char toReplace, char by){int len_=len(); for(int i=0;i<len_;i++) if(s[i] == toReplace) s[i]=by;}

    ///Vide la chaêe
    void reset(){delete [] s; s = new char[1]; s[0] = '\0';}

    // Changer ses dimensions
    void resize(int newSize);
    void resizeInverse(int newSize);

    // Lui setter une nouvelle valeur
    void set(char* fmt, ...);

    // Pour le loader d'un fichier
    void loadFromFile(FILE *fic);

    /// Convertir en float
    float toFloat(){/*float tmp;sscanf(s, "%f", &tmp);*/return (float)atof(s)/*tmp*/;}
    ///Convertir en int
    int toInt(){/*int tmp;sscanf(s, "%i", &tmp);*/return atoi(s)/* tmp*/;}

    /// Mettre la chaêe en minuscule
    void toLower(){int len_=len(); for(int i=0;i<len_;i++) if(s[i]>='A' && s[i]<='Z') s[i]+=32;}

    ///Mettre la chaêe en majuscule
    void toUpper(){int len_=len(); for(int i=0;i<len_;i++) if(s[i]>='a' && s[i]<='z') s[i]-=32;}

    // Pour trimer
    void trim(char caracter);



    /// Accêer êun caractêe du string
    char& operator[](const int i) const {return (i>=0 && i<len()) ? s[i] : ((i<0) ? s[0] : s[len()-1]);}

    ///Copier êpartir d'un pointeur
    void operator=(const char* string)
    {
        if(this->s != string)
        {
            delete [] s;
            s = new char[strlen(string)+1];
            strcpy(s, string);
        }
    }
    ///Copier êpartir de l'adresse de l'objet
    void operator=(const CString &objToCopy)
    {
        if(this != &objToCopy)
        {
            delete [] s;
            s = new char[strlen(objToCopy.s)+1];
            strcpy(s, objToCopy.s);
        }
    }
    ///Copier êpartir d'un int
    void operator=(int value){set("%i", value);}
    ///Copier êpartir d'un flaot
    void operator=(float value){set("%f", value);}

    ///Concatêation êpartir de l'adresse de la string (retourne une string)
    CString operator+(const CString& string){return CString("%s%s", s, string.s);}
    ///Concatêation êpartir de l'adresse de la string (en affectant la valeur êla string courante)
    void operator+=(const CString &string){set("%s%s", s, string.s);}
    ///Concatêation êpartir d'un pointeur sur une string (retourne une string)
    CString operator+(char* string){return CString("%s%s", s, string);}
    ///Concatêation êpartir d'un pointeur sur une string (en affectant la valeur êla string courante)
    void operator+=(char* string){set("%s%s", s, string);}
    ///Concatêation êpartir d'un int (retourne une string)
    CString operator+(int value){return CString("%s%i", s, value);}
    ///Concatêation êpartir d'un int sur une string (en affectant la valeur êla string courante)
    void operator+=(int value){set("%s%i", s, value);}
    ///Concatêation êpartir d'un float (retourne une string)
    CString operator+(float value){return CString("%s%f", s, value);}
    ///Concatêation êpartir d'un float sur une string (en affectant la valeur êla string courante)
    void operator+=(float value){set("%s%f", s, value);}

    ///Retourne vrai si les deux string sont identiques
    bool operator==(const CString &string){return (stricmp(s, string.s)==0);}
    ///Retourne vrai si les deux string sont diffêentes
    bool operator!=(const CString &string){return !(stricmp(s, string.s)==0);}
    ///Retourne vrai si les deux string sont identiques
    bool operator==(const char* string){return (stricmp(s, string)==0);}
    ///Retourne vrai si les deux string sont diffêentes
    bool operator!=(const char* string){return !(stricmp(s, string)==0);}
    ///Retourne vrai si la string est plus petite
    bool operator<(const CString& string)const{return (stricmp(s, string.s)<0);}
    ///Retourne vrai si la string est plus grande
    bool operator>(const CString& string)const{return (stricmp(s, string.s)>0);}
    ///Retourne vrai si la string est plus petite ou êale
    bool operator<=(const CString& string){return (stricmp(s, string.s)<=0);}
    ///Retourne vrai si la string est plus grande ou êale
    bool operator>=(const CString& string){return (stricmp(s, string.s)>=0);}
};

/*
CString operator+(const char * string, const CString& string2);
CString operator+(const char * string, int & value);
CString operator+(const char * string, float & value);
CString operator+(const char * string1, const char* string2);
*/

#endif


