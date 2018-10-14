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
#include "BaboUI.h"
#include "KeyManager.h"
#include "Helper.h"
#include <glad/glad.h>

//
// Constructeur
//
Control::Control(CVector2i screenPosition, CString caption, unsigned int font, float textSize, control_justify justify)
{
    m_isMouseHandle = true;
    m_screenPosition = screenPosition;
    m_leftOffset = 0;//-(float)m_screenPosition[0] - (float)m_screenPosition[1];
    m_font = font;
    m_textSize = textSize;
    m_justify = justify;
    m_state = CONTROL_NOTHING;
    m_clicDelay = 0;
    anim = 0;
    changeCaption(caption);
    m_sfxOver = dksCreateSoundFromFile("main/sounds/ControlOver.wav", false);
    tex_menu = dktCreateTextureFromFile("main/textures/Menu.png", DKT_FILTER_LINEAR);
}



//
// Destructeur
//
Control::~Control()
{
    dksDeleteSound(m_sfxOver);
    dktDeleteTexture(&tex_menu);
}



//
// L'updater le gèrer
//
void Control::update(float delay)
{
    anim += delay * rand(.5f, 2.0f);
    while (anim >= PI*2) anim -= PI*2;

    if (m_leftOffset < 0)
    {
        m_leftOffset += delay * fabsf(m_leftOffset) * 4;
        if (m_leftOffset > 0) m_leftOffset = 0;
    }

    if (m_isMouseHandle)
    {
        if (m_clicDelay > 0)
        {
            m_clicDelay -= delay;
            if (m_clicDelay < 0)
            {
                m_clicDelay = 0;
                m_state = CONTROL_NOTHING;
            }
        }

        // On check si la souri est dans notre Rectangle
        CVector2i mousePos = dkwGetCursorPos_main();
        CVector2i res = dkwGetResolution();

        // Il faut maintenant ajuster la mousePos dans notre 800x600
        mousePos[0] = (int)((float)mousePos[0] / (float)res[0] * 800.0f);
        mousePos[1] = (int)((float)mousePos[1] / (float)res[1] * 600.0f);

        // On test maintenant avec le rectange du bouton
        if (mousePos[0] >= (int)m_rect[0]-10 &&
            mousePos[0] <= (int)m_rect[2]+10 &&
            mousePos[1] >= (int)m_rect[1] &&
            mousePos[1] <= (int)m_rect[3])
        {
            // Si on n'avait pas de state, on tombe over
            if (m_state == CONTROL_NOTHING)
            {
                dksPlaySound(m_sfxOver, -1, 150);
                m_state = CONTROL_OVER;
            }
            if (dkiGetState(DKI_MOUSE_BUTTON1) == DKI_DOWN)
            {
                m_state = CONTROL_DOWN;
                m_clicDelay = .1f;
                onClic();
            }
            else if (dkiGetState(DKI_MOUSE_BUTTON2) == DKI_DOWN)
            {
                m_state = CONTROL_DOWN;
                m_clicDelay = .1f;
                onRightClic();
            }
        }
        else if (m_state == CONTROL_OVER)
        {
            m_state = CONTROL_NOTHING;
        }
    }
}



