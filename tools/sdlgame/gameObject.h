#pragma once

#include "system/SDL_Engine.h"
#include "system/SDL_Renderer.h"

class LevelLoader;
class FileSystem;
class ResourceManager;
class ResourceLoaders;
namespace ui {
    class Manager;
}
class StateMachine;

/*
 * Used to hold game global objects.
*/
class GameObject
{
    public:
        GameObject();
        ~GameObject();
        bool init();
        void quit();

    LevelLoader* levelLoader;
    FileSystem* fs;
    ResourceManager* resourceManager;
    ResourceLoaders* resourceLoaders;
    ui::Manager* uiManager;
    SDLEngine* engine;
    StateMachine* stateMachine;
    SDLRenderer* renderer;
};
