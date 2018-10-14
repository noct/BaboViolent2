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
#ifndef BABOUI_H
#define BABOUI_H

#include <Zeven/Gfx.h>
#include <vector>
#include "Writting.h"

#define NO_VALIDATION 0
#define VALIDATE_INT 1
#define VALIDATE_FLOAT 2

enum control_state
{
    CONTROL_NOTHING,
    CONTROL_OVER,
    CONTROL_DOWN
};

enum control_justify
{
    JUSTIFY_LEFT,
    JUSTIFY_CENTER,
    JUSTIFY_RIGHT
};

class Control
{
protected:
    // Caption
    CString m_caption;

    // Son état
    control_state m_state;

    // Sa font
    unsigned int m_font;

    // Si on vient de le cliquer
    float m_clicDelay;

    // La grosseur de la font
    float m_textSize;

    // Son pourcent d'arrivé
    float m_leftOffset;

    float anim;

    // la position du text par rapport à sa screenPosition
    control_justify m_justify;

    // Le son pour quand on passe au dessus
    FSOUND_SAMPLE * m_sfxOver;

    // On ne gère pas les clic sur lui
    bool m_isMouseHandle;

    // La texture
    unsigned int tex_menu;

public:
    // Son RECT (avec la caption)
    CVector4f m_rect;

    // Constructeur
    Control(CVector2i screenPosition, CString caption, unsigned int m_font, float textSize, control_justify justify);

    // Sa position sur l'écran
    CVector2i m_screenPosition;

    // Destructeur
    virtual ~Control();

    // L'updater le gèrer
    virtual void update(float delay);

    // Pour quand on clic dessus, le child saura quoi faire
    virtual void onClic() {}
    virtual void onRightClic() {}

    // Pour changer la caption
    void changeCaption(CString caption);

    // gets
    CString getCaption() {return m_caption;}

    // Pour l'afficher
    virtual void render();
};

class ControlListener
{
protected:
    // Son array de control
    std::vector<Control *> m_controls;

public:
    // Constructeur
    ControlListener();

    // Destructeur
    virtual ~ControlListener();

    // pour ajouter un control
    void add(Control * control);

    // Si on clic sur un bouton
    virtual void onClick(Control * control) {}

    // Pour afficher
    void renderMenu();
    virtual void renderUnique() {}

    // pour updater
    void updateMenu(float delay);
    virtual void updateUnique(float delay) {}
};

class Button : public Control
{
private:
    // Le menu qui écoute si on pèse sur un bouton
    ControlListener * m_listener;

    // Le son pour quand on clic
    FSOUND_SAMPLE * m_sfxClic;

public:
    // Constructeur
    Button(CVector2i screenPosition, CString caption, unsigned int m_font, float textSize, control_justify justify, ControlListener* listener);

    // Destructeur
    virtual ~Button();

    // Pour quand on clic dessus, le child saura quoi faire
    void onClic();
};

class Choice : public Control
{
private:
    // Le son pour quand on clic
    FSOUND_SAMPLE * m_sfxClic;

    // Nos choix
    std::vector<CString> m_choices;

    // Notre choix courant
    int m_choice;

public:
    // Constructeur
    Choice(CVector2i screenPosition, CString caption, unsigned int m_font, float textSize, control_justify justify);

    // Destructeur
    virtual ~Choice();

    // Pour quand on clic dessus, le child saura quoi faire
    void onClic();
    void onRightClic();

    // Pour ajouter un choix
    void addChoice(CString choice);

    // gets
    CString getChoice() {return (m_choices.size())?m_choices[m_choice]:CString();}
    int getChoiceIndex() {return m_choice;}

    // sets
    void setChoice(int choice);
};

class Key : public Control
{
private:
    // La valeur de la touche
    int m_keyValue;

    // Si on est en attente de presser une touche !
    bool m_isWaitingForKey;

    // Le son pour quand on clic
    FSOUND_SAMPLE * m_sfxClic;

public:
    // Constructeur
    Key(CVector2i screenPosition, CString caption, unsigned int m_font, float textSize, control_justify justify);

    // Destructeur
    virtual ~Key();

    // sets
    void setKeyValue(int keyValue);

    // gets
    int getKeyValue() {return m_keyValue;}

    // L'updater le gèrer
    virtual void update(float delay);

    // Pour quand on clic dessus, il faut activer le wait for key hehe (SICK)
    void onClic();
};

class Label : public Control
{
public:
    // Constructeur
    Label(CVector2i screenPosition, CString caption, unsigned int m_font, float textSize, control_justify justify);

    // Affichage
    void render();
};

class Write : public Control, public Writting
{
private:
    // Le son pour quand on clic
    FSOUND_SAMPLE * m_sfxClic;

    int validationType;

public:
    // Constructeur
    Write(CVector2i screenPosition, CString caption, unsigned int m_font, float textSize, control_justify justify, int flag = NO_VALIDATION);

    // Destructeur
    virtual ~Write();

    // L'updater le gèrer
    virtual void update(float delay);

    // Pour quand on clic dessus, le child saura quoi faire
    void onClic();

    // Pour l'afficher
    virtual void render();
};

#endif