//
// Pour l'afficher
//
void Control::render()
{
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        // Le contour
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex_menu);
        float offset = 0;
        switch (m_state)
        {
        case CONTROL_DOWN:
        case CONTROL_NOTHING:
            glColor3f(.95f,.3f,.25f);
            break;
        case CONTROL_OVER:
            glColor3f(.95f,1,.25f);
            offset = 8;
            break;
        }
        glBegin(GL_QUAD_STRIP);
            glColor3f(sinf(anim)*.25f + .75f, .5f, .5f);
            glTexCoord2f(0,1);
            glVertex2f(m_rect[0]+m_leftOffset-m_textSize*.5f-offset,m_rect[1]);

            glTexCoord2f(0,0);
            glColor3f(sinf(anim+1)*.25f + .75f, .5f, .5f);
            glVertex2f(m_rect[0]+m_leftOffset-m_textSize*.5f-offset,m_rect[3]);

            glTexCoord2f(.5f,1);
            glColor3f(sinf(anim+2)*.25f + .75f, .5f, .5f);
            glVertex2f(m_rect[0]+m_leftOffset-offset,m_rect[1]);

            glTexCoord2f(.5f,0);
            glColor3f(sinf(anim+3)*.25f + .75f, .5f, .5f);
            glVertex2f(m_rect[0]+m_leftOffset-offset,m_rect[3]);

            glTexCoord2f(.5f,1);
            glColor3f(sinf(anim+4)*.25f + .75f, .5f, .5f);
            glVertex2f(m_rect[2]+m_leftOffset+offset,m_rect[1]);

            glTexCoord2f(.5f,0);
            glColor3f(sinf(anim+5)*.25f + .75f, .5f, .5f);
            glVertex2f(m_rect[2]+m_leftOffset+offset,m_rect[3]);

            glTexCoord2f(0,1);
            glColor3f(sinf(anim+6)*.25f + .75f, .5f, .5f);
            glVertex2f(m_rect[2]+m_leftOffset+m_textSize*.5f+offset,m_rect[1]);

            glTexCoord2f(0,0);
            glColor3f(sinf(anim+7)*.25f + .75f, .5f, .5f);
            glVertex2f(m_rect[2]+m_leftOffset+m_textSize*.5f+offset,m_rect[3]);
        glEnd();

        dkfBindFont(m_font);

        // Shadow
        glColor3f(0,0,0);
        dkfPrint(m_textSize, m_rect[0]+m_leftOffset+1, m_rect[1]+1, 0, textColorLess(m_caption).s);

        // Le standard pour les bouton
        switch (m_state)
        {
        case CONTROL_DOWN:
        case CONTROL_NOTHING:
            glColor3f(1,1,1);
            dkfPrint(m_textSize, m_rect[0]+m_leftOffset, m_rect[1], 0, m_caption.s);
            break;
        case CONTROL_OVER:
            glColor3f(.5f,.5f,1);
            dkfPrint(m_textSize, m_rect[0]+m_leftOffset, m_rect[1], 0, m_caption.s);
            break;
        }
    glPopAttrib();
}



//
// Pour changer la caption
//
void Control::changeCaption(CString caption)
{
    m_caption = caption;

    // On va chercher les dimension du string
    dkfBindFont(m_font);
    float w = dkfGetStringWidth(m_textSize, m_caption.s);
    float h = dkfGetStringHeight(m_textSize, m_caption.s);

    // On met à jour notre Rect avec ça
    switch (m_justify)
    {
    case JUSTIFY_LEFT:
        m_rect[0] = (float)m_screenPosition[0];
        m_rect[1] = (float)m_screenPosition[1];
        m_rect[2] = (float)m_screenPosition[0] + w;
        m_rect[3] = (float)m_screenPosition[1] + h;
        break;
    case JUSTIFY_CENTER:
        m_rect[0] = (float)m_screenPosition[0] - w/2;
        m_rect[1] = (float)m_screenPosition[1];
        m_rect[2] = (float)m_screenPosition[0] + w/2;
        m_rect[3] = (float)m_screenPosition[1] + h;
        break;
    case JUSTIFY_RIGHT:
        m_rect[0] = (float)m_screenPosition[0] - w;
        m_rect[1] = (float)m_screenPosition[1];
        m_rect[2] = (float)m_screenPosition[0];
        m_rect[3] = (float)m_screenPosition[1] + h;
        break;
    }
}

//
// Constructeur
//
ControlListener::ControlListener()
{
}

//
// Destructeur
//
ControlListener::~ControlListener()
{
    int i;
    ZEVEN_DELETE_VECTOR(m_controls, i);
}



//
// Update
//
void ControlListener::updateMenu(float delay)
{
/*  if (dkiGetState(DIK_ESCAPE) == DKI_DOWN)
    {
        if (m_controls.size() > 0)
        {
            onClick(m_controls[0]);
        }
    }*/

    for (int i=0;i<(int)m_controls.size();m_controls[i++]->update(delay));
    updateUnique(delay);
}



//
// Render
//
void ControlListener::renderMenu()
{
    dkglPushOrtho(800,600);
        glPushAttrib(GL_ENABLE_BIT);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBegin(GL_QUADS);
                glColor4f(0,0,0,.5f);
                glVertex2f(0,0);
                glVertex2f(0,600);
                glVertex2f(800,600);
                glVertex2f(800,0);
            glEnd();
            for (int i=0;i<(int)m_controls.size();m_controls[i++]->render());
            renderUnique();
        glPopAttrib();
    dkglPopOrtho();
}

