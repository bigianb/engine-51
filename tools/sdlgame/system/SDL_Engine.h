#pragma once

#include "../../../a51lib/system/Engine.h"

class SDLEngine : public Engine
{
public:
    void init() override {}

    bool  input_IsPressed(InputGadget gadget, int controllerID = 0) override;
    bool  input_WasPressed(InputGadget gadget, int controllerID = 0) override;
    float input_GetValue(InputGadget gadget, int controllerID = 0) override;

};
