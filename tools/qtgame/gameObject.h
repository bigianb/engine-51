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
class Renderer;

/*
 * Used to hold game global objects.
*/
class GameObject
{
    public:
        GameObject();
        ~GameObject();
        void init();

    LevelLoader* levelLoader;
    FileSystem* fs;
    ResourceManager* resourceManager;
    ResourceLoaders* resourceLoaders;
    ui::Manager* uiManager;
    Engine* engine;
    StateMachine* stateMachine;
    Renderer* renderer;
};
