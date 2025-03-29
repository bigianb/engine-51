#pragma once

#include <string>
#include <thread>

class FileSystem;
class ResourceManager;

class LevelLoader
{
public:
    void mountDefaultFilesystems(FileSystem& fs);
    void loadDFS(FileSystem& fs,  ResourceManager* rm, std::string name);

    /* If a level isn't already loading, start a thread to load the level. */
    void loadLevel(bool fullLoad);

    void finishLoading();

    bool isLoading() const
    {
        return loadRequested;
    }

    bool isLevelLoaded() const
    {
        return levelLoaded;
    }

private:
    std::thread* loadThread = nullptr;;

    void loadLevelThreadFunction();

    // Thred has been spun up to load a level. levelLoaded tells us if it is finished.
    bool loadRequested = false;

    // Indicates whether the level loading thread has finished;
    bool levelLoaded = false;

    bool fullLoad = false;
};
