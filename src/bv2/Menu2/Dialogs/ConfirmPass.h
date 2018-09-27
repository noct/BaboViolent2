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

#ifndef CONFIRMPASS_H
#define CONFIRMPASS_H

#include <Zeven/Zeven.h>
#include "CControl.h"
#include "Dialog.h"

class ConfirmPass : public IDialog
{
public:
    // Used for sub dialogs (errors, warnings)
    IDialog * subDialog;
    // Controls
    CControl * lbl_confirmPass;
    CControl * txt_pass;
public:
    // Constructor/Destructor
    ConfirmPass(unsigned int font, CVector2i size = CVector2i(32, 32));
    ~ConfirmPass();

    // update
    virtual void update(float delay);

    // render
    virtual void render();

    // Events
    virtual void Click(CControl * control);
    virtual void Validate(CControl * control);

    // Dialog events
    virtual void OnOk();
};


#endif
