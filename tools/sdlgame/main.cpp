#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "gameObject.h"
#include "../../a51lib/state/StateMachine.h"
#include "system/SDL_Renderer.h"

struct Context
{
    const char*    ExampleName;
    const char*    BasePath;
    SDL_Window*    Window;
    SDL_GPUDevice* Device;
    bool           LeftPressed;
    bool           RightPressed;
    bool           DownPressed;
    bool           UpPressed;
    float          DeltaTime;
};

int CommonInit(Context* context, SDL_WindowFlags windowFlags)
{
    context->Device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
        false,
        NULL);

    if (context->Device == NULL) {
        SDL_Log("GPUCreateDevice failed");
        return -1;
    }

    context->Window = SDL_CreateWindow(context->ExampleName, 640, 480, windowFlags);
    if (context->Window == NULL) {
        SDL_Log("CreateWindow failed: %s", SDL_GetError());
        return -1;
    }

    if (!SDL_ClaimWindowForGPUDevice(context->Device, context->Window)) {
        SDL_Log("GPUClaimWindow failed");
        return -1;
    }

    return 0;
}

void CommonQuit(Context* context)
{
    SDL_ReleaseWindowFromGPUDevice(context->Device, context->Window);
    SDL_DestroyWindow(context->Window);
    SDL_DestroyGPUDevice(context->Device);
}

int main(int argc, char** argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }
    Context      context;
    float        lastTime = 0;
    SDL_Gamepad* gamepad = nullptr;

    CommonInit(&context, 0);

    GameObject gameObject;
    gameObject.init();

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
        }
    }

    CommonQuit(&context);

    SDL_Log("Done");
    return 0;
}
