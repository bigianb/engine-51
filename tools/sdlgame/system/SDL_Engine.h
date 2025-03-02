#pragma once

#include <SDL3/SDL.h>
#include "../../../a51lib/system/Engine.h"

class SDLEngine : public Engine
{
public:
    void init() override;

    bool  input_IsPressed(InputGadget gadget, int controllerID = 0) override;
    bool  input_WasPressed(InputGadget gadget, int controllerID = 0) override;
    float input_GetValue(InputGadget gadget, int controllerID = 0) override;

    void processEventQueue();
    bool isQuitRequested() { return quit; }

    private:
        void setKey(SDL_Keycode key, bool down);

        SDL_Gamepad* gamepad;
        bool quit;

        bool keyPressed[static_cast<int>(InputGadget::NUM_INPUT_GADGETS)];
};
