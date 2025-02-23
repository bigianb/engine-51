#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "gameObject.h"
#include "../../a51lib/state/StateMachine.h"
#include "system/SDL_Renderer.h"

struct Context
{
    const char*    ExampleName;
    const char*    BasePath;

    bool           LeftPressed;
    bool           RightPressed;
    bool           DownPressed;
    bool           UpPressed;
    float          DeltaTime;
};


int main(int argc, char** argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }
    Context      context;
    float        lastTime = 0;
    SDL_Gamepad* gamepad = nullptr;

    GameObject gameObject;
    if (!gameObject.init()){
        return 1;
    }

    bool quit = false;
    while (!quit) {
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
            }
        }
        if (quit) {
            break;
        }
        float newTime = SDL_GetTicks() / 1000.0f;
        context.DeltaTime = newTime - lastTime;
        lastTime = newTime;

        float deltaTime = 1.0 / 30.0;
        gameObject.stateMachine->update(context.DeltaTime);
        if (gameObject.stateMachine->getState() != StateMachine::State::playing_game) {

            gameObject.uiManager->render(*gameObject.renderer);
            gameObject.renderer->draw();
        }
    }

    SDL_Log("Done");
    return 0;
}
