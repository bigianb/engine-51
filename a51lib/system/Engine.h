#pragma once

#include "InputGadget.h"

/* Generic engine. Implement system specific ones. */
class Engine
{
public:
    virtual ~Engine() {}
    virtual void init() = 0;

    virtual bool  input_IsPressed(InputGadget gadget, int controllerID = 0) = 0;
    virtual bool  input_WasPressed(InputGadget gadget, int controllerID = 0) = 0;
    virtual float input_GetValue(InputGadget gadget, int controllerID = 0) = 0;
};
