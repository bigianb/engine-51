#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "gameObject.h"
#include "../../a51lib/state/StateMachine.h"
#include "../../a51lib/levels/LevelLoader.h"
#include "../../a51lib/objectManager/ObjectManager.h"
#include "system/SDL_Renderer.h"
#include "../../a51lib/view/View.h"
#include "../../a51lib/objects/Player.h"

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

void SetupViewAndFog( GameObject& gameObject, zone_mgr::zone_id StartZone )
{
    texture::handle FogPalette;

    // get the default fog palette and z buffer settings
    bool   QuickFog = false;
    slot_id SlotID = gameObject.objectManager->GetFirst( Object::TYPE_LEVEL_SETTINGS );
    /* IJB TODO
    if ( SlotID != SLOT_NULL )
    {
        Object* pObject = gameObject.objectManager->GetObjectBySlot( SlotID );
        
        level_settings& Settings = level_settings::GetSafeType( *pObject );
        g_View.SetZLimits( 10.0f, Settings.GetFarPlane() );

        FogPalette = Settings.GetFogPalette();
        
    }
    else
    */
    {
        g_View.SetZLimits( 10.0f, 8000.0f );
    }

    // if the zone we're in has a different fog from the default, use
    // that one instead
    const char* pFog = g_ZoneMgr.GetZoneFog(StartZone,QuickFog);
    if ( *pFog != '\0' ){
        FogPalette.setName( pFog );
    }

    // set the pixel scale (aspect ratio)
    g_View.SetPixelScale();

    // Set the viewport
    //eng_SetView( g_View );

    // Set the fog
    //render::SetCustomFogPalette( FogPalette, QuickFog, g_RenderContext.LocalPlayerIndex );
}


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
                

                player* pPlayers[MAX_LOCAL_PLAYERS] = { 0 };
                slot_id ID                          = gameObject.objectManager->GetFirst( Object::TYPE_PLAYER );
                int     nPlayers                    = 0;

                while( ID != SLOT_NULL )
                {
                    Object* pObj    = gameObject.objectManager->GetObjectBySlot(ID);
                    player* pPlayer = &player::GetSafeType( *pObj );

                    if( pPlayer && (pPlayer->GetLocalSlot() != -1) )
                    {
                        pPlayers[ pPlayer->GetLocalSlot() ] = pPlayer;
                        nPlayers++;
                    }

                    ID = gameObject.objectManager->GetNext(ID);
                }

                assert(nPlayers > 0);

                // For now assume only one local player.
                // one view, set it to the entire screen
                view& rView0 = pPlayers[0]->GetView();
                int XRes, YRes;
                gameObject.renderer->getRes(XRes, YRes);
                rView0.SetViewport( 0, 0, XRes, YRes );
                pPlayers[0]->ComputeView( rView0 );

                //if ( !g_FreeCam )
                {
                    view& rView0 = pPlayers[0]->GetView();
                    pPlayers[0]->ComputeView( rView0, player::VIEW_NULL );
                    g_View = rView0;
                }
                
                uint8_t playerViewZone = pPlayers[0]->GetPlayerViewZone();
                SetupViewAndFog(gameObject, playerViewZone);
                gameObject.objectManager->Render(true, g_View, playerViewZone, gameObject.renderer);
            }
        }
        gameObject.renderer->draw();
    }

    SDL_Log("Done");
    return 0;
}
