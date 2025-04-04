#pragma once

#include <string>
#include <thread>
#include <cassert>

#include "../state/MapList.h"
#include "Level.h"

class FileSystem;
class ResourceManager;
class ObjectManager;
class map_entry;

class LevelLoader
{
public:
    LevelLoader(FileSystem* fs, ResourceManager* rm, ObjectManager* om)
        : fs(fs), resourceManager(rm), objectManager(om), level(nullptr)
    {
        assert(fs != nullptr);
    }
    ~LevelLoader()
    {
        if (loadThread != nullptr) {
            loadThread->join();
            delete loadThread;
        }
    }
    void mountDefaultFilesystems();
    void loadDFS(std::string name);

    /* If a level isn't already loading, start a thread to load the level. */
    void loadLevel(bool fullLoad, const map_entry* pMapEntry, Level* level);

    void finishLoading();

    bool isLoading() const
    {
        return loadRequested;
    }

    bool isLevelLoaded() const
    {
        return levelLoaded;
    }

    void InitSlideShow(const char* pSlideShowScriptFile);

private:
    FileSystem*  fs;
    ResourceManager* resourceManager;
    ObjectManager* objectManager;
    // The level that is being loaded. We don't own this, it is passed into loadLevel.
    Level* level;

    std::thread* loadThread = nullptr;

    void loadLevelThreadFunction();

    // Thred has been spun up to load a level. levelLoaded tells us if it is finished.
    bool loadRequested = false;

    // Indicates whether the level loading thread has finished;
    bool levelLoaded = false;

    // The level to lead
    const map_entry* mapEntry;

    bool fullLoad = false;
};
