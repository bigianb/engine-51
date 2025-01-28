#include "LevelLoader.h"

#include <set>
#include <iostream>

#include "../io/FileSystem.h"
#include "../DFSFile.h"
#include "../resourceManager/ResourceManager.h"

void LevelLoader::mountDefaultFilesystems(FileSystem& fs)
{
    fs.mount("BOOT", 1);
    fs.mount("PRELOAD", 1);
    fs.mount("AUDIO/MUSIC", 10);
    fs.mount("AUDIO/VOICE", 11);
    fs.mount("AUDIO/HOT", 12);
    fs.mount("AUDIO/AMBIENT", 12);
    fs.mount("STRINGS", 13);
    fs.mount("COMMON", 14);
}

void LevelLoader::loadDFS(FileSystem& fs, ResourceManager* rm, std::string dfsName)
{
    static const std::set<std::string> extensions{
        ".XBMP",
        ".RIGIDGEOM",
        ".SKINGEOM",
        ".ANIM",
        ".DECALPKG",
        ".ENVMAP",
        ".RIGIDCOLOR",
        ".STRINGBIN",
        ".FXO",
        ".AUDIOPKG",
        ".FONT"};

    DFSFile* dfsFile = fs.getMount(dfsName);
    if (dfsFile == nullptr) {
        std::cout << "ERROR: could not find mount for " << dfsName << std::endl;
        return;
    }
    int numFiles = dfsFile->numFiles();
    for (int i = 0; i < numFiles; ++i) {
        std::string extension = dfsFile->getFileExtension(i);
        //std::cout << "Extension is " << extension << std::endl;
        if (extensions.contains(extension)) {
            //std::cout << "Reading " << dfsFile->getFilename(i) << std::endl;
            ResourceHandleBase handle(rm);
            handle.setName(dfsFile->getFilename(i));
            handle.getPointer();
        }
    }
}
