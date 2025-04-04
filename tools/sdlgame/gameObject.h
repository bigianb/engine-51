#pragma once

#include "system/SDL_Engine.h"
#include "system/SDL_Renderer.h"

class LevelLoader;
class Level;
class FileSystem;
class ResourceManager;
class ResourceLoaders;
class ObjectManager;
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
    Level* level;
    FileSystem* fs;
    ResourceManager* resourceManager;
    ResourceLoaders* resourceLoaders;
    ObjectManager* objectManager;
    ui::Manager* uiManager;
    SDLEngine* engine;
    StateMachine* stateMachine;
    SDLRenderer* renderer;
};
