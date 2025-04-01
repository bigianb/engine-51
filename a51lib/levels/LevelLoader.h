#pragma once

#include <string>
#include <thread>
#include <cassert>

#include "../state/MapList.h"

class FileSystem;
class ResourceManager;
class map_entry;

class LevelLoader
{
public:
    LevelLoader(FileSystem* fs, ResourceManager* rm)
        : fs(fs), resourceManager(rm)
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
    void loadLevel(bool fullLoad, const map_entry* pMapEntry);

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