//
// pour ajouter un control
//
void ControlListener::add(Control * control)
{
    m_controls.push_back(control);
}


//
// Constructeur
//
Button::Button(CVector2i screenPosition,
               CString caption,
               unsigned int m_font,
               float textSize,
               control_justify justify,
               ControlListener* listener)
: Control(screenPosition, caption, m_font, textSize, justify)
{
    m_listener = listener;
    m_sfxClic = dksCreateSoundFromFile("main/sounds/Button.wav", false);
}



//
// Destructeur
//
Button::~Button()
{
    dksDeleteSound(m_sfxClic);
}



//
// Pour quand on clic dessus, le child saura quoi faire
//
void Button::onClic()
{
    dksPlaySound(m_sfxClic, -1, 200);
    if (m_listener) m_listener->onClick(this);
}


//
// Constructeur
//
Choice::Choice(CVector2i screenPosition,
               CString caption,
               unsigned int m_font,
               float textSize,
               control_justify justify)
: Control(screenPosition, caption, m_font, textSize, justify)
{
    m_sfxClic = dksCreateSoundFromFile("main/sounds/Button.wav", false);
    m_choice = 0;
}



//
// Destructeur
//
Choice::~Choice()
{
    dksDeleteSound(m_sfxClic);
    m_choices.clear();
}



//
// Pour quand on clic dessus, le child saura quoi faire
//
void Choice::onClic()
{
    dksPlaySound(m_sfxClic, -1, 200);

    // On loop les choix
    m_choice++;
    if (m_choice >= (int)m_choices.size()) m_choice = 0;
    changeCaption(getChoice());
}



//
// Pour quand on clic dessus, le child saura quoi faire
//
void Choice::onRightClic()
{
    dksPlaySound(m_sfxClic, -1, 200);

    // On loop les choix
    m_choice--;
    if (m_choice < 0) m_choice = (int)m_choices.size() - 1;
    changeCaption(getChoice());
}



//
// Pour ajouter un choix
//
void Choice::addChoice(CString choice)
{
    m_choices.push_back(choice);
    if (m_choices.size() == 1)
    {
        changeCaption(m_choices[0]);
    }
}



//
// sets
//
void Choice::setChoice(int choice)
{
    m_choice = choice;
    if (m_choice < 0) m_choice = 0;
    if (m_choice >= (int)m_choices.size()) m_choice = 0;
    changeCaption(getChoice());
}

//
// Constructeur
//
Key::Key(CVector2i screenPosition, CString caption, unsigned int m_font, float textSize, control_justify justify)
: Control(screenPosition, caption, m_font, textSize, justify)
{
    m_isWaitingForKey = false;
    m_sfxClic = dksCreateSoundFromFile("main/sounds/Button.wav", false);
}



//
// Destructeur
//
Key::~Key()
{
    dksDeleteSound(m_sfxClic);
}



//
// sets
//
void Key::setKeyValue(int keyValue)
{
    m_keyValue = keyValue;
    changeCaption(CString("[ ") + keyManager.getKeyName(m_keyValue) + " ]");
    m_isWaitingForKey = false;
}



//
// L'updater le gèrer
//
void Key::update(float delay)
{
    if (m_leftOffset < 0)
    {
        m_leftOffset += delay * fabsf(m_leftOffset) * 4;
        if (m_leftOffset > 0) m_leftOffset = 0;
    }

    if (m_clicDelay > 0)
    {
        m_clicDelay -= delay;
        if (m_clicDelay < 0)
        {
            m_clicDelay = 0;
            m_state = CONTROL_NOTHING;
        }
    }

    // On check si la souri est dans notre Rectangle
    CVector2i mousePos = dkwGetCursorPos_main();
    CVector2i res = dkwGetResolution();

    // Il faut maintenant ajuster la mousePos dans notre 800x600
    mousePos[0] = (int)((float)mousePos[0] / (float)res[0] * 800.0f);
    mousePos[1] = (int)((float)mousePos[1] / (float)res[1] * 600.0f);

    // On check si on ne doit pas attendre pour une touche
    if (m_isWaitingForKey)
    {
        int key = dkiGetFirstDown();
        if (key != DKI_NOKEY)
        {
            dksPlaySound(m_sfxClic, -1, 200);
            setKeyValue(key);
        }
    }
    else
    {
        // On test maintenant avec le rectange du bouton
        if (mousePos[0] >= (int)m_rect[0]-10 &&
            mousePos[0] <= (int)m_rect[2]+10 &&
            mousePos[1] >= (int)m_rect[1] &&
            mousePos[1] <= (int)m_rect[3])
        {
            // Si on n'avait pas de state, on tombe over
            if (m_state == CONTROL_NOTHING)
            {
                dksPlaySound(m_sfxOver, -1, 150);
                m_state = CONTROL_OVER;
            }
            if (dkiGetState(DKI_MOUSE_BUTTON1) == DKI_DOWN)
            {
                m_state = CONTROL_DOWN;
                m_clicDelay = .1f;
                onClic();
            }
        }
        else if (m_state == CONTROL_OVER)
        {
            m_state = CONTROL_NOTHING;
        }
    }
}



