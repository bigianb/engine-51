#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "gameObject.h"
#include "../../a51lib/state/StateMachine.h"
#include "../../a51lib/levels/LevelLoader.h"
#include "system/SDL_Renderer.h"

struct Context
{
    const char* ExampleName;
    const char* BasePath;

    bool  LeftPressed;
    bool  RightPressed;
    bool  DownPressed;
    bool  UpPressed;
    float DeltaTime;
};

int main(int argc, char** argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }
    Context context;
    float   lastTime = 0;

    GameObject gameObject;
    if (!gameObject.init()) {
        return 1;
    }

    bool bFullLevelLoad = true;
    while (!gameObject.engine->isQuitRequested()) {
        gameObject.engine->processEventQueue();
        if (gameObject.engine->isQuitRequested()) {
            break;
        }
        float newTime = SDL_GetTicks() / 1000.0f;
        context.DeltaTime = newTime - lastTime;
        lastTime = newTime;

        float deltaTime = 1.0 / 30.0;
        if (gameObject.stateMachine->getState() != StateMachine::State::playing_game) {
            gameObject.uiManager->processInput(gameObject.engine, context.DeltaTime);
        }
        gameObject.stateMachine->update(context.DeltaTime);

        if (gameObject.stateMachine->getState() == StateMachine::State::single_player_load_mission) {
            // kick off the load if we are not already loading
            if (!gameObject.levelLoader->isLoading()) {
                const map_entry* mapEntry = gameObject.stateMachine->getActiveMap();
                gameObject.levelLoader->loadLevel(bFullLevelLoad, mapEntry);
            } else {
                // check if the level is loaded
                if (gameObject.levelLoader->isLevelLoaded()) {
                    gameObject.levelLoader->finishLoading();
                    gameObject.stateMachine->setState(StateMachine::State::playing_game);
                }
            }
        }
        if (gameObject.stateMachine->getState() != StateMachine::State::playing_game) {
            gameObject.uiManager->update(context.DeltaTime);
            gameObject.uiManager->render(*gameObject.renderer);
            gameObject.renderer->draw();
        }
    }

    SDL_Log("Done");
    return 0;
}
