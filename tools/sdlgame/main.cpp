#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "gameObject.h"
#include "../../a51lib/state/StateMachine.h"
#include "../../a51lib/levels/LevelLoader.h"
#include "../../a51lib/objectManager/ObjectManager.h"
#include "system/SDL_Renderer.h"
#include "../../a51lib/view/View.h"

view g_View;

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

void RenderGame();

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

    int logicFramesAfterLoad = 0;
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
                gameObject.levelLoader->loadLevel(bFullLevelLoad, mapEntry, gameObject.level, gameObject.playsurfaceManager);
            } else {
                // check if the level is loaded
                if (gameObject.levelLoader->isLevelLoaded()) {
                    gameObject.levelLoader->finishLoading();
                    gameObject.levelLoader->spawnPlayer();
                    gameObject.stateMachine->setState(StateMachine::State::playing_game);
                    logicFramesAfterLoad = 0;
                }
            }
        }
        if (gameObject.stateMachine->getState() != StateMachine::State::playing_game) {
            gameObject.uiManager->update(context.DeltaTime);
            gameObject.uiManager->render(*gameObject.renderer);
        } else {
            if (logicFramesAfterLoad == 0) {
                std::cout << "Playing game..." << std::endl;
            }
            logicFramesAfterLoad++;
/*
            render::Update(DeltaTime);
            g_TracerMgr.OnUpdate(DeltaTime);

            corpse::LimitCount();

            g_PhysicsMgr.Advance(DeltaTime);
            {
                STAT_LOGGER(temp, k_stats_OnAdvance);
                g_ObjMgr.AdvanceAllLogic(DeltaTime);
            }
            g_LightMgr.OnUpdate(DeltaTime);
            g_PostEffectMgr.OnUpdate(DeltaTime);
            g_DecalMgr.OnUpdate(DeltaTime);
            g_AlienGlobMgr.Advance(DeltaTime);
            g_TriggerExMgr.OnUpdate(DeltaTime);
            g_NetworkMgr.Update( DeltaTime );       // runs the character logic
            g_GameTextMgr.Update(DeltaTime);
            UpdateAudio(DeltaTime);
*/
            if (logicFramesAfterLoad > 10) {
                uint8_t playerViewZone = 0;
                gameObject.objectManager->Render(true, g_View, playerViewZone);
                //g_ObjMgr.Render(TRUE,g_View,pPlayers[i]->GetPlayerViewZone());
            }
        }
        gameObject.renderer->draw();
    }

    SDL_Log("Done");
    return 0;
}
