#include "SDL_Engine.h"

void SDLEngine::init()
{
    gamepad = nullptr;
    quit = false;
    memset(keyPressed, 0, sizeof(keyPressed));
}

bool SDLEngine::input_IsPressed(InputGadget gadget, int controllerID)
{
    return keyPressed[static_cast<int>(gadget)];
}

bool SDLEngine::input_WasPressed(InputGadget gadget, int controllerID)
{
    return keyPressed[static_cast<int>(gadget)];
}

float SDLEngine::input_GetValue(InputGadget gadget, int controllerID)
{
    return 0.0;
}

void SDLEngine::processEventQueue()
{
    SDL_Event evt;
    while (SDL_PollEvent(&evt)) {
        if (evt.type == SDL_EVENT_QUIT) {
            quit = true;
        } else if (evt.type == SDL_EVENT_GAMEPAD_ADDED) {
            if (gamepad == nullptr) {
                gamepad = SDL_OpenGamepad(evt.gdevice.which);
            }
        } else if (evt.type == SDL_EVENT_GAMEPAD_REMOVED) {
            if (evt.gdevice.which == SDL_GetGamepadID(gamepad)) {
                SDL_CloseGamepad(gamepad);
            }
        } else if (evt.type == SDL_EVENT_KEY_DOWN) {
            setKey(evt.key.key, true);
        } else if (evt.type == SDL_EVENT_KEY_UP) {
            setKey(evt.key.key, false);
        } else if (evt.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {

        } else if (evt.type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
        }
    }
}

void SDLEngine::setKey(SDL_Keycode key, bool down)
{
    InputGadget gadget = InputGadget::Undefined;
    switch (key) {
    case SDLK_X:
        gadget = InputGadget::INPUT_PS2_BTN_CROSS;
        break;
    case SDLK_UP:
        gadget = InputGadget::INPUT_PS2_BTN_L_UP;
        break;
    case SDLK_DOWN:
        gadget = InputGadget::INPUT_PS2_BTN_L_DOWN;
        break;
    case SDLK_LEFT:
        gadget = InputGadget::INPUT_PS2_BTN_L_LEFT;
        break;
    case SDLK_RIGHT:
        gadget = InputGadget::INPUT_PS2_BTN_L_RIGHT;
        break;
        // TODO: Implement
    }
    keyPressed[static_cast<int>(gadget)] = down;
}
