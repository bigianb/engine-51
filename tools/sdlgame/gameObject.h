#pragma once


class LevelLoader;
class FileSystem;
class ResourceManager;
class ResourceLoaders;
namespace ui {
    class Manager;
}
class Engine;
class StateMachine;
class SDLRenderer;

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
    Engine* engine;
    StateMachine* stateMachine;
    SDLRenderer* renderer;
};