//
// Pour quand on clic dessus, il faut activer le wait for key hehe (SICK)
//
void Key::onClic()
{
    if (m_isWaitingForKey == false)
    {
        dksPlaySound(m_sfxClic, -1, 200);
        m_isWaitingForKey = true;
        changeCaption("\x4HIT KEY");
    }
}

//
// Constructeur
//
Label::Label(CVector2i screenPosition, CString caption, unsigned int m_font, float textSize, control_justify justify)
: Control(screenPosition, caption, m_font, textSize, justify)
{
    m_isMouseHandle = false;
}



//
// Pour l'afficher
//
void Label::render()
{
    glPushAttrib(GL_CURRENT_BIT);

        dkfBindFont(m_font);

        // Shadow
        glColor3f(0,0,0);
        dkfPrint(m_textSize, m_rect[0]+m_leftOffset+1, m_rect[1]+1, 0, textColorLess(m_caption).s);

        // Le standard pour les bouton
        switch (m_state)
        {
        case CONTROL_DOWN:
        case CONTROL_NOTHING:
            glColor3f(1,1,1);
            dkfPrint(m_textSize, m_rect[0]+m_leftOffset, m_rect[1], 0, m_caption.s);
            break;
        case CONTROL_OVER:
            glColor3f(.5f,.5f,1);
            dkfPrint(m_textSize, m_rect[0]+m_leftOffset, m_rect[1], 0, m_caption.s);
            break;
        }
    glPopAttrib();
}

//
// Constructeur
//
Write::Write(CVector2i screenPosition,
               CString caption,
               unsigned int m_font,
               float textSize,
               control_justify justify,
               int flag)
: Control(screenPosition, caption, m_font, textSize, justify)
{
    validationType = flag;
    set("%s", caption.s);
    m_sfxClic = dksCreateSoundFromFile("main/sounds/Button.wav", false);
}



//
// Destructeur
//
Write::~Write()
{
    loseFocus();
    dksDeleteSound(m_sfxClic);
}



//
// L'updater le gèrer
//
void Write::update(float delay)
{
    if (m_leftOffset < 0)
    {
        m_leftOffset += delay * fabsf(m_leftOffset) * 4;
        if (m_leftOffset > 0) m_leftOffset = 0;
    }

    if (m_clicDelay > 0)
    {
        m_clicDelay -= delay;
        if (m_clicDelay < 0)
        {
            m_clicDelay = 0;
            m_state = CONTROL_NOTHING;
        }
    }

    // On check si la souri est dans notre Rectangle
    CVector2i mousePos = dkwGetCursorPos_main();
    CVector2i res = dkwGetResolution();

    // Il faut maintenant ajuster la mousePos dans notre 800x600
    mousePos[0] = (int)((float)mousePos[0] / (float)res[0] * 800.0f);
    mousePos[1] = (int)((float)mousePos[1] / (float)res[1] * 600.0f);

    if (isActivated())
    {
        loseFocus();
        dksPlaySound(m_sfxClic, -1, 200);

        if (validationType == VALIDATE_INT)
        {
            int value = toInt();
            set("%i", value);
        }
        if (validationType == VALIDATE_FLOAT)
        {
            float value = toFloat();
            set("%.02f", value);
        }

        if (len() != 0) changeCaption(*this);
        else set("%s", m_caption.s);
    }

    if (haveFocus())
    {
        changeCaption(*this); // On change la caption en tout temps pour updater le sizing du control
        if (dkiGetState(DKI_MOUSE_BUTTON1) == DKI_DOWN)
        {
            if (len() != 0) changeCaption(*this);
            else set("%s", m_caption.s);
            loseFocus();
            dksPlaySound(m_sfxClic, -1, 200);
        }
    }
    else
    {
        // On test maintenant avec le rectange du bouton
        if (mousePos[0] >= (int)m_rect[0]-10 &&
            mousePos[0] <= (int)m_rect[2]+10 &&
            mousePos[1] >= (int)m_rect[1] &&
            mousePos[1] <= (int)m_rect[3])
        {
            // Si on n'avait pas de state, on tombe over
            if (m_state == CONTROL_NOTHING)
            {
                dksPlaySound(m_sfxOver, -1, 150);
                m_state = CONTROL_OVER;
            }
            if (dkiGetState(DKI_MOUSE_BUTTON1) == DKI_DOWN)
            {
                m_state = CONTROL_DOWN;
                m_clicDelay = .1f;
                onClic();
            }
        }
        else if (m_state == CONTROL_OVER)
        {
            m_state = CONTROL_NOTHING;
        }
    }
}



