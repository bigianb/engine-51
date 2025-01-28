#pragma once

#include <string>

class FileSystem;
class ResourceManager;

class LevelLoader
{
public:
    void mountDefaultFilesystems(FileSystem& fs);
    void loadDFS(FileSystem& fs,  ResourceManager* rm, std::string name);
};