//
// Pour l'afficher
//
void Write::render()
{
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        // Le contour
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex_menu);
        float offset = 0;
        switch (m_state)
        {
        case CONTROL_DOWN:
        case CONTROL_NOTHING:
            glColor3f(.5f,1,0);
            break;
        case CONTROL_OVER:
            glColor3f(0,1,.5f);
            offset = 8;
            break;
        }
        glBegin(GL_QUAD_STRIP);
            glTexCoord2f(0,1);
            glVertex2f(m_rect[0]+m_leftOffset-m_textSize*.5f-offset,m_rect[1]);
            glTexCoord2f(0,0);
            glVertex2f(m_rect[0]+m_leftOffset-m_textSize*.5f-offset,m_rect[3]);
            glTexCoord2f(.5f,1);
            glVertex2f(m_rect[0]+m_leftOffset-offset,m_rect[1]);
            glTexCoord2f(.5f,0);
            glVertex2f(m_rect[0]+m_leftOffset-offset,m_rect[3]);
            glTexCoord2f(.5f,1);
            glVertex2f(m_rect[2]+m_leftOffset+offset,m_rect[1]);
            glTexCoord2f(.5f,0);
            glVertex2f(m_rect[2]+m_leftOffset+offset,m_rect[3]);
            glTexCoord2f(0,1);
            glVertex2f(m_rect[2]+m_leftOffset+m_textSize*.5f+offset,m_rect[1]);
            glTexCoord2f(0,0);
            glVertex2f(m_rect[2]+m_leftOffset+m_textSize*.5f+offset,m_rect[3]);
        glEnd();

        if (haveFocus())
        {
            // Le standard pour les bouton
            switch (m_state)
            {
            case CONTROL_NOTHING:
                glColor3f(1,1,1);
                print(m_textSize, m_rect[0]+m_leftOffset, m_rect[1], 0);
                break;
            case CONTROL_OVER:
                glColor3f(.5f,.5f,1);
                print(m_textSize, m_rect[0]+m_leftOffset, m_rect[1], 0);
                break;
            case CONTROL_DOWN:
                glColor3f(1,.5f,.5f);
                print(m_textSize, m_rect[0]+m_leftOffset+2, m_rect[1]+2, 0);
                break;
            }
        }
        else
        {
            // Le standard pour les bouton
            switch (m_state)
            {
            case CONTROL_NOTHING:
                glColor3f(1,1,1);
                print(m_textSize, m_rect[0]+m_leftOffset, m_rect[1], 0);
                break;
            case CONTROL_OVER:
                glColor3f(.5f,.5f,1);
                print(m_textSize, m_rect[0]+m_leftOffset, m_rect[1], 0);
                break;
            case CONTROL_DOWN:
                glColor3f(1,.5f,.5f);
                print(m_textSize, m_rect[0]+m_leftOffset+2, m_rect[1]+2, 0);
                break;
            }
        }
    glPopAttrib();
}



//
// Pour quand on clic dessus, le child saura quoi faire
//
void Write::onClic()
{
    giveFocus();
    dksPlaySound(m_sfxClic, -1, 200);
}
